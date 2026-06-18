#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "PluginProcessor.h"
#include "TapeStopLookAndFeel.h"

/**
    Full-width STOP button with the design's two states (idle "STOP" /
    engaged "STOPPING"). A real juce::Button so a ButtonAttachment keeps it in
    sync with the automatable `stop` parameter both ways. Self-scales from its
    own height (design height 48 px).
*/
class StopButton : public juce::Button
{
public:
    explicit StopButton (TapeFonts& f) : juce::Button ("Stop"), fonts (f)
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

private:
    TapeFonts& fonts;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StopButton)
};

/**
    Subtle two-segment Return toggle ("SPIN UP" | "SNAP") that fronts the
    `returnMode` choice parameter. Toggle-on == Snap (choice index 1),
    toggle-off == Spin Up (index 0); a ButtonAttachment maps the bool toggle to
    the parameter's 0/1 normalised values.
*/
class ReturnToggle : public juce::Button
{
public:
    explicit ReturnToggle (TapeFonts& f) : juce::Button ("Return"), fonts (f)
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics&, bool shouldDrawHighlighted, bool shouldDrawDown) override;

private:
    TapeFonts& fonts;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReturnToggle)
};

/**
    "Neon Cassette" editor. Custom-painted: a dark panel with a spinning-hub
    cassette, a spectrum tape-speed bar, a STOP button, a slowdown-curve
    histogram with a live playhead, and three gradient sliders. A 60 Hz timer
    polls the engine's speed/phase and advances the hub rotation.
*/
class TapeStopAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit TapeStopAudioProcessorEditor (TapeStopAudioProcessor&);
    ~TapeStopAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // The design's 1.0 scale. Width drives the uniform scale; height follows.
    static constexpr float designWidth  = 360.0f;
    static constexpr float designHeight = 704.0f;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    // All widget rectangles for one frame, in pixels, derived from the width.
    struct Layout
    {
        float s = 1.0f;
        juce::Rectangle<float> panel;
        juce::Rectangle<float> title;
        juce::Rectangle<float> pill;
        juce::Rectangle<float> cassette;
        juce::Rectangle<float> labelStrip;
        juce::Rectangle<float> spectrumBar;
        juce::Rectangle<float> tapeWindow;
        std::array<juce::Point<float>, 2> hub; // [0] = left reel, [1] = right reel
        float hubRadius = 0.0f;
        juce::Rectangle<float> tapeBarLabel;
        juce::Rectangle<float> tapeBarTrack;
        juce::Rectangle<float> stopButton;
        juce::Rectangle<float> returnLabel;
        juce::Rectangle<float> returnToggle;
        juce::Rectangle<float> curveLabel;
        juce::Rectangle<float> histogram;
        juce::Rectangle<float> divider;
        std::array<juce::Rectangle<float>, 3> sliderLabel;
        std::array<juce::Rectangle<float>, 3> sliderTrack;
    };

    Layout computeLayout (float widthPx) const;

    void timerCallback() override;

    // Paint helpers (all take the live frame state). const: they only read
    // member state (fonts, hub angle, slider values) and draw via the context.
    void drawHeader     (juce::Graphics&, const Layout&) const;
    void drawCassette   (juce::Graphics&, const Layout&, float speed, float phase) const;
    void drawHub        (juce::Graphics&, juce::Point<float> centre, float radius,
                         juce::Colour capA, juce::Colour capB, float angleDeg) const;
    void drawTapeBar    (juce::Graphics&, const Layout&, float speed) const;
    void drawHistogram  (juce::Graphics&, const Layout&, float curve, float phase) const;
    void drawSliderLabels (juce::Graphics&, const Layout&) const;

    // Tracked (letter-spaced) text via JUCE's extra-kerning factor.
    void drawLabel (juce::Graphics&, const juce::String&, juce::Font, juce::Colour,
                    juce::Rectangle<float>, juce::Justification, float trackingEm = 0.0f) const;

    TapeStopAudioProcessor& processorRef;

    TapeFonts            fonts;
    TapeStopLookAndFeel  lnf;

    StopButton   stopButton  { fonts };
    ReturnToggle returnToggle { fonts };
    juce::Slider stopTimeSlider;
    juce::Slider startTimeSlider;
    juce::Slider curveSlider;

    std::unique_ptr<APVTS::ButtonAttachment> stopAttachment;
    std::unique_ptr<APVTS::ButtonAttachment> returnAttachment;
    std::unique_ptr<APVTS::SliderAttachment> stopTimeAttachment;
    std::unique_ptr<APVTS::SliderAttachment> startTimeAttachment;
    std::unique_ptr<APVTS::SliderAttachment> curveAttachment;

    // Cached panel background gradient (the one full-area fill); rebuilt on resize.
    juce::Image background;

    // Animation state.
    double hubAngleDeg = 0.0;
    double lastTickMs  = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeStopAudioProcessorEditor)
};
