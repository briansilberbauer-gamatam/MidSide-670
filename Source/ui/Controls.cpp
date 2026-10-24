#include "Controls.h"

#include "Parameters.h"
#include "Presets.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace retro80s::ui
{
namespace
{
float normaliseDb (float linear, float floorDb = -48.0f)
{
    if (! std::isfinite (linear) || linear <= 0.0f)
        return 0.0f;

    const float db = juce::Decibels::gainToDecibels (linear, floorDb);
    return juce::jlimit (0.0f, 1.0f, (db - floorDb) / -floorDb);
}
} // namespace

//======================================================================================
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& state,
                      const juce::String& id,
                      juce::String caption,
                      std::function<juce::String (float)> formatter,
                      juce::Colour accent)
    : apvts (state),
      parameterID (id),
      captionText (std::move (caption)),
      formatValue (std::move (formatter)),
      accentColour (accent)
{
    setOpaque (false);
    setInterceptsMouseClicks (true, true);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, inkMuted);
    slider.setMouseDragSensitivity (180);
    slider.setScrollWheelEnabled (true);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, parameterID, slider);

    if (auto* parameter = apvts.getParameter (parameterID))
    {
        const float defaultValue = parameter->getNormalisableRange().convertFrom0to1 (
            parameter->getDefaultValue());
        slider.setDoubleClickReturnValue (true, defaultValue);
        slider.setTooltip (parameter->getName (64));
    }

    slider.onValueChange = [this] { refreshValueText(); };

    refreshValueText();
    startTimerHz (24);
}

ParamKnob::~ParamKnob()
{
    stopTimer();
}

void ParamKnob::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Red index tab beside the caption, like the printed legends on a boombox.
    g.setColour (accentColour.withAlpha (hover ? 1.0f : 0.82f));
    g.fillRect (bounds.getX() + 1.0f, bounds.getY() + 2.0f, 2.0f, 8.0f);

    g.setColour (hover ? signalRedDark : inkBlack);
    g.setFont (font (8.2f, Weight::black, 0.12f));
    g.drawFittedText (captionText,
                      bounds.withTrimmedLeft (7.0f).withHeight (12.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1);

    // Recessed mini counter for the live value.
    const auto valueBounds = juce::Rectangle<float> (bounds.getX() + 4.0f,
                                                      bounds.getBottom() - 19.0f,
                                                      bounds.getWidth() - 8.0f, 17.0f);
    g.setColour (juce::Colours::black.withAlpha (0.24f));
    g.fillRoundedRectangle (valueBounds.translated (0.0f, 1.0f), 2.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (valueBounds, 2.0f);
    g.setColour (seamHighlight.withAlpha (0.22f));
    g.drawRoundedRectangle (valueBounds.reduced (0.5f), 2.0f, 0.8f);

    g.setColour (hover ? signalRedBright : inkLightOnDark);
    g.setFont (font (8.4f, Weight::semibold, 0.04f));
    g.drawFittedText (valueText, valueBounds.toNearestInt().reduced (5, 0),
                      juce::Justification::centred, 1);
}

void ParamKnob::resized()
{
    const int titleH = 14;
    const int valueH = 23;
    auto area = getLocalBounds().withTrimmedTop (titleH).withTrimmedBottom (valueH);

    const int diameter = juce::jmin (area.getWidth(), area.getHeight());
    slider.setBounds (area.withSizeKeepingCentre (diameter, diameter));
}

void ParamKnob::mouseEnter (const juce::MouseEvent&)
{
    hover = true;
    repaint();
}

void ParamKnob::mouseExit (const juce::MouseEvent&)
{
    hover = false;
    repaint();
}

void ParamKnob::timerCallback()
{
    refreshValueText();
}

void ParamKnob::refreshValueText()
{
    if (formatValue == nullptr)
        return;

    const auto newText = formatValue (static_cast<float> (slider.getValue()));

    if (newText != valueText)
    {
        valueText = newText;
        repaint();
    }
}

//======================================================================================
ModeSwitch::ModeSwitch (juce::AudioProcessorValueTreeState& state, const juce::String& id)
    : apvts (state),
      parameterID (id),
      parameter (state.getParameter (id))
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    lastMidSide = isMidSide();
    startTimerHz (24);
}

bool ModeSwitch::isMidSide() const noexcept
{
    return parameter != nullptr && parameter->getValue() > 0.5f;
}

void ModeSwitch::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const bool midSide = isMidSide();

    g.setColour (inkBlack);
    g.setFont (font (7.2f, Weight::black, 0.14f));
    g.drawText ("MATRIX MODE", bounds.withHeight (11.0f), juce::Justification::centredLeft, false);

    g.setColour (midSide ? signalRed : inkMuted);
    g.setFont (font (6.5f, Weight::black, 0.15f));
    g.drawText (midSide ? "M/S DUAL" : "STEREO LINK",
                bounds.withHeight (11.0f).withTrimmedRight (1.0f),
                juce::Justification::centredRight, false);

    const auto track = juce::Rectangle<float> (bounds.getX(), bounds.getY() + 28.0f,
                                                bounds.getWidth(), bounds.getHeight() - 30.0f);
    const float halfWidth = track.getWidth() * 0.5f;

    // Silkscreened position labels above the slide track.
    const juce::Rectangle<float> leftLabel (track.getX(), track.getY() - 14.0f, halfWidth, 12.0f);
    const juce::Rectangle<float> rightLabel (track.getX() + halfWidth, track.getY() - 14.0f,
                                              halfWidth, 12.0f);

    g.setColour (! midSide ? signalRed : inkCharcoal);
    g.setFont (font (7.4f, Weight::black, 0.12f));
    g.drawText ("STEREO", leftLabel, juce::Justification::centred, false);

    g.setColour (midSide ? signalRed : inkCharcoal);
    g.drawText ("MID / SIDE", rightLabel, juce::Justification::centred, false);

    const float activeLabelY = track.getY() - 3.0f;
    g.setColour (signalRed);
    g.fillRect (! midSide ? track.getX() + 8.0f : track.getX() + halfWidth + 8.0f,
                activeLabelY, halfWidth - 16.0f, 1.5f);

    // Recessed charcoal switch track.
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (track.translated (0.0f, 1.5f), 3.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (seamHighlight.withAlpha (0.34f));
    g.drawRoundedRectangle (track.reduced (0.5f), 3.0f, 0.8f);
    g.setColour (signalRed.withAlpha (0.55f));
    g.drawRoundedRectangle (track.reduced (2.0f), 2.0f, 0.7f);

    // Raised slider thumb with three moulded grip ridges.
    const float thumbW = halfWidth - 3.0f;
    const float thumbX = midSide ? track.getX() + halfWidth + 1.5f : track.getX() + 1.5f;
    auto thumb = juce::Rectangle<float> (thumbX, track.getY() + 2.0f,
                                          thumbW, track.getHeight() - 4.0f);

    g.setColour (juce::Colours::black.withAlpha (0.38f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 1.5f), 2.0f);

    juce::ColourGradient thumbFill (silverLight.brighter (0.07f), thumb.getTopLeft(),
                                    silverMid, thumb.getBottomLeft(), false);
    g.setGradientFill (thumbFill);
    g.fillRoundedRectangle (thumb, 2.0f);
    g.setColour (seamDark);
    g.drawRoundedRectangle (thumb, 2.0f, 0.9f);
    g.setColour (seamHighlight.withAlpha (0.8f));
    g.drawRoundedRectangle (thumb.reduced (0.8f), 1.6f, 0.7f);

    const auto centre = thumb.getCentre();
    for (int i = -1; i <= 1; ++i)
    {
        g.setColour (signalRed.withAlpha (0.9f));
        g.fillRect (centre.x + static_cast<float> (i) * 3.0f - 0.6f,
                    thumb.getY() + 5.0f, 1.2f, thumb.getHeight() - 10.0f);
    }
}

void ModeSwitch::mouseDown (const juce::MouseEvent& event)
{
    setFromX (static_cast<float> (event.x));
}

void ModeSwitch::mouseDrag (const juce::MouseEvent& event)
{
    setFromX (static_cast<float> (event.x));
}

void ModeSwitch::setFromX (float x)
{
    if (parameter == nullptr)
        return;

    const float half = static_cast<float> (juce::jmax (1, getWidth())) * 0.5f;
    parameter->setValueNotifyingHost (x >= half ? 1.0f : 0.0f);
    lastMidSide = isMidSide();
    repaint();
}

void ModeSwitch::timerCallback()
{
    const bool nowMidSide = isMidSide();

    if (nowMidSide != lastMidSide)
    {
        lastMidSide = nowMidSide;
        repaint();
    }
}

//======================================================================================
LatchButton::LatchButton (juce::AudioProcessorValueTreeState& state,
                          const juce::String& parameterID,
                          const juce::String& buttonText,
                          juce::Colour accent)
    : Button (buttonText), accentColour (accent)
{
    setClickingTogglesState (true);
    setButtonText (buttonText);
    setTooltip (parameterID == midside670::params::bypass
                    ? "Hard bypass with latency-matched dry signal"
                    : buttonText);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        state, parameterID, *this);
}

void LatchButton::paintButton (juce::Graphics& g, bool isHighlighted, bool isDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();

    const auto ledBounds = juce::Rectangle<float> (bounds.getCentreX() - 11.0f,
                                                    bounds.getY() + 3.0f, 22.0f, 6.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (ledBounds.expanded (1.4f, 1.0f), 1.5f);
    g.setColour (on ? ledRedLit : ledRedUnlit);
    g.fillRoundedRectangle (ledBounds, 1.0f);

    if (on)
    {
        g.setColour (ledRedLit.withAlpha (0.20f));
        g.fillEllipse (ledBounds.expanded (4.0f, 3.0f));
    }

    auto well = bounds.withTrimmedTop (13.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (well, 2.5f);
    g.setColour (seamHighlight.withAlpha (0.28f));
    g.drawRoundedRectangle (well.reduced (0.5f), 2.5f, 0.8f);

    auto cap = well.reduced (3.0f);
    if (on)
        cap = cap.translated (0.0f, 1.4f);

    g.setColour (juce::Colours::black.withAlpha (0.34f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), 2.0f);

    juce::ColourGradient capFill (on ? signalRed : silverLight.brighter (isHighlighted ? 0.08f : 0.0f),
                                  cap.getTopLeft(),
                                  on ? signalRedDark : silverMid,
                                  cap.getBottomLeft(), false);
    g.setGradientFill (capFill);
    g.fillRoundedRectangle (cap, 2.0f);

    g.setColour (on ? signalRedBright.withAlpha (0.75f) : seamHighlight);
    g.drawRoundedRectangle (cap.reduced (0.7f), 1.6f, 0.8f);
    g.setColour (seamDark.withAlpha (0.9f));
    g.drawRoundedRectangle (cap, 2.0f, 1.0f);

    if (! on)
    {
        g.setColour (signalRed.withAlpha (isHighlighted ? 0.9f : 0.55f));
        g.fillRect (cap.getX() + 5.0f, cap.getBottom() - 2.2f, cap.getWidth() - 10.0f, 1.2f);
    }

    g.setColour (on ? inkLightOnDark : inkBlack);
    g.setFont (font (8.6f, Weight::black, 0.12f));
    g.drawFittedText (getButtonText(), cap.toNearestInt().translated (0, isDown ? 1 : 0),
                      juce::Justification::centred, 1);
}

//======================================================================================
GainReductionMeter::GainReductionMeter (MidSide670AudioProcessor& p)
    : processor (p),
      modeParameter (p.getValueTreeState().getRawParameterValue (midside670::params::mode))
{
    setOpaque (false);
    startTimerHz (30);
}

void GainReductionMeter::timerCallback()
{
    midSide = modeParameter != nullptr && modeParameter->load() > 0.5f;

    float targetMid = processor.getGainReductionMid();
    float targetSide = processor.getGainReductionSide();

    if (! midSide)
    {
        const float linked = juce::jmax (targetMid, targetSide);
        targetMid = linked;
        targetSide = linked;
    }

    auto smooth = [] (float& display, float target)
    {
        if (target > display)
            display = target;
        else
            display = juce::jmax (target, display - 0.16f);
    };

    smooth (displayMid, targetMid);
    smooth (displaySide, targetSide);
    repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawSmokedBezel (g, bounds);

    const auto inner = bounds.reduced (10.0f, 8.0f);

    g.setColour (inkMutedOnDark);
    g.setFont (font (7.0f, Weight::black, 0.17f));
    g.drawText ("GAIN REDUCTION", inner.withHeight (14.0f),
                juce::Justification::centredLeft, false);

    const juce::String modeTag = midSide ? "M/S DUAL" : "STEREO LINK";
    const auto tagBounds = juce::Rectangle<float> (inner.getRight() - 76.0f, inner.getY() + 1.0f,
                                                    76.0f, 13.0f);
    g.setColour (signalRed.withAlpha (0.88f));
    g.fillRoundedRectangle (tagBounds, 1.5f);
    g.setColour (inkLightOnDark);
    g.setFont (font (6.3f, Weight::black, 0.14f));
    g.drawText (modeTag, tagBounds, juce::Justification::centred, false);

    const auto rowArea = inner.withTrimmedTop (22.0f).withTrimmedBottom (15.0f);
    const float rowHeight = juce::jmin (30.0f, (rowArea.getHeight() - 10.0f) * 0.5f);
    auto topRow = juce::Rectangle<float> (rowArea.getX(), rowArea.getY() + 2.0f,
                                          rowArea.getWidth(), rowHeight);
    auto bottomRow = juce::Rectangle<float> (rowArea.getX(), rowArea.getY() + rowHeight + 10.0f,
                                             rowArea.getWidth(), rowHeight);

    drawMeterRow (g, topRow, midSide ? "MID" : "L/R", displayMid, true);
    drawMeterRow (g, bottomRow, midSide ? "SIDE" : "LINK", displaySide, true);

    // Common GR scale underneath the two rows.
    const float scaleX = inner.getX() + 56.0f;
    const float scaleW = inner.getWidth() - 56.0f - 62.0f;
    const float scaleY = inner.getBottom() - 8.0f;
    g.setFont (font (5.8f, Weight::semibold, 0.04f));

    for (int i = 0; i <= 3; ++i)
    {
        const float prop = static_cast<float> (i) / 3.0f;
        const juce::String label = i == 0 ? "0" : "-" + juce::String (i * 10);
        g.setColour (inkMutedOnDark);
        g.drawText (label,
                    juce::Rectangle<float> (scaleX + prop * scaleW - 14.0f, scaleY, 28.0f, 8.0f),
                    juce::Justification::centred, false);
    }

    g.setColour (inkMutedOnDark.withAlpha (0.8f));
    g.drawText ("GR dB",
                juce::Rectangle<float> (inner.getRight() - 52.0f, scaleY, 48.0f, 8.0f),
                juce::Justification::centredRight, false);
}

void GainReductionMeter::drawMeterRow (juce::Graphics& g, juce::Rectangle<float> bounds,
                                       const juce::String& label, float grDb, bool allRed) const
{
    constexpr int segments = 26;
    constexpr float maxGr = 30.0f;

    g.setColour (inkLightOnDark);
    g.setFont (font (9.0f, Weight::black, 0.10f));
    g.drawText (label, bounds.withWidth (46.0f), juce::Justification::centredLeft, false);

    const auto bar = bounds.withTrimmedLeft (52.0f).withTrimmedRight (60.0f).reduced (0.0f, 2.5f);
    const float gap = 2.0f;
    const float cellW = (bar.getWidth() - gap * static_cast<float> (segments - 1))
                        / static_cast<float> (segments);

    const float normalised = juce::jlimit (0.0f, 1.0f, grDb / maxGr);
    const int litCount = juce::roundToInt (normalised * static_cast<float> (segments));

    for (int i = 0; i < segments; ++i)
    {
        const auto cell = juce::Rectangle<float> (bar.getX() + static_cast<float> (i) * (cellW + gap),
                                                   bar.getY(), cellW, bar.getHeight());
        juce::Colour lit = allRed ? ledRedLit : ledGreenLit;
        juce::Colour unlit = allRed ? ledRedUnlit : ledGreenUnlit;

        if (! allRed)
        {
            const float prop = static_cast<float> (i) / static_cast<float> (segments);
            if (prop > 0.84f)
            {
                lit = ledRedLit;
                unlit = ledRedUnlit;
            }
            else if (prop > 0.62f)
            {
                lit = ledAmberLit;
                unlit = ledAmberUnlit;
            }
        }

        g.setColour (i < litCount ? lit : unlit);
        g.fillRoundedRectangle (cell, 1.0f);

        if (i < litCount)
        {
            g.setColour (juce::Colours::white.withAlpha (0.40f));
            g.fillRect (cell.reduced (1.0f).withHeight (1.1f));
        }
    }

    const auto numeric = bounds.withTrimmedLeft (bounds.getWidth() - 58.0f);
    g.setColour (grDb > 0.1f ? signalRedBright : inkMutedOnDark);
    g.setFont (font (8.4f, Weight::black, 0.02f));
    g.drawText (grDb > 0.05f ? "-" + juce::String (grDb, 1) + " dB" : "0.0 dB",
                numeric, juce::Justification::centredRight, false);
}

//======================================================================================
OutputMeter::OutputMeter (MidSide670AudioProcessor& p)
    : processor (p)
{
    setOpaque (false);
    startTimerHz (30);
}

void OutputMeter::timerCallback()
{
    auto follow = [] (float& display, float target)
    {
        if (target > display)
            display = target;
        else
            display += (target - display) * 0.22f;
    };

    follow (levelLeft, normaliseDb (processor.getOutputLevelLeft()));
    follow (levelRight, normaliseDb (processor.getOutputLevelRight()));

    auto updatePeak = [] (float current, float& held, int& holdCounter)
    {
        if (current >= held)
        {
            held = current;
            holdCounter = 30;
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
        }
        else
        {
            held = juce::jmax (current, held - 0.004f);
        }
    };

    updatePeak (levelLeft, peakLeft, peakHoldLeft);
    updatePeak (levelRight, peakRight, peakHoldRight);

    if (processor.getOutputLevel() >= 0.985f)
        clipHold = 55;
    else if (clipHold > 0)
        --clipHold;

    repaint();
}

void OutputMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawSmokedBezel (g, bounds);

    const auto inner = bounds.reduced (11.0f, 8.0f);

    g.setColour (inkMutedOnDark);
    g.setFont (font (6.8f, Weight::black, 0.16f));
    g.drawText ("OUTPUT LEVEL", inner.withHeight (12.0f), juce::Justification::centredLeft, false);

    const auto clip = juce::Rectangle<float> (inner.getRight() - 49.0f, inner.getY() + 1.0f, 49.0f, 12.0f);
    g.setColour (clipHold > 0 ? ledRedLit : ledRedUnlit);
    g.fillRoundedRectangle (clip, 1.0f);
    g.setColour (clipHold > 0 ? inkLightOnDark : inkMutedOnDark.withAlpha (0.7f));
    g.setFont (font (5.6f, Weight::black, 0.16f));
    g.drawText ("PEAK", clip, juce::Justification::centred, false);

    auto rows = inner.withTrimmedTop (16.0f).withTrimmedBottom (11.0f);
    auto leftRow = rows.removeFromTop (rows.getHeight() * 0.5f).reduced (0.0f, 1.2f);
    auto rightRow = rows.reduced (0.0f, 1.2f);

    drawChannel (g, leftRow, "L", levelLeft, peakLeft);
    drawChannel (g, rightRow, "R", levelRight, peakRight);

    const float scaleY = inner.getBottom() - 8.0f;
    const float x = inner.getX() + 25.0f;
    const float w = inner.getWidth() - 25.0f;
    g.setFont (font (5.4f, Weight::semibold, 0.02f));
    g.setColour (inkMutedOnDark);

    const std::array<std::pair<const char*, float>, 5> ticks {{
        { "-48", 0.0f }, { "-24", 0.25f }, { "-12", 0.5f }, { "-6", 0.75f }, { "0", 1.0f }
    }};

    for (const auto& tick : ticks)
        g.drawText (tick.first,
                    juce::Rectangle<float> (x + tick.second * w - 11.0f, scaleY, 22.0f, 8.0f),
                    juce::Justification::centred, false);
}

void OutputMeter::drawChannel (juce::Graphics& g, juce::Rectangle<float> bounds,
                               const juce::String& label, float normalisedLevel,
                               float normalisedPeak) const
{
    constexpr int segments = 20;

    g.setColour (inkLightOnDark);
    g.setFont (font (8.0f, Weight::black, 0.04f));
    g.drawText (label, bounds.withWidth (20.0f), juce::Justification::centredLeft, false);

    const auto bar = bounds.withTrimmedLeft (23.0f).reduced (0.0f, 1.5f);
    const float gap = 1.8f;
    const float cellW = (bar.getWidth() - gap * static_cast<float> (segments - 1))
                        / static_cast<float> (segments);
    const int litCount = juce::roundToInt (juce::jlimit (0.0f, 1.0f, normalisedLevel)
                                           * static_cast<float> (segments));

    for (int i = 0; i < segments; ++i)
    {
        const auto cell = juce::Rectangle<float> (bar.getX() + static_cast<float> (i) * (cellW + gap),
                                                   bar.getY(), cellW, bar.getHeight());
        const float prop = static_cast<float> (i) / static_cast<float> (segments);

        juce::Colour lit = ledGreenLit;
        juce::Colour unlit = ledGreenUnlit;

        if (prop >= 0.84f)
        {
            lit = ledRedLit;
            unlit = ledRedUnlit;
        }
        else if (prop >= 0.64f)
        {
            lit = ledAmberLit;
            unlit = ledAmberUnlit;
        }

        g.setColour (i < litCount ? lit : unlit);
        g.fillRoundedRectangle (cell, 0.8f);

        if (i < litCount)
        {
            g.setColour (juce::Colours::white.withAlpha (0.34f));
            g.fillRect (cell.reduced (0.8f).withHeight (1.0f));
        }
    }

    if (normalisedPeak > 0.01f)
    {
        const float px = bar.getX() + juce::jlimit (0.0f, 1.0f, normalisedPeak) * bar.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.fillRect (px - 0.6f, bar.getY() - 1.0f, 1.2f, bar.getHeight() + 2.0f);
    }
}

//======================================================================================
InputMeter::InputMeter (MidSide670AudioProcessor& p)
    : processor (p)
{
    setOpaque (false);
    startTimerHz (30);
}

void InputMeter::timerCallback()
{
    const float target = normaliseDb (processor.getInputLevel());

    if (target > displayLevel)
        displayLevel = target;
    else
        displayLevel += (target - displayLevel) * 0.25f;

    repaint();
}

void InputMeter::paint (juce::Graphics& g)
{
    constexpr int segments = 14;
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.16f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.0f), 2.0f);
    g.setColour (charcoalDeep);
    g.fillRoundedRectangle (bounds, 2.0f);
    g.setColour (seamHighlight.withAlpha (0.30f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 2.0f, 0.7f);

    auto inner = bounds.reduced (5.0f, 3.0f);
    g.setColour (inkLightOnDark);
    g.setFont (font (6.4f, Weight::black, 0.12f));
    g.drawText ("INPUT", inner.removeFromLeft (34.0f), juce::Justification::centredLeft, false);

    auto bar = inner.reduced (0.0f, 2.0f);
    const float gap = 1.8f;
    const float cellW = (bar.getWidth() - gap * static_cast<float> (segments - 1))
                        / static_cast<float> (segments);
    const int litCount = juce::roundToInt (juce::jlimit (0.0f, 1.0f, displayLevel)
                                           * static_cast<float> (segments));

    for (int i = 0; i < segments; ++i)
    {
        const auto cell = juce::Rectangle<float> (bar.getX() + static_cast<float> (i) * (cellW + gap),
                                                   bar.getY(), cellW, bar.getHeight());
        const float prop = static_cast<float> (i) / static_cast<float> (segments);
        juce::Colour lit = prop >= 0.85f ? ledRedLit : (prop >= 0.65f ? ledAmberLit : ledGreenLit);
        juce::Colour unlit = prop >= 0.85f ? ledRedUnlit : (prop >= 0.65f ? ledAmberUnlit : ledGreenUnlit);

        g.setColour (i < litCount ? lit : unlit);
        g.fillRoundedRectangle (cell, 0.8f);
    }
}

//======================================================================================
PresetBar::PresetBar (MidSide670AudioProcessor& p)
    : processor (p)
{
    setOpaque (false);

    previousButton.setButtonText ("<");
    previousButton.setTooltip ("Previous factory preset");
    nextButton.setButtonText (">");
    nextButton.setTooltip ("Next factory preset");

    auto configure = [this] (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, silverBase);
        b.setColour (juce::TextButton::textColourOffId, inkBlack);
        addAndMakeVisible (b);
    };

    configure (previousButton);
    configure (nextButton);

    previousButton.onClick = [this] { step (-1); };
    nextButton.onClick = [this] { step (1); };

    presetBox.setTooltip ("Factory preset library");
    presetBox.setJustificationType (juce::Justification::centredLeft);

    for (int i = 0; i < processor.getNumPrograms(); ++i)
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "   "
                               + processor.getProgramName (i), i + 1);

    presetBox.onChange = [this]
    {
        if (syncing)
            return;

        const int selected = presetBox.getSelectedId();

        if (selected > 0)
        {
            processor.loadFactoryPreset (selected - 1);
            lastPreset = selected - 1;
            lastDirty = processor.isPresetDirty();
            repaint();
        }
    };

    addAndMakeVisible (presetBox);

    lastPreset = processor.getCurrentPresetIndex();
    lastDirty = processor.isPresetDirty();
    presetBox.setSelectedId (lastPreset + 1, juce::dontSendNotification);

    startTimerHz (12);
}

void PresetBar::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (inkMutedOnDark);
    g.setFont (font (6.8f, Weight::black, 0.17f));
    g.drawText ("FACTORY PRESET", bounds.withHeight (13.0f),
                juce::Justification::centredLeft, false);

    const auto characterBounds = juce::Rectangle<float> (bounds.getX() + 412.0f,
                                                         bounds.getY() + 19.0f, 112.0f, 28.0f);
    const auto editBounds = juce::Rectangle<float> (bounds.getRight() - 84.0f,
                                                     bounds.getY() + 18.0f, 84.0f, 31.0f);

    g.setColour (signalRed.withAlpha (0.55f));
    g.fillRect (characterBounds.getX() - 6.0f, characterBounds.getY() + 2.0f,
                1.0f, characterBounds.getHeight() - 4.0f);

    g.setColour (inkMutedOnDark);
    g.setFont (font (5.9f, Weight::semibold, 0.10f));
    g.drawFittedText (midside670::getFactoryPresetCharacter (lastPreset),
                      characterBounds.toNearestInt(),
                      juce::Justification::centredLeft, 2);

    if (lastDirty)
    {
        g.setColour (signalRed.withAlpha (0.90f));
        g.fillRoundedRectangle (editBounds, 2.0f);
        g.setColour (inkLightOnDark);
        g.setFont (font (6.5f, Weight::black, 0.12f));
        g.drawText ("EDITED", editBounds, juce::Justification::centred, false);
    }
    else
    {
        g.setColour (ledGreenUnlit);
        g.drawRoundedRectangle (editBounds, 2.0f, 0.8f);
        g.setColour (ledGreenLit.withAlpha (0.95f));
        g.setFont (font (6.5f, Weight::black, 0.12f));
        g.drawText ("FACTORY", editBounds, juce::Justification::centred, false);
    }
}

void PresetBar::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (17);

    previousButton.setBounds (bounds.removeFromLeft (34).withHeight (32));
    bounds.removeFromLeft (4);
    presetBox.setBounds (bounds.removeFromLeft (330).withHeight (32));
    bounds.removeFromLeft (4);
    nextButton.setBounds (bounds.removeFromLeft (34).withHeight (32));
}

void PresetBar::timerCallback()
{
    const int preset = processor.getCurrentPresetIndex();
    const bool dirty = processor.isPresetDirty();

    if (preset != lastPreset)
    {
        lastPreset = preset;
        syncing = true;
        presetBox.setSelectedId (preset + 1, juce::dontSendNotification);
        syncing = false;
        repaint();
    }

    if (dirty != lastDirty)
    {
        lastDirty = dirty;
        repaint();
    }
}

void PresetBar::step (int delta)
{
    const int count = processor.getNumPrograms();

    if (count <= 0)
        return;

    const int next = (processor.getCurrentPresetIndex() + delta + count) % count;
    processor.loadFactoryPreset (next);
    lastPreset = next;
    lastDirty = false;
    syncing = true;
    presetBox.setSelectedId (next + 1, juce::dontSendNotification);
    syncing = false;
    repaint();
}
} // namespace retro80s::ui
