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

    // Return-mode selector. Item IDs (1, 2) map to the "Return" choice indices
    // (0 = Spin Up, 1 = Snap); the ComboBoxAttachment keeps them in sync.
    returnBox.addItem ("Spin Up", 1);
    returnBox.addItem ("Snap",    2);
    addAndMakeVisible (returnBox);

    returnLabel.setText ("Return", juce::dontSendNotification);
    returnLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (returnLabel);

    stopAttachment      = std::make_unique<APVTS::ButtonAttachment>   (processorRef.apvts, "stop",       stopButton);
    stopTimeAttachment  = std::make_unique<APVTS::SliderAttachment>   (processorRef.apvts, "stopTime",   stopTimeKnob);
    startTimeAttachment = std::make_unique<APVTS::SliderAttachment>   (processorRef.apvts, "startTime",  startTimeKnob);
    curveAttachment     = std::make_unique<APVTS::SliderAttachment>   (processorRef.apvts, "curve",      curveKnob);
    returnAttachment    = std::make_unique<APVTS::ComboBoxAttachment> (processorRef.apvts, "returnMode", returnBox);

    setSize (420, 380);
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

    // Return selector lives in a strip at the bottom; knobs fill what remains.
    auto returnRow = area.removeFromBottom (50);
    returnLabel.setBounds (returnRow.removeFromTop (20));
    returnBox.setBounds (returnRow.reduced (110, 2));

    area.removeFromBottom (10);

    // Three knobs in a row, each with its label above.
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
