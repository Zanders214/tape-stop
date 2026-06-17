#include "TapeStopLookAndFeel.h"
#include "BinaryData.h"

// ---------------------------------------------------------------------------
// Fonts: load the four bundled typefaces from BinaryData once.
// ---------------------------------------------------------------------------
TapeFonts::TapeFonts()
{
    sansRegular = juce::Typeface::createSystemTypefaceFor (
        BinaryData::SpaceGroteskRegular_ttf,  BinaryData::SpaceGroteskRegular_ttfSize);
    sansSemi    = juce::Typeface::createSystemTypefaceFor (
        BinaryData::SpaceGroteskSemiBold_ttf, BinaryData::SpaceGroteskSemiBold_ttfSize);
    sansBold    = juce::Typeface::createSystemTypefaceFor (
        BinaryData::SpaceGroteskBold_ttf,     BinaryData::SpaceGroteskBold_ttfSize);
    monoFace    = juce::Typeface::createSystemTypefaceFor (
        BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
}

// ---------------------------------------------------------------------------
// Sliders.
// ---------------------------------------------------------------------------
const juce::Identifier TapeStopLookAndFeel::gradStartId { "tapeGradStart" };
const juce::Identifier TapeStopLookAndFeel::gradEndId   { "tapeGradEnd" };

void TapeStopLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/,
                                            juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

    // All metrics derive from the row height (design: 18px row, 4px track, 13px thumb).
    const float trackH = juce::jmax (2.0f, bounds.getHeight() * (4.0f  / 18.0f));
    const float thumbD = juce::jmax (6.0f, bounds.getHeight() * (13.0f / 18.0f));
    const float cy     = bounds.getCentreY();

    // Position from the slider's own value, so the NormalisableRange skew is honoured
    // and the thumb centre travels edge-to-edge (matching the prototype's translateX(-50%)).
    const float prop = (float) slider.valueToProportionOfLength (slider.getValue());
    const float fillRight = bounds.getX() + prop * bounds.getWidth();

    // Track.
    auto trackRect = juce::Rectangle<float> (bounds.getX(), cy - trackH * 0.5f,
                                             bounds.getWidth(), trackH);
    g.setColour (tape::track);
    g.fillRoundedRectangle (trackRect, trackH * 0.5f);

    // Gradient fill (left -> thumb). Endpoints come from per-slider properties.
    const auto props    = slider.getProperties();
    const auto gradFrom = juce::Colour ((juce::uint32) (juce::int64) props.getWithDefault (gradStartId, (juce::int64) tape::cyan.getARGB()));
    const auto gradTo   = juce::Colour ((juce::uint32) (juce::int64) props.getWithDefault (gradEndId,   (juce::int64) tape::violet.getARGB()));

    if (fillRight > bounds.getX() + 0.5f)
    {
        auto fillRect = trackRect.withWidth (fillRight - bounds.getX());
        // Gradient spans the fill itself (as in the prototype's gradient-on-fill),
        // so the full sweep shows regardless of how far the thumb has travelled.
        juce::ColourGradient grad (gradFrom, bounds.getX(), cy, gradTo, fillRight, cy, false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fillRect, trackH * 0.5f);
    }

    // Thumb: soft shadow then a light round cap.
    const float tx = juce::jlimit (bounds.getX(), bounds.getRight(), fillRight);
    auto thumb = juce::Rectangle<float> (thumbD, thumbD).withCentre ({ tx, cy });

    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (thumb.translated (0.0f, thumbD * 0.08f).expanded (thumbD * 0.06f));

    g.setColour (tape::textStrong);
    g.fillEllipse (thumb);
}
