#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "Theme.h"

#include <functional>
#include <memory>

namespace retro80s::ui
{
    //==================================================================================
    // Physical fluted knob with silkscreened caption and a recessed counter window.
    //==================================================================================
    class ParamKnob final : public juce::Component,
                            private juce::Timer
    {
    public:
        ParamKnob (juce::AudioProcessorValueTreeState&,
                   const juce::String& parameterID,
                   juce::String caption,
                   std::function<juce::String (float)> formatter,
                   juce::Colour accent = signalRed);

        ~ParamKnob() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseEnter (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;

    private:
        void timerCallback() override;
        void refreshValueText();

        juce::AudioProcessorValueTreeState& apvts;
        juce::String parameterID;
        juce::String captionText;
        std::function<juce::String (float)> formatValue;
        juce::Colour accentColour;

        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String valueText;
        bool hover = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
    };

    //==================================================================================
    // Two-position mechanical slide switch for STEREO / MID-SIDE.
    //==================================================================================
    class ModeSwitch final : public juce::Component,
                             private juce::Timer
    {
    public:
        ModeSwitch (juce::AudioProcessorValueTreeState&, const juce::String& parameterID);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;

        bool isMidSide() const noexcept;

    private:
        void timerCallback() override;
        void setFromX (float x);

        juce::AudioProcessorValueTreeState& apvts;
        juce::String parameterID;
        juce::RangedAudioParameter* parameter = nullptr;
        bool lastMidSide = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeSwitch)
    };

    //==================================================================================
    // Cassette-deck style latching push key with a rectangular red LED lens.
    //==================================================================================
    class LatchButton final : public juce::Button
    {
    public:
        LatchButton (juce::AudioProcessorValueTreeState&,
                     const juce::String& parameterID,
                     const juce::String& text,
                     juce::Colour accent = signalRed);

        void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    private:
        juce::Colour accentColour;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatchButton)
    };

    //==================================================================================
    // Smoked-acrylic dual bargraph for variable-mu gain reduction.
    //==================================================================================
    class GainReductionMeter final : public juce::Component,
                                     private juce::Timer
    {
    public:
        explicit GainReductionMeter (MidSide670AudioProcessor&);

        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;
        void drawMeterRow (juce::Graphics&, juce::Rectangle<float> bounds,
                           const juce::String& label, float grDb, bool allRed) const;

        MidSide670AudioProcessor& processor;
        const std::atomic<float>* modeParameter = nullptr;
        float displayMid = 0.0f;
        float displaySide = 0.0f;
        bool midSide = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
    };

    //==================================================================================
    // Dual L/R discrete output bargraph with peak hold and a red CLIP jewel.
    //==================================================================================
    class OutputMeter final : public juce::Component,
                              private juce::Timer
    {
    public:
        explicit OutputMeter (MidSide670AudioProcessor&);

        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;
        void drawChannel (juce::Graphics&, juce::Rectangle<float> bounds, const juce::String& label,
                          float normalisedLevel, float normalisedPeak) const;

        MidSide670AudioProcessor& processor;
        float levelLeft = 0.0f;
        float levelRight = 0.0f;
        float peakLeft = 0.0f;
        float peakRight = 0.0f;
        int peakHoldLeft = 0;
        int peakHoldRight = 0;
        int clipHold = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputMeter)
    };

    //==================================================================================
    // Small input-level LED strip in the input stage module.
    //==================================================================================
    class InputMeter final : public juce::Component,
                             private juce::Timer
    {
    public:
        explicit InputMeter (MidSide670AudioProcessor&);

        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;

        MidSide670AudioProcessor& processor;
        float displayLevel = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputMeter)
    };

    //==================================================================================
    // Factory preset browser with mechanical previous/next keys and EDIT badge.
    //==================================================================================
    class PresetBar final : public juce::Component,
                            private juce::Timer
    {
    public:
        explicit PresetBar (MidSide670AudioProcessor&);

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        void step (int delta);

        MidSide670AudioProcessor& processor;
        juce::ComboBox presetBox;
        juce::TextButton previousButton;
        juce::TextButton nextButton;
        int lastPreset = -1;
        bool lastDirty = false;
        bool syncing = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
    };
} // namespace retro80s::ui
