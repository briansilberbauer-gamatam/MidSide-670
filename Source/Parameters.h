#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>

namespace midside670::params
{
    inline constexpr const char* inputGain      = "inputGain";
    inline constexpr const char* tubeDrive      = "tubeDrive";
    inline constexpr const char* outputGain     = "outputGain";
    inline constexpr const char* mix            = "mix";
    inline constexpr const char* width          = "width";
    inline constexpr const char* bypass         = "bypass";

    inline constexpr const char* mode           = "mode";
    inline constexpr const char* threshold      = "threshold";
    inline constexpr const char* ratio          = "ratio";
    inline constexpr const char* timeConstant   = "timeConstant";
    inline constexpr const char* link           = "link";
    inline constexpr const char* midTrim        = "midTrim";
    inline constexpr const char* sideTrim       = "sideTrim";
    inline constexpr const char* sidechainHpf   = "sidechainHpf";
    inline constexpr const char* autoMakeup     = "autoMakeup";

    inline constexpr std::array<const char*, 15> allParameterIDs
    {
        inputGain, tubeDrive, outputGain, mix, width, bypass,
        mode, threshold, ratio, timeConstant, link, midTrim, sideTrim,
        sidechainHpf, autoMakeup
    };

    inline constexpr int stereoMode = 0;
    inline constexpr int midSideMode = 1;
} // namespace midside670::params
