#include "RetroLookAndFeel.h"

#include <cmath>

namespace retro80s::ui
{
RetroLookAndFeel::RetroLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, charcoalDeep);
    setColour (juce::Label::textColourId, inkBlack);
    setColour (juce::TextButton::buttonColourId, silverBase);
    setColour (juce::TextButton::buttonOnColourId, signalRed);
    setColour (juce::TextButton::textColourOffId, inkBlack);
    setColour (juce::TextButton::textColourOnId, inkLightOnDark);
    setColour (juce::ComboBox::backgroundColourId, charcoalDeep);
    setColour (juce::ComboBox::textColourId, inkLightOnDark);
    setColour (juce::ComboBox::outlineColourId, seamDark);
    setColour (juce::ComboBox::arrowColourId, signalRed);
    setColour (juce::ComboBox::focusedOutlineColourId, signalRed);
    setColour (juce::PopupMenu::backgroundColourId, charcoalDeep);
    setColour (juce::PopupMenu::textColourId, inkLightOnDark);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, signalRed);
    setColour (juce::PopupMenu::highlightedTextColourId, inkLightOnDark);
    setColour (juce::TooltipWindow::backgroundColourId, charcoalDeep);
    setColour (juce::TooltipWindow::textColourId, inkLightOnDark);
    setColour (juce::TooltipWindow::outlineColourId, signalRed);
    setColour (juce::Slider::rotarySliderFillColourId, signalRed);
    setColour (juce::Slider::thumbColourId, signalRed);
}

void RetroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height))
                            .reduced (2.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    if (radius < 9.0f)
        return;

    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + juce::jlimit (0.0f, 1.0f, sliderPosProportional)
                                           * (rotaryEndAngle - rotaryStartAngle);

    const float scaleOuter = radius - 0.5f;
    const float scaleInnerMajor = radius - 6.0f;
    const float scaleInnerMinor = radius - 4.0f;
    const float redZoneStart = rotaryStartAngle + 0.75f * (rotaryEndAngle - rotaryStartAngle);

    // Moulded silkscreen scale in the silver faceplate.
    juce::Path redBand;
    redBand.addCentredArc (centre.x, centre.y, scaleOuter - 1.4f, scaleOuter - 1.4f,
                           0.0f, redZoneStart, rotaryEndAngle, true);
    g.setColour (signalRed);
    g.strokePath (redBand, juce::PathStrokeType (1.8f));

    for (int i = 0; i <= 20; ++i)
    {
        const float prop = static_cast<float> (i) / 20.0f;
        const float a = rotaryStartAngle + prop * (rotaryEndAngle - rotaryStartAngle);
        const bool isMajor = (i % 2 == 0);
        const bool inRedZone = prop >= 0.75f;

        const auto p1 = centre.getPointOnCircumference (isMajor ? scaleInnerMajor : scaleInnerMinor, a);
        const auto p2 = centre.getPointOnCircumference (scaleOuter, a);

        g.setColour (inRedZone ? signalRed : (isMajor ? inkBlack : inkCharcoal));
        g.drawLine ({ p1, p2 }, isMajor ? 1.4f : 0.8f);
    }

    // Recessed mounting well.
    const float wellR = radius * 0.745f;
    const juce::Rectangle<float> wellBounds (centre.x - wellR, centre.y - wellR,
                                              wellR * 2.0f, wellR * 2.0f);
    g.setColour (seamHighlight);
    g.fillEllipse (wellBounds.translated (0.8f, 1.0f));
    g.setColour (charcoalDeep);
    g.fillEllipse (wellBounds);

    // Drop shadow and fluted charcoal skirt.
    const float skirtR = radius * 0.685f;
    const juce::Rectangle<float> skirtBounds (centre.x - skirtR, centre.y - skirtR,
                                               skirtR * 2.0f, skirtR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillEllipse (skirtBounds.translated (0.0f, 2.1f));

    juce::ColourGradient skirtGrad (charcoalBezel.brighter (0.13f), skirtBounds.getTopLeft(),
                                    charcoalDeep, skirtBounds.getBottomRight(), false);
    g.setGradientFill (skirtGrad);
    g.fillEllipse (skirtBounds);

    for (int i = 0; i < 24; ++i)
    {
        const float fluteAngle = angle + static_cast<float> (i) * (juce::MathConstants<float>::twoPi / 24.0f);
        const auto f1 = centre.getPointOnCircumference (skirtR * 0.78f, fluteAngle);
        const auto f2 = centre.getPointOnCircumference (skirtR * 0.98f, fluteAngle);
        g.setColour (i % 2 == 0 ? juce::Colours::black.withAlpha (0.52f)
                                : juce::Colours::white.withAlpha (0.10f));
        g.drawLine ({ f1, f2 }, 1.2f);
    }

    // Brushed silver cap with a crisp inner step.
    const float capR = radius * 0.505f;
    const juce::Rectangle<float> capBounds (centre.x - capR, centre.y - capR,
                                             capR * 2.0f, capR * 2.0f);
    juce::ColourGradient capGrad (silverLight.brighter (0.08f), capBounds.getTopLeft(),
                                  silverShadow, capBounds.getBottomRight(), false);
    capGrad.addColour (0.5, silverBase);
    g.setGradientFill (capGrad);
    g.fillEllipse (capBounds);

    g.setColour (seamHighlight);
    g.drawEllipse (capBounds, 1.0f);
    g.setColour (seamDark.withAlpha (0.55f));
    g.drawEllipse (capBounds.reduced (2.0f), 0.8f);

    // Moulded signal-red pointer wedge.
    const auto pStart = centre.getPointOnCircumference (capR * 0.18f, angle);
    const auto pEnd = centre.getPointOnCircumference (skirtR * 0.95f, angle);

    g.setColour (juce::Colours::black.withAlpha (0.50f));
    g.drawLine ({ pStart.translated (0.7f, 1.0f), pEnd.translated (0.7f, 1.0f) }, 3.5f);
    g.setColour (signalRed);
    g.drawLine ({ pStart, pEnd }, 2.9f);
    g.setColour (signalRedBright);
    g.drawLine ({ pStart, pEnd }, 1.0f);
}

void RetroLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                             const juce::Colour&, bool isHighlighted,
                                             bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = button.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), 3.0f);

    if (isDown)
        bounds = bounds.translated (0.0f, 1.2f);

    juce::ColourGradient fill (on ? signalRed : silverLight,
                               bounds.getTopLeft(),
                               on ? signalRedDark : silverMid,
                               bounds.getBottomLeft(), false);
    if (on)
        fill.addColour (0.45, signalRed);

    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 2.5f);

    g.setColour (on ? signalRedBright.withAlpha (0.55f) : seamHighlight);
    g.drawRoundedRectangle (bounds.reduced (0.7f), 2.0f, 1.0f);
    g.setColour (seamDark.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds, 2.5f, 1.0f);

    if (! on)
    {
        g.setColour (signalRed.withAlpha (isHighlighted ? 0.85f : 0.5f));
        g.fillRect (bounds.getX() + 4.0f, bounds.getBottom() - 2.5f,
                    bounds.getWidth() - 8.0f, 1.4f);
    }
}

void RetroLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                       bool, bool isDown)
{
    g.setColour (button.getToggleState() ? inkLightOnDark : inkBlack);
    g.setFont (font (13.5f, Weight::black, 0.02f));

    const auto bounds = button.getLocalBounds().toFloat().translated (0.0f, isDown ? 1.0f : 0.0f);
    g.drawText (button.getButtonText(), bounds, juce::Justification::centred, false);
}

juce::Font RetroLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return font (13.0f, Weight::black, 0.03f);
}

void RetroLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                 static_cast<float> (width),
                                                 static_cast<float> (height));

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f), 3.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (bounds.reduced (0.5f), 2.5f);

    g.setColour (seamHighlight.withAlpha (0.42f));
    g.drawRoundedRectangle (bounds.reduced (0.8f), 2.2f, 0.8f);
    g.setColour (signalRed.withAlpha (0.75f));
    g.drawRoundedRectangle (bounds.reduced (2.0f).withHeight (1.0f).translated (0.0f, 1.0f), 1.0f, 0.8f);

    const juce::Rectangle<float> arrowZone (static_cast<float> (buttonX),
                                             static_cast<float> (buttonY),
                                             static_cast<float> (buttonW),
                                             static_cast<float> (buttonH));
    const auto arrowCentre = arrowZone.getCentre();

    juce::Path arrow;
    arrow.startNewSubPath (arrowCentre.x - 4.5f, arrowCentre.y - 2.0f);
    arrow.lineTo (arrowCentre.x, arrowCentre.y + 2.8f);
    arrow.lineTo (arrowCentre.x + 4.5f, arrowCentre.y - 2.0f);
    g.setColour (box.isMouseOver() ? signalRedBright : signalRed);
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

juce::Font RetroLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (10.0f, Weight::semibold, 0.055f);
}

void RetroLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (10, 1, box.getWidth() - 32, box.getHeight() - 2);
    label.setFont (font (10.0f, Weight::semibold, 0.055f));
    label.setColour (juce::Label::textColourId, inkLightOnDark);
    label.setJustificationType (juce::Justification::centredLeft);
}

void RetroLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                 static_cast<float> (width),
                                                 static_cast<float> (height));
    g.setColour (charcoalDeep);
    g.fillRect (bounds);
    g.setColour (signalRed.withAlpha (0.35f));
    g.drawRect (bounds.reduced (0.5f), 1.0f);
    g.setColour (seamHighlight.withAlpha (0.30f));
    g.drawHorizontalLine (1, 1.0f, bounds.getWidth() - 1.0f);
}

void RetroLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted,
                                          bool isTicked, bool hasSubMenu, const juce::String& text,
                                          const juce::String&, const juce::Drawable*,
                                          const juce::Colour*)
{
    auto bounds = area.toFloat().reduced (1.0f);

    if (isSeparator)
    {
        g.setColour (seamDark);
        g.fillRect (bounds.getX() + 6.0f, bounds.getCentreY(), bounds.getWidth() - 12.0f, 1.0f);
        g.setColour (seamHighlight.withAlpha (0.18f));
        g.fillRect (bounds.getX() + 6.0f, bounds.getCentreY() + 1.0f,
                    bounds.getWidth() - 12.0f, 1.0f);
        return;
    }

    if (isHighlighted)
    {
        g.setColour (signalRed.withAlpha (isActive ? 0.95f : 0.30f));
        g.fillRect (bounds);
        g.setColour (signalRedBright.withAlpha (0.5f));
        g.fillRect (bounds.withHeight (1.0f));
    }

    if (isTicked)
    {
        const auto tick = juce::Rectangle<float> (bounds.getX() + 7.0f, bounds.getCentreY() - 2.5f,
                                                   5.0f, 5.0f);
        g.setColour (isHighlighted ? inkLightOnDark : signalRed);
        g.fillRect (tick);
    }

    g.setColour (isActive ? (isHighlighted ? inkLightOnDark : inkLightOnDark.withAlpha (0.82f))
                          : inkMutedOnDark);
    g.setFont (font (10.5f, Weight::semibold, 0.035f));
    g.drawText (text, bounds.reduced (20.0f, 0.0f), juce::Justification::centredLeft, false);

    if (hasSubMenu)
    {
        const auto centre = bounds.getCentre();
        juce::Path arrow;
        arrow.startNewSubPath (centre.x + bounds.getWidth() * 0.36f, centre.y - 4.0f);
        arrow.lineTo (centre.x + bounds.getWidth() * 0.43f, centre.y);
        arrow.lineTo (centre.x + bounds.getWidth() * 0.36f, centre.y + 4.0f);
        g.setColour (inkLightOnDark.withAlpha (0.7f));
        g.strokePath (arrow, juce::PathStrokeType (1.2f));
    }
}

juce::Font RetroLookAndFeel::getPopupMenuFont()
{
    return font (10.5f, Weight::semibold, 0.03f);
}

void RetroLookAndFeel::getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight, int& idealWidth,
                                                  int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 120;
        idealHeight = 9;
        return;
    }

    idealHeight = juce::jmax (24, standardMenuItemHeight);
    idealWidth = juce::jlimit (180, 520, text.length() * 7 + 60);
}

void RetroLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                 static_cast<float> (width),
                                                 static_cast<float> (height)).reduced (1.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (bounds, 3.0f);
    g.setColour (signalRed.withAlpha (0.65f));
    g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    g.setColour (inkLightOnDark);
    g.setFont (font (10.0f, Weight::regular, 0.02f));
    g.drawFittedText (text, bounds.toNearestInt().reduced (8, 3),
                      juce::Justification::centredLeft, 2);
}
} // namespace retro80s::ui
