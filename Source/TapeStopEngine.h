#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "RtSafety.h"
#include <atomic>
#include <vector>
#include <cmath>

/**
    TapeStopEngine

    The DSP heart of the tape-stop effect, deliberately kept free of any host /
    plugin plumbing so it can be unit-tested or reused on its own.

    Model: a variable-rate resampler. Each channel owns a ring buffer that is
    written at a constant rate (one sample in, one sample out). A single, shared
    read pointer advances by the current `speed` every sample:

        speed == 1  -> read keeps pace with write  -> transparent / unity
        speed  < 1  -> read falls behind write     -> slower + pitched down
        speed -> 0  -> read pointer freezes         -> the tape has stopped

    `speed` is driven by a normalized ramp `phase` in [0, 1] so the curve knob is
    meaningful:  speed = (1 - phase) ^ (1 + curve * kCurveExp).
    Engaging Stop ramps phase 0->1 over `stopTime`; releasing ramps it 1->0 over
    `startTime`. The ramp itself is the smoother, so `speed` never jumps -> no
    clicks. A short gain fade in the last few percent guarantees a clean,
    DC-free silence at full stop.

    Returning to live: while slowing/stopped the read pointer lags the write
    pointer; because `speed` is capped at 1, that lag would otherwise be
    permanent (and accumulate across gestures). On release the engine resyncs the
    reader back to the write head so the effect returns to transparent, zero-
    latency passthrough. Two behaviours are selectable via `ReturnMode`:

        Snap   -> rejoin the live signal instantly (jump to the write head while
                  output is silent, i.e. fully stopped; click-free). If released
                  mid-fade while still audible, falls back to a quick catch-up.
        SpinUp -> turntable style: the reader runs faster than the writer
                  (capped at kCatchUpRate) until it reaches the write head, so you
                  hear the tape accelerate through the buffered audio and lock in.
*/
class TapeStopEngine
{
public:
    /** How the tape rejoins the live signal when Stop is released. */
    enum class ReturnMode { SpinUp, Snap };

    TapeStopEngine() = default;

    /** Allocates all buffers. Must be called (off the audio thread) before
        process(). `maxStopTimeSeconds` bounds the worst-case read lag and thus
        the ring-buffer size. */
    void prepare (double sampleRate, int numChannels, double maxStopTimeSeconds)
    {
        sr = sampleRate;

        // Worst-case the reader can fall behind the writer is roughly the full
        // spin-down time (reader frozen while writer runs). Cap the lag there
        // and size the ring to hold it, rounded up to a power of two for cheap
        // mask-wrapping. A little headroom covers the +1 interpolation tap.
        maxLag = (long long) std::ceil (maxStopTimeSeconds * sr) + 8;

        long long n = 2;
        while (n < maxLag + 8)
            n <<= 1;
        bufferSize = n;
        mask = bufferSize - 1;

        channels.assign ((size_t) juce::jmax (1, numChannels),
                         std::vector<float> ((size_t) bufferSize, 0.0f));

        reset();
    }

    /** Clears history and returns to a transparent, at-rest state. */
    void reset()
    {
        for (auto& ch : channels)
            std::fill (ch.begin(), ch.end(), 0.0f);

        writeIndex = 0;
        readPos    = 0.0;
        phase      = 0.0;
        speed      = 1.0;

        speedPhase = -1.0;          // force advanceRamp to recompute on first use
        speedCurve = -1.0f;
        cachedGain = 1.0f;

        speedPublished.store (1.0f, std::memory_order_relaxed);
        phasePublished.store (0.0f, std::memory_order_relaxed);
    }

    /** The automatable Stop toggle. Engaging winds the tape down; releasing
        winds it back up. */
    void setStopEngaged (bool shouldStop) noexcept { engaged = shouldStop; }

    /** Spin-down / spin-up durations in milliseconds. */
    void setTimes (float stopTimeMs, float startTimeMs) noexcept
    {
        stopMs  = juce::jmax (1.0f, stopTimeMs);
        startMs = juce::jmax (1.0f, startTimeMs);
    }

    /** 0 = linear slowdown, 1 = heavy exponential (fast drop, long linger). */
    void setCurve (float curve01) noexcept { curve = juce::jlimit (0.0f, 1.0f, curve01); }

    /** Selects how the tape rejoins the live signal on release. */
    void setReturnMode (ReturnMode m) noexcept { returnMode = m; }

    /** Live playback rate in [0, 1] (1 = full speed, 0 = stopped). Published
        once per processed block; safe to read from any thread (e.g. the editor's
        UI timer) without locking. */
    float getSpeed() const noexcept { return speedPublished.load (std::memory_order_relaxed); }

    /** Live ramp position in [0, 1] (0 = running, 1 = fully stopped). Published
        once per processed block; thread-safe to read from the UI. */
    float getPhase() const noexcept { return phasePublished.load (std::memory_order_relaxed); }

    /** Processes a block in place. One output sample per input sample.

        Runs on the audio thread, so it must never allocate, lock or syscall.
        TAPESTOP_RT_NONBLOCKING marks it [[clang::nonblocking]] under the RTSan
        build, which enforces exactly that (no-op everywhere else). All buffers
        are allocated up front in prepare(). */
    void process (juce::AudioBuffer<float>& buffer) noexcept TAPESTOP_RT_NONBLOCKING
    {
        const int numCh      = juce::jmin (buffer.getNumChannels(), (int) channels.size());
        const int numSamples = buffer.getNumSamples();

        for (int s = 0; s < numSamples; ++s)
        {
            // 1) Write the incoming sample for every channel at the same index.
            const long long w = writeIndex & mask;
            for (int ch = 0; ch < numCh; ++ch)
                channels[(size_t) ch][(size_t) w] = buffer.getSample (ch, s);

            // 2) Read each channel at the shared fractional position (linear interp).
            const long long i0   = (long long) std::floor (readPos);
            const float     frac = (float) (readPos - (double) i0);
            const long long a0   = i0 & mask;
            const long long a1   = (i0 + 1) & mask;

            // Fade to silence over the last few percent of the ramp so a fully
            // stopped tape lands on clean silence rather than a held DC tone.
            // gain is a pure function of speed and is recomputed only when speed
            // changes (in advanceRamp / on Snap), so steady states cost nothing here.
            const float gain = cachedGain;

            for (int ch = 0; ch < numCh; ++ch)
            {
                const auto& buf = channels[(size_t) ch];
                const float v   = buf[(size_t) a0] + frac * (buf[(size_t) a1] - buf[(size_t) a0]);
                buffer.setSample (ch, s, v * gain);
            }

            // 3) Advance writer (by 1) and reader. While Stop is released the
            //    reader must close the lag it built up so the effect returns to
            //    transparent, zero-latency passthrough (otherwise the latency is
            //    permanent and accumulates across gestures).
            ++writeIndex;

            double readInc = speed;   // curve/ramp-driven base rate, in [0, 1]

            if (! engaged)
            {
                const double gap = (double) writeIndex - readPos; // samples behind live

                if (returnMode == ReturnMode::Snap && gain <= kSnapSilenceGain)
                {
                    // Fully (or near) stopped: output is silent, so we can jump
                    // straight to the live edge and resume at full speed with no
                    // click. The Start Time ramp is intentionally bypassed here.
                    readPos = (double) writeIndex;
                    phase   = 0.0;
                    speed   = 1.0;
                    readInc = 0.0;
                    speedPhase = -1.0;   // invalidate cache: advanceRamp re-derives speed
                    cachedGain = 1.0f;   // resumes at unity
                }
                else if (gap > 0.0)
                {
                    // SpinUp (always) or Snap-released-while-audible: run faster
                    // than the writer, capped, and never overshoot the live edge.
                    readInc = juce::jmin (gap, juce::jmax (readInc, kCatchUpRate));
                }
            }

            readPos += readInc;

            // Keep the reader within the ring: never let it fall further behind
            // than maxLag (bounds the buffer when Stop is held a long time).
            const double minReadPos = (double) (writeIndex - maxLag);
            if (readPos < minReadPos)
                readPos = minReadPos;

            // Once spun back up and caught up, hard-lock to unity so we sit
            // exactly on the write head (no fractional drift, bit-transparent).
            if (! engaged && phase <= 0.0 && (double) writeIndex - readPos <= 1.0)
                readPos = (double) writeIndex;

            // 4) Advance the control ramp, then map phase -> speed via the curve.
            advanceRamp();
        }

        // Publish the latest control state for the UI (read-only, lock-free).
        speedPublished.store ((float) speed, std::memory_order_relaxed);
        phasePublished.store ((float) phase, std::memory_order_relaxed);
    }

private:
    void advanceRamp() noexcept
    {
        const double target = engaged ? 1.0 : 0.0;

        if (phase < target)
        {
            const double inc = 1000.0 / (stopMs * sr);   // 0 -> 1 over stopMs
            phase = juce::jmin (target, phase + inc);
        }
        else if (phase > target)
        {
            const double inc = 1000.0 / (startMs * sr);  // 1 -> 0 over startMs
            phase = juce::jmax (target, phase - inc);
        }

        // speed (and the gain derived from it) is a pure function of (phase, curve).
        // Recompute the transcendental only when one of those actually changed —
        // steady running (phase=0) and steady stopped (phase=1) then skip it entirely.
        if (phase != speedPhase || curve != speedCurve)
        {
            const double exponent = 1.0 + (double) curve * kCurveExp;
            speed = std::pow (1.0 - phase, exponent);
            speedPhase = phase;
            speedCurve = curve;
            cachedGain = (speed >= kFadeStart) ? 1.0f : (float) (speed / kFadeStart);
        }
    }

    static constexpr double kCurveExp     = 4.0;     // curve=1 -> exponent 5
    static constexpr double kFadeStart    = 0.05;    // fade to silence below 5% speed
    static constexpr double kCatchUpRate  = 2.0;     // max read rate while returning to live
    static constexpr float  kSnapSilenceGain = 1.0e-3f; // "silent enough" to jump in Snap mode

    double    sr         = 44100.0;
    long long bufferSize = 0;
    long long mask       = 0;
    long long maxLag     = 0;

    std::vector<std::vector<float>> channels;

    long long writeIndex = 0;     // monotonic write counter
    double    readPos     = 0.0;  // fractional, shared across channels
    double    phase       = 0.0;  // 0 = running, 1 = fully stopped
    double    speed       = 1.0;  // playback rate (read increment per sample)

    // Cache for the speed/gain curve: recompute std::pow only when (phase, curve)
    // change, so steady running/stopped states pay nothing per sample. Updated
    // wherever speed is written (advanceRamp, the Snap branch, reset).
    double    speedPhase  = -1.0; // phase `speed` was last computed for (-1 = force)
    float     speedCurve  = -1.0f;// curve `speed` was last computed for
    float     cachedGain  = 1.0f; // gain = f(speed)

    bool       engaged    = false;
    ReturnMode returnMode = ReturnMode::Snap;
    float      stopMs     = 500.0f;
    float      startMs    = 300.0f;
    float      curve      = 0.5f;

    // UI-facing snapshots of speed/phase, published once per block. Atomic so the
    // editor's message-thread timer can read them while the audio thread writes.
    std::atomic<float> speedPublished { 1.0f };
    std::atomic<float> phasePublished { 0.0f };
};
