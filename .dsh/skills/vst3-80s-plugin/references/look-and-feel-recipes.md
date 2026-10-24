# 1980s Silver & Red Plastic Electronics — JUCE 8 Drawing Recipes

Use these reference implementations in `Source/ui/Theme.cpp`, `Source/ui/RetroLookAndFeel.cpp`, and `Source/ui/Controls.cpp` to achieve an authentic **1980s tape/radio combo (boombox / component deck)** look and feel.

---

## 1. Molded Silver Panel, Parting Seams & Silkscreened Red Frame

```cpp
void drawPlasticSeamHorizontal (juce::Graphics& g, float x1, float x2, float y)
{
    g.setColour (seamDark);
    g.fillRect (x1, y, x2 - x1, 1.0f);
    g.setColour (seamHighlight);
    g.fillRect (x1, y + 1.0f, x2 - x1, 1.0f);
}

void drawPlasticSeamVertical (juce::Graphics& g, float x, float y1, float y2)
{
    g.setColour (seamDark);
    g.fillRect (x, y1, 1.0f, y2 - y1);
    g.setColour (seamHighlight);
    g.fillRect (x + 1.0f, y1, 1.0f, y2 - y1);
}

void drawRecessedScrew (juce::Graphics& g, juce::Point<float> centre, float radius = 4.5f)
{
    const juce::Rectangle<float> well (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (seamDark);
    g.fillEllipse (well);
    g.setColour (seamHighlight.withAlpha (0.85f));
    g.drawEllipse (well.translated (0.5f, 0.5f), 0.8f);

    const auto head = well.reduced (1.2f);
    juce::ColourGradient grad (silverMid, head.getTopLeft(), silverShadow, head.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillEllipse (head);

    g.setColour (charcoalDeep.withAlpha (0.85f));
    g.drawLine (centre.x - radius * 0.48f, centre.y, centre.x + radius * 0.48f, centre.y, 1.1f);
    g.drawLine (centre.x, centre.y - radius * 0.48f, centre.x, centre.y + radius * 0.48f, 1.1f);
}

void drawMoldedModuleBay (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          const juce::String& badgeNumber,
                          const juce::String& title,
                          const juce::String& subtitle,
                          bool active)
{
    constexpr float corner = 4.5f;

    // 1. Outer molded plastic recess seam
    g.setColour (seamDark);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
    g.setColour (seamHighlight);
    g.drawRoundedRectangle (bounds.translated (0.0f, 1.0f), corner, 1.0f);

    // 2. Brushed/molded silver panel fill
    auto inner = bounds.reduced (1.5f);
    juce::ColourGradient panelGrad (silverLight, inner.getTopLeft(),
                                    silverMid, inner.getBottomLeft(), false);
    panelGrad.addColour (0.5, silverBase);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (inner, corner - 1.0f);

    // 3. Silkscreened red graphic frame inside the silver bay
    auto redFrame = inner.reduced (8.0f, 8.0f);
    g.setColour (signalRed);
    g.drawRoundedRectangle (redFrame, 3.0f, 1.5f);

    // 4. Solid red header block at top-left of the silkscreen frame
    const float headerH = 22.0f;
    auto headerBar = redFrame.removeFromTop (headerH);
    g.setColour (signalRed);
    g.fillRect (headerBar.withHeight (headerH));

    // Number chip inside header bar
    auto numBox = headerBar.removeFromLeft (32.0f).reduced (3.0f, 3.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (numBox, 2.0f);
    g.setColour (inkLightOnDark);
    g.setFont (font (10.0f, FontWeight::black, 0.08f));
    g.drawText (badgeNumber, numBox, juce::Justification::centred, false);

    // Module title in crisp white/silver silkscreen ink on red bar
    g.setColour (inkLightOnDark);
    g.setFont (font (12.5f, FontWeight::black, 0.10f));
    g.drawText (title, headerBar.reduced (8.0f, 0.0f), juce::Justification::centredLeft, false);

    // Subtitle strip directly below red header bar on silver surface
    auto subStrip = juce::Rectangle<float> (inner.getX() + 14.0f, inner.getY() + 33.0f,
                                            inner.getWidth() - 28.0f, 16.0f);
    g.setColour (inkCharcoal);
    g.setFont (font (8.0f, FontWeight::semibold, 0.14f));
    g.drawText (subtitle, subStrip, juce::Justification::centredLeft, false);

    // Status indicator text (physical hardware never goes transparent when bypassed)
    g.setColour (active ? signalRed : inkMuted);
    g.setFont (font (8.0f, FontWeight::black, 0.12f));
    g.drawText (active ? "ACTIVE" : "BYPASS", subStrip.withTrimmedRight (68.0f),
                juce::Justification::centredRight, false);
}
```

---

## 2. 1980s Fluted Knob with Silkscreened Scale & Red Pointer (`drawRotarySlider`)

```cpp
void RetroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height))
                            .reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    if (radius < 8.0f)
        return;

    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + juce::jlimit (0.0f, 1.0f, sliderPosProportional)
                                           * (rotaryEndAngle - rotaryStartAngle);

    // 1. Silkscreened radial scale ticks on the silver faceplate
    const float scaleOuter = radius - 1.5f;
    const float scaleInnerMajor = radius - 6.5f;
    const float scaleInnerMinor = radius - 4.5f;

    // Red overload zone band (75% to 100%)
    const float redZoneStart = rotaryStartAngle + 0.75f * (rotaryEndAngle - rotaryStartAngle);
    juce::Path redBand;
    redBand.addCentredArc (centre.x, centre.y, scaleOuter - 1.0f, scaleOuter - 1.0f,
                           0.0f, redZoneStart, rotaryEndAngle, true);
    g.setColour (signalRed);
    g.strokePath (redBand, juce::PathStrokeType (2.0f));

    for (int i = 0; i <= 20; ++i)
    {
        const float prop = static_cast<float> (i) / 20.0f;
        const float a = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        const bool isMajor = (i % 2 == 0);
        const bool inRedZone = (prop >= 0.75f);

        const auto p1 = centre.getPointOnCircumference (isMajor ? scaleInnerMajor : scaleInnerMinor, a);
        const auto p2 = centre.getPointOnCircumference (scaleOuter, a);

        g.setColour (inRedZone ? signalRed : (isMajor ? inkBlack : inkCharcoal));
        g.drawLine ({ p1, p2 }, isMajor ? 1.5f : 0.9f);
    }

    // 2. Recessed dark plastic mounting well
    const float wellR = radius * 0.74f;
    const juce::Rectangle<float> wellBounds (centre.x - wellR, centre.y - wellR, wellR * 2.0f, wellR * 2.0f);
    g.setColour (seamHighlight);
    g.fillEllipse (wellBounds.translated (0.8f, 1.0f));
    g.setColour (charcoalDeep);
    g.fillEllipse (wellBounds);

    // 3. Drop shadow + Fluted charcoal plastic skirt
    const float skirtR = radius * 0.68f;
    const juce::Rectangle<float> skirtBounds (centre.x - skirtR, centre.y - skirtR, skirtR * 2.0f, skirtR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (skirtBounds.translated (0.0f, 2.2f));

    juce::ColourGradient skirtGrad (charcoalBezel.brighter (0.12f), skirtBounds.getTopLeft(),
                                    charcoalDeep, skirtBounds.getBottomRight(), false);
    g.setGradientFill (skirtGrad);
    g.fillEllipse (skirtBounds);

    // Fluted grip serrations around the skirt
    for (int i = 0; i < 24; ++i)
    {
        const float fluteAngle = angle + static_cast<float> (i) * (juce::MathConstants<float>::twoPi / 24.0f);
        const auto f1 = centre.getPointOnCircumference (skirtR * 0.78f, fluteAngle);
        const auto f2 = centre.getPointOnCircumference (skirtR * 0.98f, fluteAngle);
        g.setColour (i % 2 == 0 ? juce::Colours::black.withAlpha (0.55f)
                                : juce::Colours::white.withAlpha (0.10f));
        g.drawLine ({ f1, f2 }, 1.2f);
    }

    // 4. Inner brushed metallic silver cap
    const float capR = radius * 0.50f;
    const juce::Rectangle<float> capBounds (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);
    juce::ColourGradient capGrad (silverLight.brighter (0.08f), capBounds.getTopLeft(),
                                  silverShadow, capBounds.getBottomRight(), false);
    capGrad.addColour (0.5, silverBase);
    g.setGradientFill (capGrad);
    g.fillEllipse (capBounds);

    g.setColour (seamHighlight);
    g.drawEllipse (capBounds, 1.0f);
    g.setColour (seamDark.withAlpha (0.5f));
    g.drawEllipse (capBounds.reduced (2.0f), 0.75f);

    // 5. Molded Red Pointer Wedge
    const auto pStart = centre.getPointOnCircumference (capR * 0.22f, angle);
    const auto pEnd   = centre.getPointOnCircumference (skirtR * 0.94f, angle);
    g.setColour (juce::Colours::black.withAlpha (0.50f));
    g.drawLine ({ pStart.translated (0.7f, 1.0f), pEnd.translated (0.7f, 1.0f) }, 3.4f);
    g.setColour (signalRed);
    g.drawLine ({ pStart, pEnd }, 2.8f);
    g.setColour (signalRedBright);
    g.drawLine ({ pStart, pEnd }, 1.0f);
}
```

---

## 3. Smoked Acrylic Window & Discrete LED Bargraph Meter

```cpp
void drawSmokedAcrylicBezel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Outer molded charcoal bezel
    g.setColour (seamHighlight);
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 4.0f);
    g.setColour (charcoalBezel);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Inner red silkscreen pinstripe around window
    auto redRim = bounds.reduced (3.0f);
    g.setColour (signalRed);
    g.drawRoundedRectangle (redRim, 2.5f, 1.2f);

    // Deep smoked acrylic window cavity
    auto glass = bounds.reduced (5.0f);
    g.setColour (smokedWindow);
    g.fillRoundedRectangle (glass, 2.0f);

    // Diagonal glass reflection highlight
    juce::Path reflection;
    reflection.startNewSubPath (glass.getX() + glass.getWidth() * 0.12f, glass.getY());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.28f, glass.getY());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.18f, glass.getBottom());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.02f, glass.getBottom());
    reflection.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.fillPath (reflection);
}

void drawDiscreteLedRow (juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         float normalisedLevel,
                         float normalisedPeak,
                         int numSegments = 16,
                         bool allRedMode = false)
{
    const float gap = 2.0f;
    const float segW = (bounds.getWidth() - gap * static_cast<float> (numSegments - 1))
                       / static_cast<float> (numSegments);
    const int litCount = juce::roundToInt (juce::jlimit (0.0f, 1.0f, normalisedLevel) * numSegments);
    const int peakIndex = juce::roundToInt (juce::jlimit (0.0f, 1.0f, normalisedPeak) * numSegments) - 1;

    for (int i = 0; i < numSegments; ++i)
    {
        const float prop = static_cast<float> (i) / static_cast<float> (numSegments);
        const juce::Rectangle<float> cell (bounds.getX() + static_cast<float> (i) * (segW + gap),
                                           bounds.getY(), segW, bounds.getHeight());
        const bool isLit = (i < litCount) || (i == peakIndex && peakIndex >= 0);

        juce::Colour litCol   = ledRedLit;
        juce::Colour unlitCol = ledRedUnlit;
        if (! allRedMode)
        {
            if (prop < 0.62f)      { litCol = ledGreenLit; unlitCol = ledGreenUnlit; }
            else if (prop < 0.84f) { litCol = ledAmberLit; unlitCol = ledAmberUnlit; }
        }

        g.setColour (isLit ? litCol : unlitCol);
        g.fillRoundedRectangle (cell, 1.0f);

        if (isLit)
        {
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillRect (cell.reduced (1.0f).withHeight (1.5f));
        }
    }
}
```
