// nanobench micro-benchmark of the real audio callback.
//
// Times processBlock at 48k/512 and emits a github-action-benchmark
// "customSmallerIsBetter" JSON consumed by .github/workflows/perf.yml, which
// tracks the trend and comments on regressions. Built only when
// TAPESTOP_BUILD_BENCH=ON.
#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000.0;
    const int    bs = 512;

    TapeStopAudioProcessor proc;
    proc.prepareToPlay (sr, bs);
    juce::AudioBuffer<float> buffer (2, bs);

    const double inc = 2.0 * juce::MathConstants<double>::pi * 220.0 / sr;
    auto fillSine = [&] (double startPhase)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            double p = startPhase;
            for (int s = 0; s < bs; ++s) { d[s] = (float) std::sin (p); p += inc; }
        }
    };

    // Engage Stop so we measure the active (worst-ish case) DSP path, and prime
    // one block so the slow-down ramp is already underway when timing starts.
    if (auto* stop = proc.apvts.getParameter ("stop"))
        stop->setValueNotifyingHost (1.0f);
    { juce::MidiBuffer m; fillSine (0.0); proc.processBlock (buffer, m); }

    double ph = 1.0;
    ankerl::nanobench::Bench bench;
    bench.title ("DSP @48k/512").unit ("block").warmup (20).minEpochIterations (200);
    bench.run ("processBlock", [&]
    {
        fillSine (ph);
        ph += 1.0;
        juce::MidiBuffer m;                       // empty: Stop is already held
        proc.processBlock (buffer, m);
        ankerl::nanobench::doNotOptimizeAway (buffer.getReadPointer (0)[0]);
    });

    // GOTCHA: the Measure enum is nested in Result, not the namespace.
    const double ns       = bench.results().back().median (ankerl::nanobench::Result::Measure::elapsed) * 1.0e9;
    const double budgetNs = (double) bs / sr * 1.0e9;     // 10.67 ms @48k/512
    const double loadPct  = ns / budgetNs * 100.0;

    // github-action-benchmark "customSmallerIsBetter" schema.
    juce::String json;
    json << "[\n"
         << "  { \"name\": \"processBlock\",      \"unit\": \"ns/block\", \"value\": " << juce::String (ns, 3)      << " },\n"
         << "  { \"name\": \"DSP load @48k/512\", \"unit\": \"%\",        \"value\": " << juce::String (loadPct, 3) << " }\n"
         << "]\n";
    const juce::String out = (argc > 1) ? juce::String (argv[1]) : juce::String ("bench_result.json");
    juce::File::getCurrentWorkingDirectory().getChildFile (out).replaceWithText (json);

    std::printf ("processBlock=%.0f ns (%.1f%% RT load)\n", ns, loadPct);
    return 0;
}
