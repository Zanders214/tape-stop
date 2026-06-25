// nanobench micro-benchmark of the real audio callback.
//
// Times processBlock at 48k/512 and emits a github-action-benchmark
// "customSmallerIsBetter" JSON consumed by .github/workflows/perf.yml, which
// tracks the trend and comments on regressions. Built only when
// TAPESTOP_BUILD_BENCH=ON.
//
// Measured in three tiers so the dashboard can attribute a change to the state
// it affects:
//   processBlock            - Stop held (mostly fully-stopped); the original metric.
//   processBlock (running)  - transparent passthrough (phase=0); most common real-world state.
//   processBlock (ramping)  - mid spin-down (phase moving); guard metric, should stay flat.
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

    auto* stopParam = proc.apvts.getParameter ("stop");

    // Drive `blocks` blocks of audio to settle the engine into a target state.
    double ph = 1.0;
    auto pump = [&] (int blocks)
    {
        for (int i = 0; i < blocks; ++i)
        {
            fillSine (ph); ph += 1.0;
            juce::MidiBuffer m;
            proc.processBlock (buffer, m);
        }
    };
    // The timed region is ONLY processBlock — the input is filled once before each
    // run (below), not inside the loop, so we measure the DSP and not sine
    // generation. processBlock's cost is independent of the sample values, and the
    // in-place buffer is kept live via doNotOptimizeAway so the call isn't elided.
    juce::MidiBuffer emptyMidi;
    auto timedBlock = [&]
    {
        proc.processBlock (buffer, emptyMidi);
        ankerl::nanobench::doNotOptimizeAway (buffer.getReadPointer (0)[0]);
    };

    ankerl::nanobench::Bench bench;
    bench.title ("DSP @48k/512").unit ("block").warmup (100).minEpochIterations (2000);

    // --- Tier 1: Stop held (original metric; default 500 ms stopTime) ---------
    if (stopParam != nullptr) stopParam->setValueNotifyingHost (1.0f);
    pump (1);                                   // prime: ramp already underway
    fillSine (0.0);
    bench.run ("processBlock", timedBlock);

    // --- Tier 2: running / transparent passthrough (phase settles to 0) -------
    // The most common real-world state, and the one that benefits most from
    // caching the per-sample std::pow. (The transient ramp path is left out:
    // nanobench's multi-epoch run far outlasts the spin-down, so it can't be held
    // mid-ramp; that path is exercised by the pitch-drop unit test instead.)
    if (stopParam != nullptr) stopParam->setValueNotifyingHost (0.0f);
    pump (8);                                   // settle to unity, reader locked to write head
    fillSine (0.0);
    bench.run ("processBlock (running)", timedBlock);

    // GOTCHA: the Measure enum is nested in Result, not the namespace.
    using ankerl::nanobench::Result;
    const auto& results   = bench.results();
    const double ns       = results[0].median (Result::Measure::elapsed) * 1.0e9;   // Stop-held (headline)
    const double nsRun    = results[1].median (Result::Measure::elapsed) * 1.0e9;
    const double budgetNs = (double) bs / sr * 1.0e9;     // 10.67 ms @48k/512
    const double loadPct  = ns / budgetNs * 100.0;

    // github-action-benchmark "customSmallerIsBetter" schema. The first two
    // entries are kept stable so the existing trend series are uninterrupted.
    juce::String json;
    json << "[\n"
         << "  { \"name\": \"processBlock\",           \"unit\": \"ns/block\", \"value\": " << juce::String (ns, 3)      << " },\n"
         << "  { \"name\": \"DSP load @48k/512\",      \"unit\": \"%\",        \"value\": " << juce::String (loadPct, 3) << " },\n"
         << "  { \"name\": \"processBlock (running)\", \"unit\": \"ns/block\", \"value\": " << juce::String (nsRun, 3)   << " }\n"
         << "]\n";
    const juce::String out = (argc > 1) ? juce::String (argv[1]) : juce::String ("bench_result.json");
    juce::File::getCurrentWorkingDirectory().getChildFile (out).replaceWithText (json);

    std::printf ("processBlock: stopped=%.0f ns (%.1f%% RT load) | running=%.0f ns\n",
                 ns, loadPct, nsRun);
    return 0;
}
