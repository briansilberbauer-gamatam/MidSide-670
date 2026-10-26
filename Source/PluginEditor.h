#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/StudioLookAndFeel.h"

#include <memory>

class MidSide670EditorContent;

class MidSide670AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MidSide670AudioProcessorEditor (MidSide670AudioProcessor&);
    ~MidSide670AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MidSide670AudioProcessor& processor;
    studio::ui::StudioLookAndFeel lookAndFeel;
    std::unique_ptr<MidSide670EditorContent> content;
    juce::TooltipWindow tooltipWindow { this, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidSide670AudioProcessorEditor)
};
