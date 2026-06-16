#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "TapeStopEngine.h"

/**
    TapeStopAudioProcessor

    Thin host wrapper around TapeStopEngine. Owns the parameter tree (APVTS) and
    feeds the engine each block; the real DSP lives in TapeStopEngine.h.
*/
class TapeStopAudioProcessor : public juce::AudioProcessor
{
public:
    TapeStopAudioProcessor();
    ~TapeStopAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter tree, public so the editor can attach to it.
    juce::AudioProcessorValueTreeState apvts;

    // The maximum spin-down time, in seconds. Also bounds the ring-buffer size.
    static constexpr double maxStopTimeSeconds = 5.0;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    TapeStopEngine engine;

    std::atomic<float>* stopParam       = nullptr;
    std::atomic<float>* stopTimeParam   = nullptr;
    std::atomic<float>* startTimeParam  = nullptr;
    std::atomic<float>* curveParam      = nullptr;
    std::atomic<float>* returnModeParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeStopAudioProcessor)
};
