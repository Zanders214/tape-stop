#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ParamID
{
    static constexpr const char* stop      = "stop";
    static constexpr const char* stopTime  = "stopTime";
    static constexpr const char* startTime = "startTime";
    static constexpr const char* curve     = "curve";
}

TapeStopAudioProcessor::TapeStopAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    stopParam      = apvts.getRawParameterValue (ParamID::stop);
    stopTimeParam  = apvts.getRawParameterValue (ParamID::stopTime);
    startTimeParam = apvts.getRawParameterValue (ParamID::startTime);
    curveParam     = apvts.getRawParameterValue (ParamID::curve);
}

juce::AudioProcessorValueTreeState::ParameterLayout
TapeStopAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::stop, 1 }, "Stop", false));

    // Time ranges skewed toward the low end where the musically useful values live.
    auto timeRange = NormalisableRange<float> (1.0f, 5000.0f, 1.0f, 0.3f);

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::stopTime, 1 }, "Stop Time", timeRange, 500.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::startTime, 1 }, "Start Time", timeRange, 300.0f,
        AudioParameterFloatAttributes().withLabel ("ms")));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::curve, 1 }, "Curve",
        NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    return layout;
}

void TapeStopAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    engine.prepare (sampleRate, getTotalNumOutputChannels(), maxStopTimeSeconds);

    // Latency is time-varying and only non-zero while the effect is active, so
    // we report none and keep the at-rest state phase-transparent.
    setLatencySamples (0);
}

bool TapeStopAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono()
        && mainOut != juce::AudioChannelSet::stereo())
        return false;

    // Input must match output (in == out for an effect).
    return layouts.getMainInputChannelSet() == mainOut;
}

void TapeStopAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that don't have matching input.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setStopEngaged (stopParam->load() >= 0.5f);
    engine.setTimes (stopTimeParam->load(), startTimeParam->load());
    engine.setCurve (curveParam->load());

    engine.process (buffer);
}

juce::AudioProcessorEditor* TapeStopAudioProcessor::createEditor()
{
    return new TapeStopAudioProcessorEditor (*this);
}

void TapeStopAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TapeStopAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeStopAudioProcessor();
}
