#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/VariableMu.h"

#include <atomic>
#include <memory>

class MidSide670AudioProcessor final : public juce::AudioProcessor,
                                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    MidSide670AudioProcessor();
    ~MidSide670AudioProcessor() override;

    using juce::AudioProcessor::processBlock;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MidSide-670"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return apvts; }

    void loadFactoryPreset (int index);

    int getCurrentPresetIndex() const noexcept { return currentPreset.load (std::memory_order_relaxed); }
    bool isPresetDirty() const noexcept { return presetDirty.load (std::memory_order_relaxed); }

    float getInputLevel() const noexcept { return inputLevel.load (std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outputLevel.load (std::memory_order_relaxed); }
    float getOutputLevelLeft() const noexcept { return outputLevelLeft.load (std::memory_order_relaxed); }
    float getOutputLevelRight() const noexcept { return outputLevelRight.load (std::memory_order_relaxed); }
    float getGainReductionMid() const noexcept { return gainReductionMid.load (std::memory_order_relaxed); }
    float getGainReductionSide() const noexcept { return gainReductionSide.load (std::memory_order_relaxed); }
    int getOversamplingFactor() const noexcept { return oversamplingFactor; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void applyPreset (int index);

    static constexpr int processingChannels = 2;
    static constexpr int oversamplingLog2 = 2; // 4x

    juce::AudioProcessorValueTreeState apvts;

    juce::dsp::Oversampling<float> oversampler {
        static_cast<size_t> (processingChannels),
        static_cast<size_t> (oversamplingLog2),
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        false,
        true
    };

    midside670::dsp::StereoVariableMu compressor;
    midside670::dsp::TubeStage tubeLeft;
    midside670::dsp::TubeStage tubeRight;

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> widthSmoothed;

    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* tubeDriveParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* timeConstantParam = nullptr;
    std::atomic<float>* linkParam = nullptr;
    std::atomic<float>* midTrimParam = nullptr;
    std::atomic<float>* sideTrimParam = nullptr;
    std::atomic<float>* sidechainHpfParam = nullptr;
    std::atomic<float>* autoMakeupParam = nullptr;

    std::atomic<int> currentPreset { 0 };
    std::atomic<bool> presetDirty { false };
    std::atomic<bool> applyingPreset { false };

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };
    std::atomic<float> gainReductionMid { 0.0f };
    std::atomic<float> gainReductionSide { 0.0f };

    double currentSampleRate = 48000.0;
    int preparedBlockSize = 512;
    int oversamplingFactor = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidSide670AudioProcessor)
};
