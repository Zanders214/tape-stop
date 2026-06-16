#include "PluginEditor.h"

TapeStopAudioProcessorEditor::TapeStopAudioProcessorEditor (TapeStopAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    stopButton.setClickingTogglesState (true);
    stopButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::orangered);
    addAndMakeVisible (stopButton);

    setupKnob (stopTimeKnob,  stopTimeLabel,  "Stop Time");
    setupKnob (startTimeKnob, startTimeLabel, "Start Time");
    setupKnob (curveKnob,     curveLabel,     "Curve");

    stopAttachment      = std::make_unique<APVTS::ButtonAttachment> (processorRef.apvts, "stop",      stopButton);
    stopTimeAttachment  = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "stopTime",  stopTimeKnob);
    startTimeAttachment = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "startTime", startTimeKnob);
    curveAttachment     = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "curve",     curveKnob);

    setSize (420, 320);
}

void TapeStopAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label,
                                              const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void TapeStopAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1f));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("TAPE STOP", getLocalBounds().removeFromTop (36),
                juce::Justification::centred);
}

void TapeStopAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (28); // title

    stopButton.setBounds (area.removeFromTop (90).reduced (40, 8));

    area.removeFromTop (10);

    // Three knobs in a row, each with its label above.
    juce::FlexBox row;
    row.flexDirection = juce::FlexBox::Direction::row;
    row.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    struct KnobPair { juce::Label& label; juce::Slider& slider; };
    KnobPair pairs[] = {
        { stopTimeLabel,  stopTimeKnob  },
        { startTimeLabel, startTimeKnob },
        { curveLabel,     curveKnob     }
    };

    const int knobW = area.getWidth() / 3;
    for (auto& kp : pairs)
    {
        auto col = area.removeFromLeft (knobW);
        kp.label.setBounds (col.removeFromTop (20));
        kp.slider.setBounds (col);
    }
}
