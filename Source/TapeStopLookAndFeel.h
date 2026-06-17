#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

/**
    Shared visual language for the "Neon Cassette" editor.

    This header centralises the design tokens (colours), the spectrum / curve
    maths that several widgets share, the bundled-font loader, and a custom
    LookAndFeel for the thin gradient sliders. Everything here is plugin-side
    (it pulls in juce_gui_basics); the headless DSP tests never include it.
*/
namespace tape
{
    // --- Spectrum + accent colours (left -> right across the design) ----------
    inline const juce::Colour cyan   { 0xff34d8ff };
    inline const juce::Colour violet { 0xff8b7bff };
    inline const juce::Colour pink   { 0xffff5fa8 };
    inline const juce::Colour amber  { 0xffffc24b };

    inline const juce::Colour badgeBlue { 0xff5e93ff };
    inline const juce::Colour stopRedTop { 0xffff5a5a };
    inline const juce::Colour stopRedBot { 0xffe23b3b };

    // --- Text -----------------------------------------------------------------
    inline const juce::Colour textStrong { 0xffe8ecf3 };
    inline const juce::Colour textMid    { 0xffcfd4dc };
    inline const juce::Colour textDim    { 0xff9aa3b3 };
    inline const juce::Colour textDimmer { 0xff7e8794 };

    // --- Surfaces -------------------------------------------------------------
    inline const juce::Colour panelTop    { 0xff1a2030 };
    inline const juce::Colour panelBot     { 0xff0a0b12 };
    inline const juce::Colour cassetteTop  { 0xff20242d };
    inline const juce::Colour cassetteBot  { 0xff0f1217 };
    inline const juce::Colour tapeWindow   { 0xff070a0e };

    inline juce::Colour whiteAlpha (float a) { return juce::Colour::fromFloatRGBA (1.0f, 1.0f, 1.0f, a); }

    inline const juce::Colour hairline = whiteAlpha (0.06f); // 1px borders / dividers
    inline const juce::Colour track    = whiteAlpha (0.10f); // slider / bar tracks
    inline const juce::Colour barIdle  = whiteAlpha (0.08f); // unlit histogram bars

    // --- Shared maths (ported verbatim from the design's <script>) ------------

    /** Per-channel sRGB lerp, matching the prototype's mix2(). */
    inline juce::Colour mix2 (juce::Colour a, juce::Colour b, float t)
    {
        return a.interpolatedWith (b, juce::jlimit (0.0f, 1.0f, t));
    }

    /** Samples the 4-stop spectrum (cyan -> violet -> pink -> amber) at x in [0,1]. */
    inline juce::Colour spectrum (float x)
    {
        x = juce::jlimit (0.0f, 1.0f, x);
        if (x < 0.4f) return mix2 (cyan,   violet, x / 0.4f);
        if (x < 0.7f) return mix2 (violet, pink,  (x - 0.4f) / 0.3f);
        return                mix2 (pink,   amber, (x - 0.7f) / 0.3f);
    }

    /** Speed the tape would have at normalised wind-down position u, for the
        given curve: speedShape(u) = (1 - u) ^ (1 + curve * 4). */
    inline float speedShape (float u, float curve)
    {
        return std::pow (1.0f - juce::jlimit (0.0f, 1.0f, u), 1.0f + curve * 4.0f);
    }

    /** Curve read-out string: "LINEAR" / "EXP" / "exp N.N". */
    inline juce::String curveLabel (float curve)
    {
        if (curve < 0.12f) return "LINEAR";
        if (curve > 0.88f) return "EXP";
        return "exp " + juce::String (1.0f + curve * 4.0f, 1);
    }

    /** Slider time value -> "NNN ms" or "N.NN s" (>= 1000 ms), matching fms(). */
    inline juce::String formatMs (float ms)
    {
        if (ms >= 1000.0f) return juce::String (ms / 1000.0f, 2) + " s";
        return juce::String (juce::roundToInt (ms)) + " ms";
    }
}

/**
    Loads and vends the bundled fonts (Space Grotesk + JetBrains Mono). One
    instance is owned by the editor; widgets borrow it by reference. The
    constructor (which touches BinaryData) lives in the .cpp.
*/
struct TapeFonts
{
    TapeFonts();

    juce::Font sans (float height) const { return make (sansRegular, height); } // body labels
    juce::Font semi (float height) const { return make (sansSemi,    height); } // title / pill
    juce::Font bold (float height) const { return make (sansBold,    height); } // STOP button
    juce::Font mono (float height) const { return make (monoFace,    height); } // numeric read-outs

    juce::Typeface::Ptr sansRegular, sansSemi, sansBold, monoFace;

private:
    static juce::Font make (juce::Typeface::Ptr tf, float height)
    {
        if (tf == nullptr)
            return juce::Font (juce::FontOptions().withHeight (height));
        return juce::Font (juce::FontOptions().withTypeface (tf).withHeight (height));
    }
};

/**
    LookAndFeel for the three Stop Time / Start Time / Curve sliders: a thin
    rounded track, a gradient fill from the left to the thumb, and a round light
    thumb with a soft shadow. The per-slider fill gradient is supplied through
    two component properties (see gradStartId / gradEndId); all sizing derives
    from the slider's own height so it scales with the editor.
*/
class TapeStopLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TapeStopLookAndFeel() = default;

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    // Property keys: each slider stores its fill-gradient endpoints (ARGB) here.
    static const juce::Identifier gradStartId;
    static const juce::Identifier gradEndId;
};
