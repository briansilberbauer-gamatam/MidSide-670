#include "PluginEditor.h"

#include "Parameters.h"
#include "ui/Controls.h"
#include "ui/Theme.h"

#include <array>
#include <cmath>
#include <utility>

using namespace retro80s::ui;

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
        midTrimKnob = std::make_unique<ParamKnob> (apvts, midside670::params::midTrim,
                                                   "MID COMP", formatDb);
        sideTrimKnob = std::make_unique<ParamKnob> (apvts, midside670::params::sideTrim,
                                                    "SIDE COMP", formatDb);
        outputKnob = std::make_unique<ParamKnob> (apvts, midside670::params::outputGain,
                                                  "OUTPUT GAIN", formatDb);
        mixKnob = std::make_unique<ParamKnob> (apvts, midside670::params::mix,
                                               "PARALLEL MIX", formatPercent);
        widthKnob = std::make_unique<ParamKnob> (apvts, midside670::params::width,
                                                 "STEREO WIDTH", formatPercent);

        for (auto* knob : { inputKnob.get(), tubeKnob.get(), hpfKnob.get(), thresholdKnob.get(),
                            ratioKnob.get(), linkKnob.get(), timeConstantKnob.get(),
                            midTrimKnob.get(), sideTrimKnob.get(), outputKnob.get(),
                            mixKnob.get(), widthKnob.get() })
        {
            addAndMakeVisible (*knob);
        }

        modeSwitch = std::make_unique<ModeSwitch> (apvts, midside670::params::mode);
        addAndMakeVisible (*modeSwitch);

        bypassButton = std::make_unique<LatchButton> (apvts, midside670::params::bypass, "BYPASS");
        addAndMakeVisible (*bypassButton);

        grMeter = std::make_unique<GainReductionMeter> (processor);
        addAndMakeVisible (*grMeter);

        outputMeter = std::make_unique<OutputMeter> (processor);
        addAndMakeVisible (*outputMeter);

        inputMeter = std::make_unique<InputMeter> (processor);
        addAndMakeVisible (*inputMeter);

        presetBar = std::make_unique<PresetBar> (processor);
        addAndMakeVisible (*presetBar);

        lastBypassed = isBypassed();
        startTimerHz (12);
    }

    ~MidSide670EditorContent() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        drawChassis (g, bounds);

        const bool active = ! isBypassed();

        paintHeader (g);

        const juce::Rectangle<float> inputPanel (18.0f, 100.0f, 256.0f, 512.0f);
        const juce::Rectangle<float> compressorPanel (286.0f, 100.0f, 604.0f, 512.0f);
        const juce::Rectangle<float> outputPanel (902.0f, 100.0f, 260.0f, 512.0f);

        drawModulePanel (g, inputPanel, "01", "INPUT STAGE",
                         "TRANSFORMER COUPLED / TRIODE DRIVE", active);
        drawModulePanel (g, compressorPanel, "02", "VARIABLE-MU COMPRESSOR",
                         "DUAL DETECTOR / PROGRAM-DEPENDENT RELEASE", active);
        drawModulePanel (g, outputPanel, "03", "MATRIX / OUTPUT",
                         "WIDTH / PARALLEL / TRANSFORMER OUT", active);

        paintFooter (g);

        // Small hardware annotations.
        g.setColour (inkMuted);
        g.setFont (font (5.8f, Weight::semibold, 0.12f));
        g.drawText ("LEVEL / PEAK", 42.0f, 540.0f, 210.0f, 10.0f,
                    juce::Justification::centredLeft, false);
        g.drawText ("6DJ8 DUAL TRIODE MODEL", 42.0f, 558.0f, 210.0f, 10.0f,
                    juce::Justification::centredLeft, false);
        g.drawText ("DC-COUPLED / 4x OVERSAMPLED", 42.0f, 572.0f, 210.0f, 10.0f,
                    juce::Justification::centredLeft, false);

        g.setColour (inkMuted);
        g.drawText ("600 OHM / TRANSFORMER BALANCED", 918.0f, 592.0f, 228.0f, 9.0f,
                    juce::Justification::centredRight, false);

        // Thin red calibration marks at the panel top corners.
        for (const auto& panel : { inputPanel, compressorPanel, outputPanel })
        {
            g.setColour (signalRed.withAlpha (0.78f));
            g.fillRect (panel.getX() + 8.0f, panel.getY() + 6.0f, 20.0f, 1.2f);
            g.fillRect (panel.getRight() - 28.0f, panel.getY() + 6.0f, 20.0f, 1.2f);
        }

        if (isBypassed())
        {
            const auto strip = juce::Rectangle<float> (inputPanel.getX() + 8.0f,
                                                        inputPanel.getBottom() - 10.0f,
                                                        inputPanel.getWidth() - 16.0f, 2.0f);
            g.setColour (signalRed.withAlpha (0.9f));
            g.fillRect (strip);
        }
    }

    void resized() override
    {
        inputKnob->setBounds (38, 158, 216, 182);
        tubeKnob->setBounds (30, 350, 108, 134);
        hpfKnob->setBounds (154, 350, 108, 134);
        inputMeter->setBounds (38, 498, 216, 38);

        grMeter->setBounds (304, 152, 568, 138);
        thresholdKnob->setBounds (310, 306, 164, 134);
        ratioKnob->setBounds (500, 306, 164, 134);
        linkKnob->setBounds (690, 306, 164, 134);
        timeConstantKnob->setBounds (310, 454, 164, 134);
        midTrimKnob->setBounds (500, 454, 164, 134);
        sideTrimKnob->setBounds (690, 454, 164, 134);

        outputKnob->setBounds (930, 154, 204, 170);
        mixKnob->setBounds (912, 330, 116, 134);
        widthKnob->setBounds (1016, 330, 116, 134);
        outputMeter->setBounds (914, 480, 236, 104);

        modeSwitch->setBounds (806, 18, 236, 58);
        bypassButton->setBounds (1052, 18, 96, 58);

        presetBar->setBounds (38, 634, 620, 54);
    }

private:
    void timerCallback() override
    {
        const bool bypassed = isBypassed();

        if (bypassed != lastBypassed)
        {
            lastBypassed = bypassed;
            repaint();
        }
    }

    bool isBypassed() const noexcept
    {
        if (auto* value = apvts.getRawParameterValue (midside670::params::bypass))
            return value->load() > 0.5f;

        return false;
    }

    void paintHeader (juce::Graphics& g)
    {
        drawRedBadge (g, { 28.0f, 18.0f, 68.0f, 54.0f }, "670", "VARIABLE-MU");

        // Embossed silkscreen title.
        g.setColour (seamHighlight);
        g.setFont (font (30.0f, Weight::black, 0.025f));
        g.drawText ("MID/SIDE-670", 109, 11, 500, 39, juce::Justification::centredLeft, false);

        g.setColour (inkBlack);
        g.setFont (font (30.0f, Weight::black, 0.025f));
        g.drawText ("MID/SIDE-670", 108, 10, 500, 39, juce::Justification::centredLeft, false);

        g.setColour (inkCharcoal);
        g.setFont (font (7.6f, Weight::semibold, 0.16f));
        g.drawText ("VARIABLE-MU TUBE COMPRESSOR   /   TRUE MID-SIDE DYNAMICS",
                    109, 51, 510, 13, juce::Justification::centredLeft, false);

        g.setColour (signalRed);
        g.fillRect (109.0f, 67.0f, 150.0f, 1.4f);

        g.setColour (inkMuted);
        g.setFont (font (6.4f, Weight::semibold, 0.16f));
        g.drawText ("BLS AUDIO  /  SERIES 670  /  STEREO MATRIX",
                    109, 73, 380, 10, juce::Justification::centredLeft, false);

        g.setColour (inkMuted);
        g.setFont (font (6.0f, Weight::semibold, 0.13f));
        g.drawText ("4x OVERSAMPLED", 642, 59, 130, 9,
                    juce::Justification::centredLeft, false);

        g.setColour (signalRed);
        g.fillRect (1064.0f, 80.0f, 84.0f, 1.0f);
    }

    void paintFooter (juce::Graphics& g)
    {
        const auto footer = juce::Rectangle<float> (24.0f, 626.0f, 1132.0f, 70.0f);

        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (footer.translated (0.0f, 2.0f), 4.0f);

        juce::ColourGradient fill (charcoalBezel, footer.getTopLeft(),
                                   charcoalDeep, footer.getBottomLeft(), false);
        fill.addColour (0.5, juce::Colour (0xff1d2025));
        g.setGradientFill (fill);
        g.fillRoundedRectangle (footer, 4.0f);

        g.setColour (seamHighlight.withAlpha (0.55f));
        g.drawRoundedRectangle (footer.reduced (0.7f), 3.5f, 0.8f);
        g.setColour (signalRed.withAlpha (0.65f));
        g.fillRect (footer.getX() + 3.0f, footer.getY() + 3.0f, footer.getWidth() - 6.0f, 1.0f);

        // Signal path chips.
        const float chipY = 635.0f;
        const float chipW = 76.0f;
        const float chipGap = 14.0f;
        float chipX = 680.0f;
        const std::array<const char*, 4> stages { "M/S ENCODE", "VARIABLE-MU", "M/S DECODE", "WIDTH" };

        for (size_t i = 0; i < stages.size(); ++i)
        {
            const auto chip = juce::Rectangle<float> (chipX, chipY, chipW, 19.0f);
            g.setColour (charcoalDeep);
            g.fillRoundedRectangle (chip, 2.0f);
            g.setColour (signalRed.withAlpha (0.72f));
            g.drawRoundedRectangle (chip.reduced (0.5f), 2.0f, 0.8f);
            g.setColour (inkLightOnDark.withAlpha (0.90f));
            g.setFont (font (5.8f, Weight::black, 0.10f));
            g.drawText (stages[i], chip, juce::Justification::centred, false);

            if (i + 1 < stages.size())
            {
                g.setColour (signalRed.withAlpha (0.80f));
                g.setFont (font (8.0f, Weight::black, 0.0f));
                g.drawText (">", juce::Rectangle<float> (chip.getRight(), chip.getY(),
                                                         chipGap, chip.getHeight()),
                            juce::Justification::centred, false);
            }

            chipX += chipW + chipGap;
        }

        g.setColour (inkMutedOnDark);
        g.setFont (font (5.8f, Weight::semibold, 0.12f));
        g.drawText ("TRUE MID/SIDE MATRIX  //  4x OVERSAMPLED  //  CONSTANT-LATENCY BYPASS",
                    680, 662, 360, 9, juce::Justification::centredLeft, false);

        // Branding / format plate.
        g.setColour (inkLightOnDark.withAlpha (0.92f));
        g.setFont (font (7.6f, Weight::black, 0.12f));
        g.drawText ("BLS AUDIO", 1030, 633, 120, 11,
                    juce::Justification::centredRight, false);

        const auto vst3Badge = juce::Rectangle<float> (1074.0f, 648.0f, 66.0f, 18.0f);
        g.setColour (signalRed);
        g.fillRoundedRectangle (vst3Badge, 1.5f);
        g.setColour (inkLightOnDark);
        g.setFont (font (7.2f, Weight::black, 0.18f));
        g.drawText ("VST3", vst3Badge, juce::Justification::centred, false);

        g.setColour (inkMutedOnDark);
        g.setFont (font (5.5f, Weight::semibold, 0.12f));
        g.drawText ("64-BIT FLOAT / 4x OS", 1010, 671, 140, 9,
                    juce::Justification::centredRight, false);
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
    std::unique_ptr<ParamKnob> midTrimKnob;
    std::unique_ptr<ParamKnob> sideTrimKnob;
    std::unique_ptr<ParamKnob> outputKnob;
    std::unique_ptr<ParamKnob> mixKnob;
    std::unique_ptr<ParamKnob> widthKnob;

    std::unique_ptr<ModeSwitch> modeSwitch;
    std::unique_ptr<LatchButton> bypassButton;
    std::unique_ptr<GainReductionMeter> grMeter;
    std::unique_ptr<OutputMeter> outputMeter;
    std::unique_ptr<InputMeter> inputMeter;
    std::unique_ptr<PresetBar> presetBar;

    bool lastBypassed = false;

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
    g.fillAll (seamDark);
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
