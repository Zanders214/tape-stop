/*
    Unit tests for TapeStopEngine.

    Headless and framework-free (plain asserts), so they run identically on
    Windows, macOS, and Linux CI without a display. Only juce_audio_basics is
    needed (for juce::AudioBuffer); no GUI / audio-device modules.

    Exit code 0 = all passed, non-zero = failure (picked up by CTest).
*/

#include "TapeStopEngine.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <cstdio>
#include <string>

namespace
{
int failures = 0;

void check (bool condition, const std::string& what)
{
    if (condition)
    {
        std::printf ("  [pass] %s\n", what.c_str());
    }
    else
    {
        std::printf ("  [FAIL] %s\n", what.c_str());
        ++failures;
    }
}

// Fills a single-channel buffer with a sine at the given frequency.
juce::AudioBuffer<float> makeSine (double sampleRate, double freq, int numSamples)
{
    juce::AudioBuffer<float> buf (1, numSamples);
    auto* d = buf.getWritePointer (0);
    for (int i = 0; i < numSamples; ++i)
        d[i] = std::sin (juce::MathConstants<double>::twoPi * freq * i / sampleRate);
    return buf;
}

int countZeroCrossings (const float* d, int start, int len)
{
    int crossings = 0;
    for (int i = start + 1; i < start + len; ++i)
        if ((d[i - 1] <= 0.0f) != (d[i] <= 0.0f))
            ++crossings;
    return crossings;
}

// ---- Test 1: with Stop never engaged, the effect is bit-transparent. -------
void testUnityPassthrough()
{
    std::printf ("Unity passthrough:\n");
    constexpr double sr = 48000.0;
    const int n = 4096;

    TapeStopEngine engine;
    engine.prepare (sr, 1, 5.0);
    engine.setStopEngaged (false);

    auto in  = makeSine (sr, 440.0, n);
    auto buf = in; // copy; processed in place

    engine.process (buf);

    float maxDiff = 0.0f;
    for (int i = 0; i < n; ++i)
        maxDiff = juce::jmax (maxDiff, std::abs (buf.getSample (0, i) - in.getSample (0, i)));

    check (maxDiff < 1.0e-6f, "output equals input when not engaged (maxDiff "
                              + std::to_string (maxDiff) + ")");
}

// ---- Test 2: holding Stop winds down to silence. --------------------------
void testStopReachesSilence()
{
    std::printf ("Stop reaches silence:\n");
    constexpr double sr = 48000.0;
    const float stopMs = 200.0f;

    TapeStopEngine engine;
    engine.prepare (sr, 1, 5.0);
    engine.setTimes (stopMs, 100.0f);
    engine.setStopEngaged (true);

    // Process well past the spin-down time so the tape is fully stopped.
    const int n = (int) (sr * (stopMs / 1000.0) * 3.0);
    auto buf = makeSine (sr, 440.0, n);
    engine.process (buf);

    // Measure RMS of the final 10% of the block (should be ~silent).
    const int tailStart = n - n / 10;
    double sumSq = 0.0;
    for (int i = tailStart; i < n; ++i)
        sumSq += (double) buf.getSample (0, i) * buf.getSample (0, i);
    const double rms = std::sqrt (sumSq / (n - tailStart));

    check (rms < 1.0e-4, "fully-stopped tail is silent (rms " + std::to_string (rms) + ")");
}

// ---- Test 3: output stays finite and bounded across a stop/start cycle. ----
void testFiniteAndBounded()
{
    std::printf ("Finite and bounded:\n");
    constexpr double sr = 44100.0;

    TapeStopEngine engine;
    engine.prepare (sr, 2, 5.0);
    engine.setTimes (300.0f, 250.0f);

    bool allFinite = true, allBounded = true;

    auto runSeconds = [&] (double seconds)
    {
        const int n = (int) (sr * seconds);
        juce::AudioBuffer<float> buf (2, n);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
                d[i] = 0.9f * std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / sr);
        }
        engine.process (buf);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
            {
                const float v = buf.getSample (ch, i);
                allFinite  &= std::isfinite (v);
                allBounded &= (std::abs (v) <= 1.5f);
            }
    };

    engine.setStopEngaged (false); runSeconds (0.3); // running
    engine.setStopEngaged (true);  runSeconds (0.6); // wind down + held
    engine.setStopEngaged (false); runSeconds (0.6); // spin back up

    check (allFinite,  "no NaN/Inf anywhere in a stop/start cycle");
    check (allBounded, "output magnitude stays within 1.5");
}

// ---- Test 4: during spin-down the pitch drops (fewer zero crossings). ------
void testPitchDrops()
{
    std::printf ("Pitch drops during spin-down:\n");
    constexpr double sr = 48000.0;
    const float stopMs = 2000.0f; // long ramp so mid-ramp speed is clearly < 1

    const int n = (int) (sr * 1.5); // 1.5 s
    auto buf = makeSine (sr, 1000.0, n);

    TapeStopEngine engine;
    engine.prepare (sr, 1, 5.0);
    engine.setTimes (stopMs, 100.0f);
    engine.setCurve (0.5f);

    // First window: still at unity (not engaged yet) -> baseline pitch.
    engine.setStopEngaged (false);
    const int win = 4000;
    juce::AudioBuffer<float> unityBlock (1, win);
    unityBlock.copyFrom (0, 0, buf, 0, 0, win);
    engine.process (unityBlock);
    const int unityCrossings = countZeroCrossings (unityBlock.getReadPointer (0), 0, win);

    // Now engage and process a chunk; sample a window partway into the ramp
    // where speed is well below 1 but signal is still audible.
    engine.setStopEngaged (true);
    juce::AudioBuffer<float> downBlock (1, n);
    downBlock.copyFrom (0, 0, buf, 0, 0, n);
    engine.process (downBlock);

    const int sampleAt = (int) (sr * 0.5); // ~quarter into the 2 s ramp
    const int slowCrossings = countZeroCrossings (downBlock.getReadPointer (0), sampleAt, win);

    check (slowCrossings < unityCrossings,
           "fewer zero crossings while slowing (slow " + std::to_string (slowCrossings)
               + " < unity " + std::to_string (unityCrossings) + ")");
}
} // namespace

int main()
{
    std::printf ("TapeStopEngine unit tests\n=========================\n");

    testUnityPassthrough();
    testStopReachesSilence();
    testFiniteAndBounded();
    testPitchDrops();

    std::printf ("=========================\n%s\n",
                 failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED");
    return failures == 0 ? 0 : 1;
}
