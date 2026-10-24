#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace retro80s::ui
{
    // --- Molded Silver ABS Plastic Chassis ---
    inline const juce::Colour silverLight      { 0xffdce1e6 };
    inline const juce::Colour silverBase       { 0xffc6ccd3 };
    inline const juce::Colour silverMid        { 0xffb5bcc4 };
    inline const juce::Colour silverRecess     { 0xff9ea6af };
    inline const juce::Colour silverShadow     { 0xff7c848d };
    inline const juce::Colour seamDark         { 0xff5a6068 };
    inline const juce::Colour seamHighlight    { 0xffedf1f5 };

    // --- Charcoal ABS Sub-Bezels & Smoked Acrylic Windows ---
    inline const juce::Colour charcoalBezel    { 0xff262930 };
    inline const juce::Colour charcoalDeep     { 0xff191b20 };
    inline const juce::Colour smokedWindow     { 0xff111317 };

    // --- 1980s Signal Red Highlights & Molded Red Plastic ---
    inline const juce::Colour signalRed        { 0xffd9232a };
    inline const juce::Colour signalRedBright  { 0xffff3b42 };
    inline const juce::Colour signalRedDark    { 0xff8f1318 };
    inline const juce::Colour redScaleZone     { 0xffc81e24 };

    // --- Discrete LED Segments ---
    inline const juce::Colour ledRedLit        { 0xffff2a2a };
    inline const juce::Colour ledRedUnlit      { 0xff381417 };
    inline const juce::Colour ledAmberLit      { 0xffffa826 };
    inline const juce::Colour ledAmberUnlit    { 0xff332412 };
    inline const juce::Colour ledGreenLit      { 0xff36d65b };
    inline const juce::Colour ledGreenUnlit    { 0xff142e1b };

    // --- Silkscreened Ink Typography ---
    inline const juce::Colour inkBlack         { 0xff181a1e };
    inline const juce::Colour inkCharcoal      { 0xff3a3f47 };
    inline const juce::Colour inkMuted         { 0xff5c636e };
    inline const juce::Colour inkLightOnDark   { 0xffe8edf2 };
    inline const juce::Colour inkMutedOnDark   { 0xff98a1ab };

    enum class Weight
    {
        regular,
        semibold,
        black
    };

    juce::Font font (float height, Weight weight = Weight::regular, float tracking = 0.0f);

    void drawChassis (juce::Graphics&, juce::Rectangle<float> bounds);
    void drawPlasticSeamHorizontal (juce::Graphics&, float x1, float x2, float y);
    void drawPlasticSeamVertical (juce::Graphics&, float x, float y1, float y2);
    void drawRecessedScrew (juce::Graphics&, juce::Point<float> centre, float radius = 4.5f);
    void drawModulePanel (juce::Graphics&,
                          juce::Rectangle<float> bounds,
                          const juce::String& number,
                          const juce::String& title,
                          const juce::String& subtitle,
                          bool active);
    void drawSmokedBezel (juce::Graphics&, juce::Rectangle<float> bounds);
    void drawVentSlots (juce::Graphics&, juce::Rectangle<float> bounds, int count);
    void drawRedBadge (juce::Graphics&, juce::Rectangle<float> bounds,
                       const juce::String& topLine, const juce::String& bottomLine);

    juce::Rectangle<float> moduleContentBounds (juce::Rectangle<float> panel);
} // namespace retro80s::ui
