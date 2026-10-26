#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <array>
#include <atomic>

namespace studio::ui
{
    // Central obsidian visualizer: dB/time grid plus a live glowing crimson waveform.
    class WaveformDisplay final : public juce::Component,
                                  private juce::Timer
    {
    public:
        explicit WaveformDisplay (MidSide670AudioProcessor&);

        void paint (juce::Graphics&) override;

    private:
        void timerCallback() override;

        MidSide670AudioProcessor& processor;
        static constexpr int viewColumns = 256;

        std::array<float, viewColumns> minTrace {};
        std::array<float, viewColumns> maxTrace {};
        int numColumns = 0;

        const std::atomic<float>* modeParameter = nullptr;
        bool midSide = true;
        float displayGrMid = 0.0f;
        float displayGrSide = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
    };
} // namespace studio::ui
