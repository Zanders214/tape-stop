#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Width of a (possibly kerned) string, via GlyphArrangement — the JUCE 8
    // replacement for the deprecated Font::getStringWidthFloat.
    float textWidth (const juce::Font& font, const juce::String& text)
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (font, text, 0.0f, 0.0f);
        return ga.getBoundingBox (0, -1, true).getWidth();
    }

    // Horizontal 4-stop spectrum gradient (cyan -> violet -> pink -> amber)
    // spanning a rectangle's width.
    juce::ColourGradient spectrumGradient (juce::Rectangle<float> r)
    {
        juce::ColourGradient g (tape::cyan, r.getX(), r.getCentreY(),
                                tape::amber, r.getRight(), r.getCentreY(), false);
        g.addColour (1.0 / 3.0, tape::violet);
        g.addColour (2.0 / 3.0, tape::pink);
        return g;
    }

    // Rounded only on the top two corners (histogram bars).
    void fillTopRounded (juce::Graphics& g, juce::Rectangle<float> r, float corner)
    {
        corner = juce::jlimit (0.0f, juce::jmin (r.getWidth(), r.getHeight()) * 0.5f, corner);
        juce::Path p;
        p.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                               corner, corner, true, true, false, false);
        g.fillPath (p);
    }
}

// ===========================================================================
// Custom buttons
// ===========================================================================
void StopButton::paintButton (juce::Graphics& g, bool, bool shouldDrawDown)
{
    auto b = getLocalBounds().toFloat();
    const float s      = b.getHeight() / 48.0f;
    const float radius = 12.0f * s;
    const bool  on     = getToggleState();

    // Engaged: soft red halo behind the button (0 0 22px glow, faked with layers).
    if (on)
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (juce::Colour (0xffff5050).withAlpha (0.10f));
            g.fillRoundedRectangle (b.expanded ((float) i * 2.0f * s), radius + (float) i * 2.0f * s);
        }

    // Body.
    if (on)
        g.setGradientFill (juce::ColourGradient (tape::stopRedTop, b.getCentreX(), b.getY(),
                                                 tape::stopRedBot, b.getCentreX(), b.getBottom(), false));
    else
        g.setColour (tape::whiteAlpha (0.06f));
    g.fillRoundedRectangle (b, radius);

    if (shouldDrawDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (b, radius);
    }

    // Border.
    g.setColour (on ? juce::Colour::fromFloatRGBA (1.0f, 0.47f, 0.47f, 0.6f) : tape::whiteAlpha (0.12f));
    g.drawRoundedRectangle (b.reduced (0.5f), radius, juce::jmax (1.0f, s));

    // Inset top highlight (box-shadow inset 0 1px 0 rgba(255,255,255, .3/.06)).
    {
        const float t = juce::jmax (1.0f, s);
        g.setColour (tape::whiteAlpha (on ? 0.30f : 0.06f));
        g.fillRect (juce::Rectangle<float> (b.getX() + radius, b.getY() + t, b.getWidth() - 2.0f * radius, t));
    }

    // Centred [dot] [gap] [label] block.
    const juce::Colour fg    = on ? juce::Colours::white : tape::textStrong;
    const juce::String label = on ? "STOPPING" : "STOP";
    auto font = fonts.bold (14.0f * s).withExtraKerningFactor (0.16f);
    const float textW = textWidth (font, label);

    const float dot = 9.0f * s, gap = 9.0f * s;
    const float x0  = b.getCentreX() - (dot + gap + textW) * 0.5f;
    const float cy  = b.getCentreY();

    auto dotRect = juce::Rectangle<float> (x0, cy - dot * 0.5f, dot, dot);
    const juce::Colour glow = on ? juce::Colours::white : juce::Colour (0xffff5050);
    g.setColour (glow.withAlpha (0.5f));
    g.fillRoundedRectangle (dotRect.expanded (2.0f * s), 3.0f * s);
    g.setColour (fg);
    g.fillRoundedRectangle (dotRect, 2.0f * s);

    g.setFont (font);
    g.drawText (label, juce::Rectangle<float> (x0 + dot + gap, b.getY(), textW + 4.0f * s, b.getHeight()).toNearestInt(),
                juce::Justification::centredLeft, false);
}

void ReturnToggle::paintButton (juce::Graphics& g, bool, bool)
{
    auto b = getLocalBounds().toFloat();
    const float radius = b.getHeight() * 0.5f;
    const bool  snap   = getToggleState(); // true == Snap

    g.setColour (tape::whiteAlpha (0.05f));
    g.fillRoundedRectangle (b, radius);
    g.setColour (tape::hairline);
    g.drawRoundedRectangle (b.reduced (0.5f), radius, 1.0f);

    auto left   = b.withWidth (b.getWidth() * 0.5f);
    auto right  = left.translated (b.getWidth() * 0.5f, 0.0f);
    auto active = snap ? right : left;

    g.setColour (tape::badgeBlue.withAlpha (0.16f));
    g.fillRoundedRectangle (active.reduced (2.0f), radius - 2.0f);

    auto font = fonts.semi (b.getHeight() * 0.42f).withExtraKerningFactor (0.08f);
    g.setFont (font);
    g.setColour (snap ? tape::textDimmer : tape::badgeBlue);
    g.drawText ("SPIN UP", left.toNearestInt(),  juce::Justification::centred, false);
    g.setColour (snap ? tape::badgeBlue : tape::textDimmer);
    g.drawText ("SNAP",    right.toNearestInt(), juce::Justification::centred, false);
}

// ===========================================================================
// Editor
// ===========================================================================
TapeStopAudioProcessorEditor::TapeStopAudioProcessorEditor (TapeStopAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    auto setupSlider = [this] (juce::Slider& sl, juce::Colour from, juce::Colour to)
    {
        sl.setSliderStyle (juce::Slider::LinearHorizontal);
        sl.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        sl.getProperties().set (TapeStopLookAndFeel::gradStartId, (juce::int64) from.getARGB());
        sl.getProperties().set (TapeStopLookAndFeel::gradEndId,   (juce::int64) to.getARGB());
        sl.setLookAndFeel (&lnf);
        addAndMakeVisible (sl);
    };

    setupSlider (stopTimeSlider,  tape::cyan, tape::violet);
    setupSlider (startTimeSlider, tape::cyan, tape::violet);
    setupSlider (curveSlider,     tape::pink, tape::amber);

    addAndMakeVisible (stopButton);
    addAndMakeVisible (returnToggle);

    stopAttachment      = std::make_unique<APVTS::ButtonAttachment> (processorRef.apvts, "stop",       stopButton);
    returnAttachment    = std::make_unique<APVTS::ButtonAttachment> (processorRef.apvts, "returnMode", returnToggle);
    stopTimeAttachment  = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "stopTime",   stopTimeSlider);
    startTimeAttachment = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "startTime",  startTimeSlider);
    curveAttachment     = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "curve",      curveSlider);

    setResizable (true, false);
    setResizeLimits ((int) (designWidth  * 0.85f), (int) (designHeight * 0.85f),
                     (int) (designWidth  * 1.6f),  (int) (designHeight * 1.6f));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) designWidth / (double) designHeight);

    setSize ((int) designWidth, (int) designHeight);

    lastTickMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

TapeStopAudioProcessorEditor::~TapeStopAudioProcessorEditor()
{
    stopTimer();
    stopTimeSlider.setLookAndFeel (nullptr);
    startTimeSlider.setLookAndFeel (nullptr);
    curveSlider.setLookAndFeel (nullptr);
}

void TapeStopAudioProcessorEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    double dt = (now - lastTickMs) / 1000.0;
    lastTickMs = now;
    dt = juce::jlimit (0.0, 0.05, dt);

    const float speed = juce::jlimit (0.0f, 1.0f, processorRef.getSpeed());
    hubAngleDeg = std::fmod (hubAngleDeg + dt * 320.0 * speed, 360.0);

    repaint();
}

// --- Layout ----------------------------------------------------------------
TapeStopAudioProcessorEditor::Layout
TapeStopAudioProcessorEditor::computeLayout (float widthPx) const
{
    Layout L;
    const float s = widthPx / designWidth;
    L.s = s;
    auto S = [s] (float v) { return v * s; };

    const float W   = widthPx;
    const float pad = S (26.0f);
    const float cx0 = pad;
    const float cw  = W - 2.0f * pad;
    float y = pad;

    L.panel = { 0.0f, 0.0f, W, designHeight * s };

    // Header: title left, WIND-DOWN pill right (pill auto-sized to its text).
    L.title = { cx0, y, cw, S (22.0f) };
    {
        auto pf = fonts.semi (10.0f * s).withExtraKerningFactor (0.14f);
        const float pillW = textWidth (pf, "WIND-DOWN") + S (22.0f);
        const float pillH = S (18.0f);
        L.pill = { cx0 + cw - pillW, y + (S (22.0f) - pillH) * 0.5f, pillW, pillH };
    }
    y += S (22.0f) + S (14.0f);

    // Cassette + interior.
    L.cassette = { cx0, y, cw, S (188.0f) };
    {
        const float innerL = L.cassette.getX() + S (16.0f);
        const float innerT = L.cassette.getY() + S (14.0f);
        const float innerW = cw - 2.0f * S (16.0f);

        L.labelStrip  = { innerL + S (6.0f), innerT, innerW - 2.0f * S (6.0f), S (30.0f) };
        L.spectrumBar = { L.labelStrip.getCentreX() - S (45.0f), L.labelStrip.getCentreY() - S (2.0f), S (90.0f), S (4.0f) };

        const float winY = L.labelStrip.getBottom() + S (14.0f);
        L.tapeWindow = { innerL + S (6.0f), winY, innerW - 2.0f * S (6.0f), S (96.0f) };

        L.hubRadius = S (41.0f);
        L.hubLeft  = { L.tapeWindow.getX()     + S (66.0f), L.tapeWindow.getCentreY() };
        L.hubRight = { L.tapeWindow.getRight()  - S (66.0f), L.tapeWindow.getCentreY() };
    }
    y += S (188.0f) + S (18.0f);

    // Tape-speed bar.
    L.tapeBarLabel = { cx0, y, cw, S (13.0f) };
    L.tapeBarTrack = { cx0, y + S (13.0f) + S (8.0f), cw, S (6.0f) };
    y += S (13.0f) + S (8.0f) + S (6.0f) + S (18.0f);

    // STOP button.
    L.stopButton = { cx0, y, cw, S (48.0f) };
    y += S (48.0f) + S (10.0f);

    // Return row: "RETURN" left, segmented toggle right.
    {
        const float togW = S (132.0f), togH = S (20.0f);
        L.returnToggle = { cx0 + cw - togW, y, togW, togH };
        L.returnLabel  = { cx0, y, cw - togW - S (8.0f), togH };
    }
    y += S (20.0f) + S (18.0f);

    // Slowdown-curve graph.
    L.curveLabel = { cx0, y, cw, S (13.0f) };
    L.histogram  = { cx0, y + S (13.0f) + S (9.0f), cw, S (64.0f) };
    y += S (13.0f) + S (9.0f) + S (64.0f) + S (18.0f);

    // Divider + three sliders.
    L.divider = { cx0, y, cw, juce::jmax (1.0f, s) };
    y += S (18.0f);
    for (int i = 0; i < 3; ++i)
    {
        // The interactive row is 18 px tall (the LookAndFeel centres the 4 px
        // track + 13 px thumb within it); 8 px label gap, 15 px between rows.
        L.sliderLabel[i] = { cx0, y, cw, S (13.0f) };
        L.sliderTrack[i] = { cx0, y + S (13.0f) + S (8.0f), cw, S (18.0f) };
        y += S (13.0f) + S (8.0f) + S (18.0f);
        if (i < 2) y += S (15.0f);
    }

    return L;
}

void TapeStopAudioProcessorEditor::resized()
{
    const auto L = computeLayout ((float) getWidth());

    stopButton.setBounds   (L.stopButton.toNearestInt());
    returnToggle.setBounds (L.returnToggle.toNearestInt());
    stopTimeSlider.setBounds  (L.sliderTrack[0].toNearestInt());
    startTimeSlider.setBounds (L.sliderTrack[1].toNearestInt());
    curveSlider.setBounds     (L.sliderTrack[2].toNearestInt());

    // Cache the panel's radial-gradient background (the one full-area fill).
    if (getWidth() > 0 && getHeight() > 0)
    {
        background = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        juce::Graphics ig (background);

        // Opaque dark base so the panel's rounded corners read as the gradient's
        // dark stop rather than showing through to the host behind the editor.
        ig.fillAll (tape::panelBot);

        auto b = getLocalBounds().toFloat();
        const float radius = 16.0f * L.s;
        const float cx = b.getCentreX();
        const float cy = -0.10f * b.getHeight();
        const float rad = b.getHeight() * 0.95f;

        juce::ColourGradient grad (tape::panelTop, cx, cy, tape::panelBot, cx, cy + rad, true);
        grad.addColour (0.6, tape::panelBot); // reach the dark stop by ~60% of the radius

        juce::Path panel;
        panel.addRoundedRectangle (b, radius);
        ig.setGradientFill (grad);
        ig.fillPath (panel);

        ig.setColour (tape::hairline);
        ig.strokePath (panel, juce::PathStrokeType (1.0f));
    }
}

// --- Paint -----------------------------------------------------------------
void TapeStopAudioProcessorEditor::drawLabel (juce::Graphics& g, const juce::String& text,
                                              juce::Font font, juce::Colour colour,
                                              juce::Rectangle<float> bounds, juce::Justification just,
                                              float trackingEm)
{
    font = font.withExtraKerningFactor (trackingEm); // 0 == no tracking (no-op)
    g.setFont (font);
    g.setColour (colour);
    g.drawText (text, bounds.toNearestInt(), just, false);
}

void TapeStopAudioProcessorEditor::drawHeader (juce::Graphics& g, const Layout& L)
{
    drawLabel (g, "Tape Stop", fonts.semi (16.0f * L.s), tape::textStrong, L.title, juce::Justification::centredLeft, -0.01f);

    g.setColour (tape::badgeBlue.withAlpha (0.12f));
    g.fillRoundedRectangle (L.pill, L.pill.getHeight() * 0.5f);
    drawLabel (g, "WIND-DOWN", fonts.semi (10.0f * L.s), tape::badgeBlue, L.pill, juce::Justification::centred, 0.14f);
}

void TapeStopAudioProcessorEditor::drawHub (juce::Graphics& g, juce::Point<float> c, float radius,
                                            juce::Colour capA, juce::Colour capB, float angleDeg)
{
    auto full = juce::Rectangle<float> (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);

    // Tape pancake: dark base + fine concentric rings (repeating-radial-gradient).
    g.setColour (juce::Colour (0xff12161c));
    g.fillEllipse (full);
    g.setColour (juce::Colour (0xff1c2129));
    const float ringStep = juce::jmax (2.0f, radius * (4.0f / 41.0f));
    for (int k = 0; ; ++k)
    {
        // Integer counter; radius recomputed each step (no float-accumulation drift).
        const float rr = radius - ringStep * (0.5f + (float) k);
        if (rr <= radius * 0.12f)
            break;
        auto ring = juce::Rectangle<float> (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
        g.drawEllipse (ring, juce::jmax (1.0f, ringStep * 0.5f));
    }

    // Inner shadow at the rim (inset 0 0 10px rgba(0,0,0,0.6)).
    {
        juce::ColourGradient sh (juce::Colours::transparentBlack, c.x, c.y,
                                 juce::Colours::black.withAlpha (0.6f), c.x, c.y + radius, true);
        sh.addColour (0.7, juce::Colours::transparentBlack);
        g.setGradientFill (sh);
        g.fillEllipse (full);
    }

    // Spinning spokes (repeating-conic white wedges over the inset-22 disc).
    {
        const float sr = radius * (19.0f / 41.0f);
        juce::Path spokes;
        for (int k = 0; k < 12; ++k)
        {
            const float from = juce::degreesToRadians (angleDeg + (float) k * 30.0f);
            spokes.addPieSegment (c.x - sr, c.y - sr, sr * 2.0f, sr * 2.0f,
                                  from, from + juce::degreesToRadians (4.0f), 0.0f);
        }
        g.setColour (tape::whiteAlpha (0.22f));
        g.fillPath (spokes);
    }

    // Centre cap: glow halo, then a radial-gradient hub.
    const float cr = radius * (11.0f / 41.0f);
    auto cap = juce::Rectangle<float> (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    {
        juce::ColourGradient glow (capA.withAlpha (0.45f), c.x, c.y,
                                   juce::Colours::transparentBlack, c.x, c.y + cr * 2.2f, true);
        g.setGradientFill (glow);
        g.fillEllipse (cap.expanded (cr * 1.1f));
    }
    juce::ColourGradient capGrad (capA, c.x - cr * 0.24f, c.y - cr * 0.4f,
                                  capB, c.x + cr, c.y + cr, true);
    g.setGradientFill (capGrad);
    g.fillEllipse (cap);
}

void TapeStopAudioProcessorEditor::drawCassette (juce::Graphics& g, const Layout& L, float speed, float phase)
{
    juce::ignoreUnused (phase);
    const float s = L.s;
    const float cr = 12.0f * s;

    // Drop shadow (0 10px 26px), faked with a few translucent layers.
    for (int i = 1; i <= 4; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillRoundedRectangle (L.cassette.translated (0.0f, (float) i * 3.0f * s), cr);
    }

    // Shell.
    g.setGradientFill (juce::ColourGradient (tape::cassetteTop, L.cassette.getX(), L.cassette.getY(),
                                             tape::cassetteBot, L.cassette.getX() + L.cassette.getWidth() * 0.15f,
                                             L.cassette.getBottom(), false));
    g.fillRoundedRectangle (L.cassette, cr);
    g.setColour (tape::whiteAlpha (0.08f));
    g.drawRoundedRectangle (L.cassette.reduced (0.5f), cr, juce::jmax (1.0f, s));

    // Four corner screws (radial-gradient dots, 6px, 8px inset).
    {
        const float d = 6.0f * s, in = 8.0f * s;
        const juce::Point<float> screws[] = {
            { L.cassette.getX() + in,                  L.cassette.getY() + in },
            { L.cassette.getRight() - in - d,          L.cassette.getY() + in },
            { L.cassette.getX() + in,                  L.cassette.getBottom() - in - d },
            { L.cassette.getRight() - in - d,          L.cassette.getBottom() - in - d }
        };
        for (auto sp : screws)
        {
            auto r = juce::Rectangle<float> (sp.x, sp.y, d, d);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4150), r.getX() + d * 0.35f, r.getY() + d * 0.30f,
                                                     juce::Colour (0xff11141a), r.getRight(), r.getBottom(), true));
            g.fillEllipse (r);
        }
    }

    // Label strip.
    g.setColour (tape::whiteAlpha (0.04f));
    g.fillRoundedRectangle (L.labelStrip, 6.0f * s);
    g.setColour (tape::hairline);
    g.drawRoundedRectangle (L.labelStrip.reduced (0.5f), 6.0f * s, juce::jmax (1.0f, s));

    auto stripInner = L.labelStrip.reduced (12.0f * s, 0.0f);
    drawLabel (g, "TYPE IV", fonts.mono (10.0f * s), tape::textDim, stripInner, juce::Justification::centredLeft, 0.2f);

    g.setGradientFill (spectrumGradient (L.spectrumBar));
    g.fillRoundedRectangle (L.spectrumBar, L.spectrumBar.getHeight() * 0.5f);

    const int pct = juce::roundToInt (speed * 100.0f);
    drawLabel (g, juce::String (pct) + "%", fonts.mono (10.0f * s), tape::textDim, stripInner, juce::Justification::centredRight, 0.16f);

    // Tape window.
    g.setColour (tape::tapeWindow);
    g.fillRoundedRectangle (L.tapeWindow, 8.0f * s);
    {
        juce::ColourGradient is (juce::Colours::black.withAlpha (0.7f), L.tapeWindow.getCentreX(), L.tapeWindow.getY(),
                                 juce::Colours::transparentBlack, L.tapeWindow.getCentreX(), L.tapeWindow.getY() + 12.0f * s, false);
        g.setGradientFill (is);
        g.fillRoundedRectangle (L.tapeWindow, 8.0f * s);
    }
    g.setColour (tape::whiteAlpha (0.05f));
    g.drawRoundedRectangle (L.tapeWindow.reduced (0.5f), 8.0f * s, juce::jmax (1.0f, s));

    // Tape bridge line across the window centre.
    g.setColour (tape::whiteAlpha (0.06f));
    g.fillRect (juce::Rectangle<float> (L.tapeWindow.getX(), L.tapeWindow.getCentreY() - s, L.tapeWindow.getWidth(), 2.0f * s));

    // Both hubs share the same angle / direction.
    drawHub (g, L.hubLeft,  L.hubRadius, tape::cyan, tape::violet, (float) hubAngleDeg);
    drawHub (g, L.hubRight, L.hubRadius, tape::pink, tape::amber,  (float) hubAngleDeg);
}

void TapeStopAudioProcessorEditor::drawTapeBar (juce::Graphics& g, const Layout& L, float speed)
{
    const float s = L.s;
    drawLabel (g, "TAPE SPEED", fonts.sans (10.0f * s), tape::textDimmer, L.tapeBarLabel, juce::Justification::centredLeft, 0.1f);

    const int pct = juce::roundToInt (speed * 100.0f);
    drawLabel (g, juce::String (pct) + "%", fonts.mono (10.0f * s), tape::textMid, L.tapeBarLabel, juce::Justification::centredRight);

    g.setColour (tape::track);
    g.fillRoundedRectangle (L.tapeBarTrack, L.tapeBarTrack.getHeight() * 0.5f);

    if (speed > 0.001f)
    {
        auto fill = L.tapeBarTrack.withWidth (L.tapeBarTrack.getWidth() * speed);
        // Soft glow under the fill (0 0 8px rgba(255,130,90,0.4)).
        g.setColour (juce::Colour (0xffff825a).withAlpha (0.30f));
        g.fillRoundedRectangle (fill.expanded (0.0f, 2.0f * s), fill.getHeight());
        g.setGradientFill (spectrumGradient (fill));
        g.fillRoundedRectangle (fill, fill.getHeight() * 0.5f);
    }
}

void TapeStopAudioProcessorEditor::drawHistogram (juce::Graphics& g, const Layout& L, float curve, float phase)
{
    const float s = L.s;
    const int   N = 44;
    const float gap = 2.0f * s;
    const float totalW = L.histogram.getWidth();
    const float barW = (totalW - gap * (float) (N - 1)) / (float) N;
    const float corner = 2.0f * s;

    for (int i = 0; i < N; ++i)
    {
        const float u  = (float) i / (float) (N - 1);
        const float sp = tape::speedShape (u, curve);
        const float h  = L.histogram.getHeight() * (0.04f + sp * 0.96f);
        const float x  = L.histogram.getX() + (float) i * (barW + gap);
        auto bar = juce::Rectangle<float> (x, L.histogram.getBottom() - h, barW, h);

        const bool lit = (u <= phase + 1.0e-6f);
        g.setColour (lit ? tape::spectrum (1.0f - sp) : tape::barIdle);
        fillTopRounded (g, bar, corner);
    }

    // Playhead at x = phase, extending ~4px above the bars.
    const float px = L.histogram.getX() + phase * totalW;
    const float top = L.histogram.getY() - 4.0f * s;
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillRect (juce::Rectangle<float> (px - 1.5f * s, top, 3.0f * s, L.histogram.getBottom() - top));
    g.setColour (juce::Colours::white);
    g.fillRect (juce::Rectangle<float> (px - juce::jmax (0.5f, s * 0.5f), top, juce::jmax (1.0f, 2.0f * s), L.histogram.getBottom() - top));
}

void TapeStopAudioProcessorEditor::drawSliderLabels (juce::Graphics& g, const Layout& L)
{
    const float s = L.s;
    const char* names[3] = { "STOP TIME", "START TIME", "CURVE" };
    const juce::String values[3] = {
        tape::formatMs ((float) stopTimeSlider.getValue()),
        tape::formatMs ((float) startTimeSlider.getValue()),
        juce::String (curveSlider.getValue(), 2)
    };

    for (int i = 0; i < 3; ++i)
    {
        drawLabel (g, names[i],  fonts.sans (10.0f * s), tape::textDimmer, L.sliderLabel[i], juce::Justification::centredLeft, 0.08f);
        drawLabel (g, values[i], fonts.mono (10.0f * s), tape::textMid,    L.sliderLabel[i], juce::Justification::centredRight);
    }
}

void TapeStopAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto L = computeLayout ((float) getWidth());

    if (background.isValid())
        g.drawImageAt (background, 0, 0);
    else
        g.fillAll (tape::panelBot);

    const float speed = juce::jlimit (0.0f, 1.0f, processorRef.getSpeed());
    const float phase = juce::jlimit (0.0f, 1.0f, processorRef.getPhase());
    const float curve = (float) curveSlider.getValue();

    drawHeader (g, L);
    drawCassette (g, L, speed, phase);
    drawTapeBar (g, L, speed);

    // Return row label (the toggle paints itself).
    drawLabel (g, "RETURN", fonts.sans (10.0f * L.s), tape::textDimmer, L.returnLabel, juce::Justification::centredLeft, 0.08f);

    // Slowdown-curve label row + graph.
    drawLabel (g, "SLOWDOWN CURVE", fonts.sans (10.0f * L.s), tape::textDimmer, L.curveLabel, juce::Justification::centredLeft, 0.1f);
    drawLabel (g, tape::curveLabel (curve), fonts.mono (10.0f * L.s), tape::amber, L.curveLabel, juce::Justification::centredRight);
    drawHistogram (g, L, curve, phase);

    // Divider above the sliders.
    g.setColour (tape::hairline);
    g.fillRect (L.divider);

    drawSliderLabels (g, L);
}
