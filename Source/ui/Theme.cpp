#include "Theme.h"

#include <BinaryData.h>

#include <cmath>

namespace retro80s::ui
{
namespace
{
juce::Typeface::Ptr makeTypeface (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
}

const juce::Typeface::Ptr& regularTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoRegular_ttf, BinaryData::LatoRegular_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& semiboldTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoSemibold_ttf, BinaryData::LatoSemibold_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& blackTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoBlack_ttf, BinaryData::LatoBlack_ttfSize);
    return typeface;
}
} // namespace

juce::Font font (float height, Weight weight, float tracking)
{
    const auto& typeface = weight == Weight::black ? blackTypeface()
                         : weight == Weight::semibold ? semiboldTypeface()
                                                      : regularTypeface();

    juce::Font result { juce::FontOptions (typeface).withHeight (height) };

    if (! juce::approximatelyEqual (tracking, 0.0f))
        result = result.withExtraKerningFactor (tracking);

    return result;
}

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

void drawRecessedScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    const juce::Rectangle<float> well (centre.x - radius, centre.y - radius,
                                       radius * 2.0f, radius * 2.0f);
    g.setColour (seamDark);
    g.fillEllipse (well);
    g.setColour (seamHighlight.withAlpha (0.85f));
    g.drawEllipse (well.translated (0.5f, 0.5f), 0.8f);

    const auto head = well.reduced (1.2f);
    juce::ColourGradient grad (silverMid, head.getTopLeft(), silverShadow, head.getBottomRight(), false);
    g.setGradientFill (grad);
    g.fillEllipse (head);

    g.setColour (charcoalDeep.withAlpha (0.85f));
    g.drawLine (centre.x - radius * 0.48f, centre.y,
                centre.x + radius * 0.48f, centre.y, 1.1f);
    g.drawLine (centre.x, centre.y - radius * 0.48f,
                centre.x, centre.y + radius * 0.48f, 1.1f);
}

void drawChassis (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    juce::ColourGradient base (silverLight.brighter (0.06f), bounds.getTopLeft(),
                               silverMid, bounds.getBottomLeft(), false);
    base.addColour (0.46, silverBase);
    g.setGradientFill (base);
    g.fillRect (bounds);

    // Very fine moulded-plastic brushing, visible only as a directional sheen.
    for (float y = bounds.getY() + 2.0f; y < bounds.getBottom(); y += 3.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (y < bounds.getCentreY() ? 0.018f : 0.010f));
        g.fillRect (bounds.getX(), y, bounds.getWidth(), 1.0f);
    }

    // Outer moulded bevel.
    g.setColour (seamHighlight); // top / left catchlight
    g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f);
    g.fillRect (bounds.getX(), bounds.getY(), 2.0f, bounds.getHeight());
    g.setColour (silverShadow);
    g.fillRect (bounds.getX(), bounds.getBottom() - 2.0f, bounds.getWidth(), 2.0f);
    g.fillRect (bounds.getRight() - 2.0f, bounds.getY(), 2.0f, bounds.getHeight());
    g.setColour (seamDark.withAlpha (0.75f));
    g.drawRect (bounds.reduced (2.0f), 1.0f);

    // Header parting seam and footer seam.
    drawPlasticSeamHorizontal (g, bounds.getX() + 8.0f, bounds.getRight() - 8.0f, 90.0f);
    drawPlasticSeamHorizontal (g, bounds.getX() + 8.0f, bounds.getRight() - 8.0f, 618.0f);

    // Recessed screw wells at the four corners.
    drawRecessedScrew (g, { 31.0f, 31.0f });
    drawRecessedScrew (g, { bounds.getRight() - 31.0f, 31.0f });
    drawRecessedScrew (g, { 31.0f, bounds.getBottom() - 30.0f });
    drawRecessedScrew (g, { bounds.getRight() - 31.0f, bounds.getBottom() - 30.0f });

    // Top ventilation bank, kept clear of all controls.
    drawVentSlots (g, { 642.0f, 25.0f, 130.0f, 33.0f }, 6);

    // Tiny moulding pips under the header.
    for (int i = 0; i < 9; ++i)
    {
        const float x = 300.0f + static_cast<float> (i) * 13.0f;
        g.setColour (seamDark.withAlpha (0.28f));
        g.fillEllipse (x, 84.0f, 2.4f, 2.4f);
        g.setColour (seamHighlight.withAlpha (0.55f));
        g.fillEllipse (x, 85.2f, 2.4f, 1.2f);
    }
}

void drawModulePanel (juce::Graphics& g,
                      juce::Rectangle<float> bounds,
                      const juce::String& number,
                      const juce::String& title,
                      const juce::String& subtitle,
                      bool active)
{
    constexpr float corner = 4.5f;

    // Recessed seam around the module.
    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), corner);
    g.setColour (seamDark);
    g.drawRoundedRectangle (bounds, corner, 1.0f);
    g.setColour (seamHighlight);
    g.drawRoundedRectangle (bounds.translated (0.0f, 1.0f), corner, 1.0f);

    // Brushed silver face.
    auto inner = bounds.reduced (1.5f);
    juce::ColourGradient panelGrad (silverLight, inner.getTopLeft(),
                                    silverMid, inner.getBottomLeft(), false);
    panelGrad.addColour (0.5, silverBase);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (inner, corner - 1.0f);

    // Silkscreened signal-red frame.
    auto redFrame = inner.reduced (7.0f);
    g.setColour (signalRed);
    g.drawRoundedRectangle (redFrame, 2.5f, 1.5f);

    // Red module-header bar.
    auto header = redFrame.removeFromTop (25.0f);
    juce::ColourGradient redGrad (signalRed, header.getTopLeft(),
                                  signalRedDark, header.getBottomLeft(), false);
    g.setGradientFill (redGrad);
    g.fillRect (header);

    auto numberBox = header.removeFromLeft (31.0f).reduced (4.0f, 3.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (numberBox, 2.0f);
    g.setColour (inkLightOnDark);
    g.setFont (font (10.0f, Weight::black, 0.08f));
    g.drawText (number, numberBox, juce::Justification::centred, false);

    g.setColour (inkLightOnDark);
    g.setFont (font (11.5f, Weight::black, 0.075f));
    g.drawText (title, header.reduced (7.0f, 0.0f), juce::Justification::centredLeft, false);

    // Hardware status badge (never dims the whole module).
    auto statusBox = juce::Rectangle<float> (redFrame.getRight() - 46.0f,
                                             header.getY() + 6.0f, 42.0f, 13.0f);
    g.setColour (active ? charcoalDeep.withAlpha (0.82f) : signalRedDark);
    g.fillRoundedRectangle (statusBox, 2.0f);
    g.setColour (active ? ledGreenLit.withAlpha (0.92f) : inkLightOnDark);
    g.setFont (font (7.0f, Weight::black, 0.12f));
    g.drawText (active ? "ACTIVE" : "BYPASS", statusBox, juce::Justification::centred, false);

    // Subtitle silkscreen on silver, directly below the red bar.
    const auto subStrip = juce::Rectangle<float> (inner.getX() + 14.0f, header.getBottom() + 6.0f,
                                                  inner.getWidth() - 28.0f, 13.0f);
    g.setColour (inkCharcoal);
    g.setFont (font (7.5f, Weight::semibold, 0.15f));
    g.drawText (subtitle, subStrip, juce::Justification::centredLeft, false);

    // Bottom injection seam.
    g.setColour (seamDark.withAlpha (0.38f));
    g.fillRect (inner.getX() + 7.0f, inner.getBottom() - 8.0f, inner.getWidth() - 14.0f, 1.0f);
    g.setColour (seamHighlight.withAlpha (0.55f));
    g.fillRect (inner.getX() + 7.0f, inner.getBottom() - 7.0f, inner.getWidth() - 14.0f, 1.0f);
}

void drawSmokedBezel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Outer moulded charcoal bezel.
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 4.5f);
    g.setColour (seamHighlight.withAlpha (0.70f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 4.0f);
    g.setColour (charcoalBezel);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Inner red silkscreen pinstripe.
    auto redRim = bounds.reduced (3.0f);
    g.setColour (signalRed);
    g.drawRoundedRectangle (redRim, 2.5f, 1.2f);

    // Deep smoked-acrylic cavity.
    auto glass = bounds.reduced (5.0f);
    g.setColour (smokedWindow);
    g.fillRoundedRectangle (glass, 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (glass.reduced (2.0f).withHeight (3.0f), 1.0f);

    // Clear-plastic diagonal reflection.
    juce::Path reflection;
    reflection.startNewSubPath (glass.getX() + glass.getWidth() * 0.10f, glass.getY());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.25f, glass.getY());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.16f, glass.getBottom());
    reflection.lineTo (glass.getX() + glass.getWidth() * 0.01f, glass.getBottom());
    reflection.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.045f));
    g.fillPath (reflection);
}

void drawVentSlots (juce::Graphics& g, juce::Rectangle<float> bounds, int count)
{
    const float height = 2.6f;
    const float gap = (bounds.getHeight() - static_cast<float> (count) * height)
                      / static_cast<float> (juce::jmax (1, count - 1));

    for (int i = 0; i < count; ++i)
    {
        const float y = bounds.getY() + static_cast<float> (i) * (height + gap);
        g.setColour (charcoalDeep);
        g.fillRoundedRectangle (bounds.getX(), y, bounds.getWidth(), height, 1.2f);
        g.setColour (seamHighlight.withAlpha (0.40f));
        g.fillRect (bounds.getX() + 1.0f, y + height, bounds.getWidth() - 2.0f, 1.0f);
    }
}

void drawRedBadge (juce::Graphics& g, juce::Rectangle<float> bounds,
                   const juce::String& topLine, const juce::String& bottomLine)
{
    juce::ColourGradient red (signalRedBright, bounds.getTopLeft(),
                              signalRedDark, bounds.getBottomLeft(), false);
    red.addColour (0.32, signalRed);
    g.setGradientFill (red);
    g.fillRoundedRectangle (bounds, 2.0f);

    g.setColour (seamHighlight.withAlpha (0.38f));
    g.drawRoundedRectangle (bounds.reduced (0.7f), 1.5f, 0.8f);

    g.setColour (inkLightOnDark);
    g.setFont (font (30.0f, Weight::black, 0.01f));
    g.drawText (topLine, bounds.withTrimmedTop (-2.0f).withHeight (bounds.getHeight() * 0.64f),
                juce::Justification::centred, false);

    g.setColour (juce::Colours::white.withAlpha (0.88f));
    g.setFont (font (6.6f, Weight::black, 0.18f));
    g.drawText (bottomLine, bounds.withTrimmedTop (bounds.getHeight() * 0.57f),
                juce::Justification::centred, false);
}

juce::Rectangle<float> moduleContentBounds (juce::Rectangle<float> panel)
{
    return panel.reduced (16.0f, 12.0f).withTrimmedTop (48.0f).withTrimmedBottom (8.0f);
}
} // namespace retro80s::ui
