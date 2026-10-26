#include "PluginEditor.h"

#include "Parameters.h"
#include "Presets.h"
#include "ui/Controls.h"
#include "ui/Theme.h"
#include "ui/WaveformDisplay.h"

#include <array>
#include <cmath>
#include <utility>

using namespace studio::ui;

namespace
{
constexpr int designWidth = 1180;
constexpr int designHeight = 720;

juce::String formatDb (float value)
{
    if (std::abs (value) < 0.05f)
        return "0.0 dB";

    return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
}

juce::String formatPercent (float value)
{
    return juce::String (juce::roundToInt (value)) + " %";
}

juce::String formatRatio (float value)
{
    return juce::String (value, 1) + ":1";
}

juce::String formatTimeConstant (float value)
{
    const int position = juce::jlimit (1, 6, juce::roundToInt (value));
    const auto timeConstant = midside670::dsp::getTimeConstant (position);
    return "TC " + juce::String (position) + " / " + juce::String (timeConstant.character);
}

juce::String formatHz (float value)
{
    return juce::String (juce::roundToInt (value)) + " Hz";
}
} // namespace

class MidSide670EditorContent final : public juce::Component,
                                      private juce::Timer
{
public:
    explicit MidSide670EditorContent (MidSide670AudioProcessor& p)
        : processor (p),
          apvts (p.getValueTreeState())
    {
        setOpaque (true);

        inputKnob = std::make_unique<ParamKnob> (apvts, midside670::params::inputGain,
                                                 "INPUT GAIN", formatDb);
        tubeKnob = std::make_unique<ParamKnob> (apvts, midside670::params::tubeDrive,
                                                "TUBE DRIVE", formatPercent);
        hpfKnob = std::make_unique<ParamKnob> (apvts, midside670::params::sidechainHpf,
                                               "SC HIGH-PASS", formatHz);
        thresholdKnob = std::make_unique<ParamKnob> (apvts, midside670::params::threshold,
                                                     "THRESHOLD", formatDb);
        ratioKnob = std::make_unique<ParamKnob> (apvts, midside670::params::ratio,
                                                 "CURVE / RATIO", formatRatio);
        linkKnob = std::make_unique<ParamKnob> (apvts, midside670::params::link,
                                                "M/S LINK", formatPercent);
        timeConstantKnob = std::make_unique<ParamKnob> (apvts, midside670::params::timeConstant,
                                                        "TIME CONSTANT", formatTimeConstant);
        widthKnob = std::make_unique<ParamKnob> (apvts, midside670::params::width,
                                                 "STEREO WIDTH", formatPercent);

        for (auto* knob : { inputKnob.get(), tubeKnob.get(), hpfKnob.get(), thresholdKnob.get(),
                            ratioKnob.get(), linkKnob.get(), timeConstantKnob.get(),
                            widthKnob.get() })
        {
            addAndMakeVisible (*knob);
        }

        outputSlider = std::make_unique<GlassSlider> (apvts, midside670::params::outputGain,
                                                      "OUTPUT", formatDb);
        mixSlider = std::make_unique<GlassSlider> (apvts, midside670::params::mix,
                                                   "PARALLEL MIX", formatPercent);
        midSlider = std::make_unique<GlassSlider> (apvts, midside670::params::midTrim,
                                                   "MID COMP", formatDb);
        sideSlider = std::make_unique<GlassSlider> (apvts, midside670::params::sideTrim,
                                                    "SIDE COMP", formatDb);

        for (auto* slider : { outputSlider.get(), mixSlider.get(), midSlider.get(), sideSlider.get() })
            addAndMakeVisible (*slider);

        stereoButton = std::make_unique<ModeButton> (apvts, midside670::params::mode,
                                                     midside670::params::stereoMode, "STEREO");
        midSideButton = std::make_unique<ModeButton> (apvts, midside670::params::mode,
                                                      midside670::params::midSideMode, "M/S");
        autoButton = std::make_unique<TactileButton> (apvts, midside670::params::autoMakeup, "AUTO");
        bypassButton = std::make_unique<TactileButton> (apvts, midside670::params::bypass, "BYPASS");

        addAndMakeVisible (*stereoButton);
        addAndMakeVisible (*midSideButton);
        addAndMakeVisible (*autoButton);
        addAndMakeVisible (*bypassButton);

        waveformDisplay = std::make_unique<WaveformDisplay> (processor);
        addAndMakeVisible (*waveformDisplay);

        inputMeter = std::make_unique<InputMeter> (processor);
        addAndMakeVisible (*inputMeter);

        outputMeter = std::make_unique<OutputMeter> (processor);
        addAndMakeVisible (*outputMeter);

        pillBar = std::make_unique<HeaderPillBar> (processor);
        pillBar->onPresetChanged = [this] { repaint(); };
        addAndMakeVisible (*pillBar);

        lastBypassed = isBypassed();
        lastPreset = processor.getCurrentPresetIndex();
        lastDirty = processor.isPresetDirty();
        startTimerHz (12);
    }

    ~MidSide670EditorContent() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        drawDesktopHardwareChassis (g, bounds);

        paintBrandHeader (g);

        g.setColour (obsidianGrid.withAlpha (0.90f));
        g.fillRect (52.0f, 97.0f, 1076.0f, 1.0f);

        drawEngravedLabel (g, { 52.0f, 102.0f, 244.0f, 26.0f },
                           "INPUT STAGE", "TRANSFORMER COUPLED / TRIODE DRIVE");
        drawEngravedLabel (g, { 328.0f, 318.0f, 524.0f, 26.0f },
                           "GAIN COMPUTER", "DUAL DETECTOR / PROGRAM-DEPENDENT RELEASE");
        drawEngravedLabel (g, { 884.0f, 102.0f, 244.0f, 26.0f },
                           "MATRIX / OUTPUT", "WIDTH / PARALLEL / CHANNEL TRIM");

        paintAnnotations (g);
        paintFooter (g);
    }

    void resized() override
    {
        pillBar->setBounds (386, 44, 408, 36);

        stereoButton->setBounds (810, 48, 70, 30);
        midSideButton->setBounds (886, 48, 70, 30);
        autoButton->setBounds (962, 48, 76, 30);
        bypassButton->setBounds (1044, 48, 84, 30);

        waveformDisplay->setBounds (328, 110, 524, 202);

        inputKnob->setBounds (54, 128, 240, 196);
        tubeKnob->setBounds (70, 340, 140, 154);
        inputMeter->setBounds (60, 502, 228, 26);

        thresholdKnob->setBounds (324, 344, 176, 126);
        ratioKnob->setBounds (500, 344, 176, 126);
        linkKnob->setBounds (676, 344, 176, 126);

        timeConstantKnob->setBounds (324, 478, 176, 122);
        hpfKnob->setBounds (500, 478, 176, 122);
        widthKnob->setBounds (676, 478, 176, 122);

        outputSlider->setBounds (884, 126, 58, 430);
        mixSlider->setBounds (946, 126, 58, 430);
        midSlider->setBounds (1008, 126, 58, 430);
        sideSlider->setBounds (1070, 126, 58, 430);

        outputMeter->setBounds (792, 614, 336, 38);
    }

private:
    void timerCallback() override
    {
        const bool bypassed = isBypassed();
        const int preset = processor.getCurrentPresetIndex();
        const bool dirty = processor.isPresetDirty();

        if (bypassed != lastBypassed || preset != lastPreset || dirty != lastDirty)
        {
            lastBypassed = bypassed;
            lastPreset = preset;
            lastDirty = dirty;
            repaint();
        }
    }

    bool isBypassed() const noexcept
    {
        if (auto* value = apvts.getRawParameterValue (midside670::params::bypass))
            return value->load() > 0.5f;

        return false;
    }

    void paintBrandHeader (juce::Graphics& g)
    {
        g.setColour (textSecondary.withAlpha (0.85f));
        g.setFont (font (6.3f, Weight::semibold, 0.22f));
        g.drawText ("BLS AUDIO  /  SERIES 670", 52, 38, 280, 9,
                    juce::Justification::centredLeft, false);

        g.setColour (textPrimary);
        g.setFont (font (21.0f, Weight::semibold, 0.04f));
        g.drawText ("MIDSIDE-670", 52, 47, 320, 27,
                    juce::Justification::centredLeft, false);

        g.setColour (textSecondary);
        g.setFont (font (6.6f, Weight::regular, 0.14f));
        g.drawText ("VARIABLE-MU TUBE COMPRESSOR  /  TRUE MID-SIDE DYNAMICS",
                    52, 75, 330, 9, juce::Justification::centredLeft, false);

        g.setColour (crimsonAccent);
        g.fillRect (52.0f, 87.0f, 54.0f, 1.4f);
    }

    void paintAnnotations (juce::Graphics& g)
    {
        g.setFont (font (6.0f, Weight::regular, 0.10f));
        g.setColour (textMuted);

        g.drawText ("6DJ8 DUAL TRIODE / TRANSFORMER COUPLED",
                    54, 536, 236, 9, juce::Justification::centredLeft, false);
        g.drawText ("DC-COUPLED / 4x OVERSAMPLED",
                    54, 549, 236, 9, juce::Justification::centredLeft, false);
        g.drawText ("NO FEEDBACK LOOP",
                    54, 562, 236, 9, juce::Justification::centredLeft, false);

        g.drawText ("600 OHM / TRANSFORMER BALANCED",
                    884, 568, 244, 9, juce::Justification::centredLeft, false);
        g.drawText ("TRUE M/S MATRIX / CONSTANT LATENCY",
                    884, 581, 244, 9, juce::Justification::centredLeft, false);
    }

    void paintFooter (juce::Graphics& g)
    {
        g.setColour (obsidianGrid);
        g.fillRect (52.0f, 608.0f, 1076.0f, 1.0f);

        g.setColour (textMuted);
        g.setFont (font (6.2f, Weight::semibold, 0.16f));
        g.drawText ("PRESET CHARACTER", 52, 616, 240, 9,
                    juce::Justification::centredLeft, false);

        const int preset = processor.getCurrentPresetIndex();
        const bool dirty = processor.isPresetDirty();

        g.setColour (textSecondary);
        g.setFont (font (7.4f, Weight::regular, 0.08f));
        g.drawFittedText (midside670::getFactoryPresetCharacter (preset),
                          juce::Rectangle<int> (52, 628, 268, 22),
                          juce::Justification::centredLeft, 2);

        drawCrimsonDot (g, { 55.0f, 660.0f }, 2.0f);

        g.setColour (dirty ? crimsonBright : textMuted);
        g.setFont (font (6.2f, Weight::semibold, 0.14f));
        g.drawText (dirty ? "EDITED / UNSAVED" : "FACTORY / CLEAN",
                    64, 655, 240, 9, juce::Justification::centredLeft, false);

        // Signal-path chips.
        const std::array<const char*, 4> stages { "M/S ENCODE", "VARIABLE-MU", "M/S DECODE", "WIDTH" };
        float chipX = 338.0f;
        constexpr float chipW = 82.0f;
        constexpr float chipGap = 12.0f;

        for (size_t i = 0; i < stages.size(); ++i)
        {
            const auto chip = juce::Rectangle<float> (chipX, 620.0f, chipW, 18.0f);
            drawSignalChip (g, chip, stages[i]);

            if (i + 1 < stages.size())
            {
                g.setColour (crimsonAccent.withAlpha (0.80f));
                g.setFont (font (7.5f, Weight::semibold, 0.0f));
                g.drawText (">", juce::Rectangle<float> (chip.getRight(), chip.getY(),
                                                          chipGap, chip.getHeight()),
                            juce::Justification::centred, false);
            }

            chipX += chipW + chipGap;
        }

        g.setColour (textMuted);
        g.setFont (font (5.8f, Weight::semibold, 0.12f));
        g.drawText ("TRUE MID/SIDE MATRIX  //  4x OVERSAMPLED  //  CONSTANT-LATENCY BYPASS",
                    338, 648, 420, 9, juce::Justification::centredLeft, false);

        g.setColour (textSecondary);
        g.setFont (font (6.2f, Weight::semibold, 0.16f));
        g.drawText ("BLS AUDIO  /  VST3  /  64-BIT FLOAT",
                    792, 658, 336, 9, juce::Justification::centredRight, false);
    }

    MidSide670AudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<ParamKnob> inputKnob;
    std::unique_ptr<ParamKnob> tubeKnob;
    std::unique_ptr<ParamKnob> hpfKnob;
    std::unique_ptr<ParamKnob> thresholdKnob;
    std::unique_ptr<ParamKnob> ratioKnob;
    std::unique_ptr<ParamKnob> linkKnob;
    std::unique_ptr<ParamKnob> timeConstantKnob;
    std::unique_ptr<ParamKnob> widthKnob;

    std::unique_ptr<GlassSlider> outputSlider;
    std::unique_ptr<GlassSlider> mixSlider;
    std::unique_ptr<GlassSlider> midSlider;
    std::unique_ptr<GlassSlider> sideSlider;

    std::unique_ptr<ModeButton> stereoButton;
    std::unique_ptr<ModeButton> midSideButton;
    std::unique_ptr<TactileButton> autoButton;
    std::unique_ptr<TactileButton> bypassButton;

    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<InputMeter> inputMeter;
    std::unique_ptr<OutputMeter> outputMeter;
    std::unique_ptr<HeaderPillBar> pillBar;

    bool lastBypassed = false;
    int lastPreset = 0;
    bool lastDirty = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidSide670EditorContent)
};

//======================================================================================
MidSide670AudioProcessorEditor::MidSide670AudioProcessorEditor (MidSide670AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    content = std::make_unique<MidSide670EditorContent> (processor);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (885, 540, 1770, 1080);

    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (static_cast<double> (designWidth)
                                          / static_cast<double> (designHeight));

    setSize (designWidth, designHeight);
}

MidSide670AudioProcessorEditor::~MidSide670AudioProcessorEditor()
{
    content.reset();
    setLookAndFeel (nullptr);
}

void MidSide670AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (studioSurfaceDark);
}

void MidSide670AudioProcessorEditor::resized()
{
    if (content == nullptr)
        return;

    const float scale = juce::jmin (static_cast<float> (getWidth()) / static_cast<float> (designWidth),
                                    static_cast<float> (getHeight()) / static_cast<float> (designHeight));
    const float offsetX = (static_cast<float> (getWidth()) - static_cast<float> (designWidth) * scale) * 0.5f;
    const float offsetY = (static_cast<float> (getHeight()) - static_cast<float> (designHeight) * scale) * 0.5f;

    content->setBounds (0, 0, designWidth, designHeight);
    content->setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));
}
