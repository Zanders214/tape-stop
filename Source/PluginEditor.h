#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

/**
    Minimal functional editor: a large Stop toggle plus three labelled rotary
    knobs (Stop Time, Start Time, Curve), all bound to the APVTS so they stay in
    sync with host automation.
*/
class TapeStopAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TapeStopAudioProcessorEditor (TapeStopAudioProcessor&);
    ~TapeStopAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    // Helper to configure a rotary slider + its label in one place.
    void setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& text);

    TapeStopAudioProcessor& processorRef;

    juce::TextButton stopButton { "STOP" };

    juce::Slider stopTimeKnob, startTimeKnob, curveKnob;
    juce::Label  stopTimeLabel, startTimeLabel, curveLabel;

    std::unique_ptr<APVTS::ButtonAttachment> stopAttachment;
    std::unique_ptr<APVTS::SliderAttachment> stopTimeAttachment;
    std::unique_ptr<APVTS::SliderAttachment> startTimeAttachment;
    std::unique_ptr<APVTS::SliderAttachment> curveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeStopAudioProcessorEditor)
};
