// RealtimeSanitizer driver for the audio-thread DSP.
//
// Runs ~2 s of processBlock callbacks through TapeStopEngine::process (marked
// [[clang::nonblocking]] via TAPESTOP_RT_NONBLOCKING). Under clang's
// -fsanitize=realtime, any malloc / lock / syscall reached from that annotated
// path aborts with a diagnostic and fails the build. Built only when
// TAPESTOP_RT_SANITIZE=ON (needs clang >= 20); see .github/workflows/quality.yml.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    TapeStopAudioProcessor proc;
    const double sr = 48000.0;
    const int    bs = 512;
    proc.prepareToPlay (sr, bs);

    juce::AudioBuffer<float> buffer (2, bs);
    juce::MidiBuffer midi;                       // unused by this FX; processBlock takes one
    auto* stop = proc.apvts.getParameter ("stop");

    const int    blocks = (int) (sr / bs) * 2;   // ~2 s of callbacks
    const double inc    = 2.0 * juce::MathConstants<double>::pi * 220.0 / sr;
    double phase = 0.0;

    for (int b = 0; b < blocks; ++b)
    {
        // Feed real signal so the variable-rate resampler actually does work.
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            double p = phase;
            for (int s = 0; s < bs; ++s) { d[s] = (float) std::sin (p); p += inc; }
        }
        phase += inc * bs;

        // Engage Stop for the first half, release for the second, so the run
        // covers both the spin-down ramp and the return-to-live resync path.
        // (This toggle is off the annotated path, so it isn't part of the gate.)
        if (stop != nullptr)
            stop->setValueNotifyingHost (b < blocks / 2 ? 1.0f : 0.0f);

        proc.processBlock (buffer, midi);        // -> engine.process(), the annotated DSP
    }

    std::printf ("PASS: %d processBlock calls, no RT-safety violations\n", blocks);
    return 0;
}
