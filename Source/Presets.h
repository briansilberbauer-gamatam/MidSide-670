#pragma once

#include "Parameters.h"

#include <vector>

namespace midside670
{
    struct PresetValue
    {
        const char* id;
        float value;
    };

    struct FactoryPreset
    {
        const char* name;
        const char* character;
        std::vector<PresetValue> overrides;
    };

    inline const std::vector<FactoryPreset>& getFactoryPresets()
    {
        using namespace params;

        static const std::vector<FactoryPreset> presets
        {
            {
                "M/S Init", "CLEAN START / TRUE MID-SIDE",
                {
                    { mode, 1.0f }, { inputGain, 0.0f }, { tubeDrive, 26.0f },
                    { threshold, -18.0f }, { ratio, 4.0f }, { timeConstant, 3.0f },
                    { link, 70.0f }, { midTrim, 0.0f }, { sideTrim, 0.0f },
                    { sidechainHpf, 95.0f }, { mix, 100.0f }, { width, 100.0f },
                    { outputGain, 0.0f }, { autoMakeup, 1.0f }
                }
            },
            {
                "670 Legacy Glue", "LINKED STEREO / SLOW VARI-MU",
                {
                    { mode, 0.0f }, { inputGain, 0.0f }, { tubeDrive, 34.0f },
                    { threshold, -16.0f }, { ratio, 4.0f }, { timeConstant, 4.0f },
                    { link, 100.0f }, { midTrim, 0.0f }, { sideTrim, 0.0f },
                    { sidechainHpf, 70.0f }, { mix, 100.0f }, { width, 100.0f },
                    { outputGain, -2.0f }
                }
            },
            {
                "Mid/Side Vocal Air", "OPEN CENTRE / DETAILED SIDES",
                {
                    { mode, 1.0f }, { inputGain, 1.0f }, { tubeDrive, 22.0f },
                    { threshold, -22.0f }, { ratio, 3.5f }, { timeConstant, 2.0f },
                    { link, 55.0f }, { midTrim, 0.0f }, { sideTrim, -3.0f },
                    { sidechainHpf, 120.0f }, { mix, 100.0f }, { width, 112.0f },
                    { outputGain, -1.0f }
                }
            },
            {
                "M/S Drum Room", "HARD SIDES / PUNCHY CENTRE",
                {
                    { mode, 1.0f }, { inputGain, 1.0f }, { tubeDrive, 48.0f },
                    { threshold, -26.0f }, { ratio, 6.0f }, { timeConstant, 1.0f },
                    { link, 40.0f }, { midTrim, -1.0f }, { sideTrim, 4.0f },
                    { sidechainHpf, 140.0f }, { mix, 100.0f }, { width, 122.0f },
                    { outputGain, -3.0f }
                }
            },
            {
                "Sidechain Pump", "INDEPENDENT M/S / LONG TAIL",
                {
                    { mode, 1.0f }, { inputGain, 2.0f }, { tubeDrive, 30.0f },
                    { threshold, -30.0f }, { ratio, 10.0f }, { timeConstant, 5.0f },
                    { link, 15.0f }, { midTrim, -4.0f }, { sideTrim, 6.0f },
                    { sidechainHpf, 160.0f }, { mix, 100.0f }, { width, 135.0f },
                    { outputGain, -3.0f }
                }
            },
            {
                "Parallel Tube Crush", "TRANSFORMER GRIT / FAST",
                {
                    { mode, 0.0f }, { inputGain, 2.0f }, { tubeDrive, 72.0f },
                    { threshold, -30.0f }, { ratio, 8.0f }, { timeConstant, 1.0f },
                    { link, 100.0f }, { midTrim, 0.0f }, { sideTrim, 0.0f },
                    { sidechainHpf, 35.0f }, { mix, 38.0f }, { width, 100.0f },
                    { outputGain, -2.0f }
                }
            },
            {
                "Mix-Bus 670", "GENTLE GLUE / WIDE WINDOW",
                {
                    { mode, 0.0f }, { inputGain, -1.0f }, { tubeDrive, 18.0f },
                    { threshold, -12.0f }, { ratio, 2.5f }, { timeConstant, 4.0f },
                    { link, 100.0f }, { midTrim, 0.0f }, { sideTrim, 0.0f },
                    { sidechainHpf, 60.0f }, { mix, 100.0f }, { width, 100.0f },
                    { outputGain, -0.5f }
                }
            },
            {
                "Mono Bass Anchor", "CENTRE WEIGHT / SLOW RELEASE",
                {
                    { mode, 1.0f }, { inputGain, 1.0f }, { tubeDrive, 36.0f },
                    { threshold, -24.0f }, { ratio, 6.0f }, { timeConstant, 5.0f },
                    { link, 100.0f }, { midTrim, 2.0f }, { sideTrim, -3.0f },
                    { sidechainHpf, 25.0f }, { mix, 100.0f }, { width, 85.0f },
                    { outputGain, -2.0f }
                }
            },
            {
                "Widescreen Strings", "SOFT SIDES / UNTOUCHED CENTRE",
                {
                    { mode, 1.0f }, { inputGain, 0.0f }, { tubeDrive, 14.0f },
                    { threshold, -14.0f }, { ratio, 2.0f }, { timeConstant, 3.0f },
                    { link, 75.0f }, { midTrim, -1.0f }, { sideTrim, 1.5f },
                    { sidechainHpf, 140.0f }, { mix, 100.0f }, { width, 138.0f },
                    { outputGain, 0.0f }
                }
            },
            {
                "Broken Radio", "SATURATED / SQUASHED / SMALL",
                {
                    { mode, 0.0f }, { inputGain, 4.0f }, { tubeDrive, 88.0f },
                    { threshold, -28.0f }, { ratio, 8.0f }, { timeConstant, 2.0f },
                    { link, 100.0f }, { midTrim, 0.0f }, { sideTrim, 0.0f },
                    { sidechainHpf, 200.0f }, { mix, 85.0f }, { width, 90.0f },
                    { outputGain, -4.0f }
                }
            }
        };

        return presets;
    }

    inline int getNumFactoryPresets()
    {
        return static_cast<int> (getFactoryPresets().size());
    }

    inline juce::String getFactoryPresetName (int index)
    {
        const auto& presets = getFactoryPresets();

        if (index < 0 || index >= static_cast<int> (presets.size()))
            return {};

        return presets[static_cast<size_t> (index)].name;
    }

    inline juce::String getFactoryPresetCharacter (int index)
    {
        const auto& presets = getFactoryPresets();

        if (index < 0 || index >= static_cast<int> (presets.size()))
            return {};

        return presets[static_cast<size_t> (index)].character;
    }
} // namespace midside670
