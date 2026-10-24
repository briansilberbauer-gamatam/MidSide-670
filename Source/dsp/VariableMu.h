#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

#include <algorithm>
#include <cmath>

namespace midside670::dsp
{
    //==================================================================================
    // Fairchild-style six-position time-constant network. The numbers are intentionally
    // musical rather than exact: position 1 is the fastest "peak" setting and position 6
    // is the long, program-dependent "level" setting.
    //==================================================================================
    struct TimeConstant
    {
        float attackMs;
        float releaseMs;
        const char* character;
    };

    inline TimeConstant getTimeConstant (int position) noexcept
    {
        static constexpr TimeConstant table[] =
        {
            { 0.20f,   250.0f, "FAST" },
            { 0.20f,   800.0f, "TIGHT" },
            { 0.40f,  2000.0f, "GLUE" },
            { 4.00f,  2500.0f, "SMOOTH" },
            { 10.0f,  5000.0f, "SLOW" },
            { 20.0f, 10000.0f, "LEVEL" }
        };

        const auto index = juce::jlimit (1, 6, position) - 1;
        return table[index];
    }

    inline void encodeMidSide (float left, float right, float& mid, float& side) noexcept
    {
        mid = (left + right) * 0.5f;
        side = (left - right) * 0.5f;
    }

    inline void decodeMidSide (float mid, float side, float& left, float& right) noexcept
    {
        left = mid + side;
        right = mid - side;
    }

    //==================================================================================
    // Asymmetric triode/transformer waveshaper. The bias term creates the even-order
    // content that makes a 670 feel thick; the output DC blocker removes the resulting
    // offset. Input is deliberately unity at low levels.
    //==================================================================================
    class TubeStage
    {
    public:
        void prepare (double newSampleRate) noexcept
        {
            sampleRate = std::max (8000.0, newSampleRate);
            dcCoefficient = static_cast<float> (std::exp (-juce::MathConstants<double>::twoPi * 8.0 / sampleRate));
            reset();
        }

        void reset() noexcept
        {
            dcX1 = dcY1 = 0.0f;
            smoothedDrive = targetDrive;
        }

        void setDrive (float newDrive) noexcept
        {
            targetDrive = juce::jlimit (0.0f, 1.0f, newDrive);
        }

        float processSample (float x) noexcept
        {
            constexpr float driveSmoothing = 0.0025f;
            smoothedDrive += driveSmoothing * (targetDrive - smoothedDrive);

            const float drive = smoothedDrive;
            const float preGain = 1.0f + 2.6f * drive;
            const float bias = 0.11f * drive;
            const float biasRemoval = std::tanh (bias);

            // Asymmetric soft saturation, normalised for small-signal unity.
            float y = (std::tanh (x * preGain + bias) - biasRemoval) / preGain;

            // A second, gentler push mimics the output transformer core.
            const float outputGain = 1.0f + 1.15f * drive;
            y = std::tanh (y * outputGain) / outputGain;

            // 8 Hz DC blocker; the asymmetric stage can otherwise leave an offset.
            const float filtered = y - dcX1 + dcCoefficient * dcY1;
            dcX1 = y;
            dcY1 = filtered;

            return filtered;
        }

    private:
        double sampleRate = 48000.0;
        float dcCoefficient = 0.999f;
        float dcX1 = 0.0f;
        float dcY1 = 0.0f;
        float targetDrive = 0.0f;
        float smoothedDrive = 0.0f;
    };

    //==================================================================================
    // Two-channel variable-mu gain computer. The detector runs in the Mid/Side domain;
    // Stereo mode simply forces a 100 % linked detector so the same gain is applied to
    // both matrix channels (which is mathematically a linked L/R compressor).
    //==================================================================================
    class StereoVariableMu
    {
    public:
        struct Parameters
        {
            float thresholdDb = -18.0f;
            float ratio = 4.0f;
            float attackMs = 0.4f;
            float releaseMs = 2000.0f;
            float link = 0.7f;
            float midTrimDb = 0.0f;
            float sideTrimDb = 0.0f;
            float sidechainHpfHz = 95.0f;
            bool stereoLinked = false;
        };

        void prepare (double newSampleRate) noexcept
        {
            sampleRate = std::max (8000.0, newSampleRate);

            const auto modeSmoothingSeconds = 0.030;
            parameterSmoothing = static_cast<float> (1.0 - std::exp (-1.0 / (modeSmoothingSeconds * sampleRate)));
            gainSmoothing = static_cast<float> (1.0 - std::exp (-1.0 / (0.00012 * sampleRate)));
            hpfSmoothing = static_cast<float> (1.0 - std::exp (-1.0 / (0.040 * sampleRate)));

            reset();
        }

        void reset() noexcept
        {
            for (auto& ch : channels)
            {
                ch.envFast = 0.0f;
                ch.envSlow = 0.0f;
                ch.gain = 1.0f;
                ch.hpfLp1 = 0.0f;
                ch.hpfLp2 = 0.0f;
                ch.grDb = 0.0f;
            }

            smoothedThreshold = targetThreshold;
            smoothedRatio = targetRatio;
            smoothedLink = targetLink;
            smoothedMidTrim = targetMidTrim;
            smoothedSideTrim = targetSideTrim;
            smoothedStereo = targetStereo;
            smoothedHpfHz = targetHpfHz;
            targetHpfCoefficient = hpfCoefficientFor (targetHpfHz);
            smoothedHpfCoefficient = targetHpfCoefficient;
            hpfCoefficient = smoothedHpfCoefficient;
        }

        void setParameters (const Parameters& newParameters) noexcept
        {
            targetThreshold = newParameters.thresholdDb;
            targetRatio = juce::jmax (1.05f, newParameters.ratio);
            targetLink = juce::jlimit (0.0f, 1.0f, newParameters.link);
            targetMidTrim = newParameters.midTrimDb;
            targetSideTrim = newParameters.sideTrimDb;
            targetHpfHz = juce::jlimit (20.0f, 400.0f, newParameters.sidechainHpfHz);
            targetStereo = newParameters.stereoLinked ? 1.0f : 0.0f;
            targetHpfCoefficient = hpfCoefficientFor (targetHpfHz);

            attackCoefficient = timeToCoefficient (newParameters.attackMs * 0.001f);
            releaseFastCoefficient = timeToCoefficient (juce::jmax (0.03f, newParameters.releaseMs * 0.00045f));
            releaseSlowCoefficient = timeToCoefficient (juce::jmin (20.0f, newParameters.releaseMs * 0.0024f));
        }

        struct GainReduction
        {
            float midDb = 0.0f;
            float sideDb = 0.0f;
        };

        GainReduction processSample (float& mid, float& side) noexcept
        {
            smoothedStereo += parameterSmoothing * (targetStereo - smoothedStereo);
            smoothedThreshold += parameterSmoothing * (targetThreshold - smoothedThreshold);
            smoothedRatio += parameterSmoothing * (targetRatio - smoothedRatio);
            smoothedLink += parameterSmoothing * (targetLink - smoothedLink);
            smoothedMidTrim += parameterSmoothing * (targetMidTrim - smoothedMidTrim);
            smoothedSideTrim += parameterSmoothing * (targetSideTrim - smoothedSideTrim);
            smoothedHpfHz += hpfSmoothing * (targetHpfHz - smoothedHpfHz);
            smoothedHpfCoefficient += hpfSmoothing * (targetHpfCoefficient - smoothedHpfCoefficient);
            hpfCoefficient = smoothedHpfCoefficient;

            const float filteredMid = processHighPass (channels[0], mid);
            const float filteredSide = processHighPass (channels[1], side);

            const float levelMid = processEnvelope (channels[0], filteredMid);
            const float levelSide = processEnvelope (channels[1], filteredSide);

            // 0 = independent M/S, 1 = fully linked. Stereo mode locks this to 1.
            const float link = juce::jlimit (0.0f, 1.0f,
                                             smoothedStereo + (1.0f - smoothedStereo) * smoothedLink);
            const float commonLevel = juce::jmax (levelMid, levelSide);

            const float detectorMid = levelMid + link * (commonLevel - levelMid);
            const float detectorSide = levelSide + link * (commonLevel - levelSide);

            // Trims are a Mid/Side feature only. In Stereo mode both trims ramp to zero,
            // so the two matrix channels always receive identical gain.
            const float midTrim = smoothedMidTrim * (1.0f - smoothedStereo);
            const float sideTrim = smoothedSideTrim * (1.0f - smoothedStereo);

            // Positive compression trims lower the channel threshold (more gain
            // reduction); negative trims raise it (less gain reduction).
            const float midThreshold = smoothedThreshold - midTrim;
            const float sideThreshold = smoothedThreshold - sideTrim;

            const float midTarget = gainForLevel (detectorMid, midThreshold, smoothedRatio);
            const float sideTarget = gainForLevel (detectorSide, sideThreshold, smoothedRatio);

            auto& midChannel = channels[0];
            auto& sideChannel = channels[1];

            midChannel.gain += gainSmoothing * (midTarget - midChannel.gain);
            sideChannel.gain += gainSmoothing * (sideTarget - sideChannel.gain);

            mid *= midChannel.gain;
            side *= sideChannel.gain;

            midChannel.grDb = -juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, midChannel.gain), -120.0f);
            sideChannel.grDb = -juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, sideChannel.gain), -120.0f);

            return { midChannel.grDb, sideChannel.grDb };
        }

    private:
        struct ChannelState
        {
            float envFast = 0.0f;
            float envSlow = 0.0f;
            float gain = 1.0f;
            float hpfLp1 = 0.0f;
            float hpfLp2 = 0.0f;
            float grDb = 0.0f;
        };

        float timeToCoefficient (float seconds) const noexcept
        {
            const float clamped = juce::jmax (0.00001f, seconds);
            return static_cast<float> (std::exp (-1.0 / (clamped * sampleRate)));
        }

        static float gainForLevel (float level, float thresholdDb, float ratio) noexcept
        {
            constexpr float kneeDb = 6.0f;

            const float levelDb = juce::Decibels::gainToDecibels (juce::jmax (level, 1.0e-9f), -120.0f);
            const float over = levelDb - thresholdDb;

            if (over <= -kneeDb * 0.5f)
                return 1.0f;

            float slope = 1.0f - 1.0f / juce::jmax (1.05f, ratio);

            // Remote-cutoff "variable mu": the ratio increases gently as the tube is
            // driven harder, giving the characteristic level-dependent curve.
            if (over > 0.0f)
                slope *= 0.88f + 0.24f * (1.0f - std::exp (-over / 12.0f));

            float reductionDb;

            if (over < kneeDb * 0.5f)
            {
                const float t = over + kneeDb * 0.5f;
                reductionDb = slope * t * t / (2.0f * kneeDb);
            }
            else
            {
                reductionDb = slope * over;
            }

            reductionDb = juce::jlimit (0.0f, 36.0f, reductionDb);
            return juce::Decibels::decibelsToGain (-reductionDb);
        }

        float processEnvelope (ChannelState& channel, float x) noexcept
        {
            const float rectified = std::abs (x);

            channel.envFast = rectified > channel.envFast
                                  ? attackCoefficient * channel.envFast + (1.0f - attackCoefficient) * rectified
                                  : releaseFastCoefficient * channel.envFast + (1.0f - releaseFastCoefficient) * rectified;

            channel.envSlow = rectified > channel.envSlow
                                  ? attackCoefficient * channel.envSlow + (1.0f - attackCoefficient) * rectified
                                  : releaseSlowCoefficient * channel.envSlow + (1.0f - releaseSlowCoefficient) * rectified;

            // The slow envelope holds the tail after a loud passage; taking the larger
            // of the two envelopes makes release program-dependent.
            return juce::jmax (channel.envFast, channel.envSlow * 0.92f);
        }

        float processHighPass (ChannelState& channel, float x) noexcept
        {
            channel.hpfLp1 += hpfCoefficient * (x - channel.hpfLp1);
            const float firstStage = x - channel.hpfLp1;

            channel.hpfLp2 += hpfCoefficient * (firstStage - channel.hpfLp2);
            return firstStage - channel.hpfLp2;
        }

        float hpfCoefficientFor (float hz) const noexcept
        {
            const float coefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi
                                                       * juce::jmax (20.0f, hz) / static_cast<float> (sampleRate));
            return juce::jlimit (0.00001f, 0.45f, coefficient);
        }

        double sampleRate = 48000.0;
        ChannelState channels[2];

        float attackCoefficient = 0.01f;
        float releaseFastCoefficient = 0.999f;
        float releaseSlowCoefficient = 0.9999f;
        float parameterSmoothing = 0.001f;
        float gainSmoothing = 0.01f;
        float hpfSmoothing = 0.001f;
        float hpfCoefficient = 0.001f;
        float targetHpfCoefficient = 0.001f;
        float smoothedHpfCoefficient = 0.001f;

        float targetThreshold = -18.0f;
        float targetRatio = 4.0f;
        float targetLink = 0.7f;
        float targetMidTrim = 0.0f;
        float targetSideTrim = 0.0f;
        float targetStereo = 0.0f;
        float targetHpfHz = 95.0f;

        float smoothedThreshold = -18.0f;
        float smoothedRatio = 4.0f;
        float smoothedLink = 0.7f;
        float smoothedMidTrim = 0.0f;
        float smoothedSideTrim = 0.0f;
        float smoothedStereo = 0.0f;
        float smoothedHpfHz = 95.0f;
    };
} // namespace midside670::dsp
