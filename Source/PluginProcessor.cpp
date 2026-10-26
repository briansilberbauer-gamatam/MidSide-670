#include "PluginProcessor.h"

#include "Parameters.h"
#include "PluginEditor.h"
#include "Presets.h"

#include <algorithm>
#include <cmath>

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;

std::unique_ptr<juce::AudioParameterFloat> makeFloat (const char* id,
                                                     const char* name,
                                                     juce::NormalisableRange<float> range,
                                                     float defaultValue,
                                                     const char* label)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 },
        name,
        range,
        defaultValue,
        juce::AudioParameterFloatAttributes().withLabel (label));
}

std::unique_ptr<juce::AudioParameterBool> makeBool (const char* id,
                                                    const char* name,
                                                    bool defaultValue)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, defaultValue);
}
} // namespace

MidSide670AudioProcessor::MidSide670AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    inputGainParam = apvts.getRawParameterValue (midside670::params::inputGain);
    tubeDriveParam = apvts.getRawParameterValue (midside670::params::tubeDrive);
    outputGainParam = apvts.getRawParameterValue (midside670::params::outputGain);
    mixParam = apvts.getRawParameterValue (midside670::params::mix);
    widthParam = apvts.getRawParameterValue (midside670::params::width);
    bypassParam = apvts.getRawParameterValue (midside670::params::bypass);
    modeParam = apvts.getRawParameterValue (midside670::params::mode);
    thresholdParam = apvts.getRawParameterValue (midside670::params::threshold);
    ratioParam = apvts.getRawParameterValue (midside670::params::ratio);
    timeConstantParam = apvts.getRawParameterValue (midside670::params::timeConstant);
    linkParam = apvts.getRawParameterValue (midside670::params::link);
    midTrimParam = apvts.getRawParameterValue (midside670::params::midTrim);
    sideTrimParam = apvts.getRawParameterValue (midside670::params::sideTrim);
    sidechainHpfParam = apvts.getRawParameterValue (midside670::params::sidechainHpf);
    autoMakeupParam = apvts.getRawParameterValue (midside670::params::autoMakeup);

    for (const auto* parameterID : midside670::params::allParameterIDs)
        apvts.addParameterListener (parameterID, this);
}

MidSide670AudioProcessor::~MidSide670AudioProcessor()
{
    for (const auto* parameterID : midside670::params::allParameterIDs)
        apvts.removeParameterListener (parameterID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout MidSide670AudioProcessor::createParameterLayout()
{
    using namespace midside670::params;

    APVTS::ParameterLayout layout;

    auto input = std::make_unique<juce::AudioProcessorParameterGroup> ("input", "Input Stage", " | ");
    input->addChild (makeFloat (inputGain, "Input Gain", { -18.0f, 18.0f, 0.01f }, 0.0f, "dB"));
    input->addChild (makeFloat (tubeDrive, "Tube Drive", { 0.0f, 100.0f, 0.1f }, 26.0f, "%"));
    layout.add (std::move (input));

    auto dynamics = std::make_unique<juce::AudioProcessorParameterGroup> ("dynamics", "Variable-Mu", " | ");
    dynamics->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { mode, 1 },
        "Matrix Mode",
        juce::StringArray { "STEREO", "MID/SIDE" },
        midSideMode));
    dynamics->addChild (makeFloat (threshold, "Threshold", { -40.0f, 6.0f, 0.1f }, -18.0f, "dB"));
    dynamics->addChild (makeFloat (ratio, "Ratio", { 1.5f, 10.0f, 0.1f, 0.7f }, 4.0f, ":1"));
    dynamics->addChild (makeFloat (timeConstant, "Time Constant", { 1.0f, 6.0f, 1.0f }, 3.0f, "TC"));
    dynamics->addChild (makeFloat (midside670::params::link, "M/S Link", { 0.0f, 100.0f, 0.1f }, 70.0f, "%"));
    dynamics->addChild (makeFloat (midTrim, "Mid Compression", { -12.0f, 12.0f, 0.1f }, 0.0f, "dB"));
    dynamics->addChild (makeFloat (sideTrim, "Side Compression", { -12.0f, 12.0f, 0.1f }, 0.0f, "dB"));
    dynamics->addChild (makeFloat (sidechainHpf, "Sidechain High-Pass", { 20.0f, 400.0f, 1.0f, 0.55f }, 95.0f, "Hz"));
    layout.add (std::move (dynamics));

    auto output = std::make_unique<juce::AudioProcessorParameterGroup> ("output", "Matrix Output", " | ");
    output->addChild (makeFloat (outputGain, "Output Gain", { -18.0f, 18.0f, 0.01f }, 0.0f, "dB"));
    output->addChild (makeFloat (mix, "Parallel Mix", { 0.0f, 100.0f, 0.1f }, 100.0f, "%"));
    output->addChild (makeFloat (width, "Stereo Width", { 0.0f, 200.0f, 0.1f }, 100.0f, "%"));
    output->addChild (makeBool (autoMakeup, "Automatic Make-up", true));
    output->addChild (makeBool (bypass, "Bypass", false));
    layout.add (std::move (output));

    return layout;
}

void MidSide670AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max (8000.0, sampleRate);
    preparedBlockSize = std::max (1, samplesPerBlock);

    const auto osSampleRate = currentSampleRate * 4.0;

    compressor.prepare (osSampleRate);
    tubeLeft.prepare (osSampleRate);
    tubeRight.prepare (osSampleRate);

    oversampler.initProcessing (static_cast<size_t> (preparedBlockSize));
    oversamplingFactor = static_cast<int> (oversampler.getOversamplingFactor());
    setLatencySamples (juce::roundToInt (oversampler.getLatencyInSamples()));

    dryBuffer.setSize (processingChannels, preparedBlockSize * oversamplingFactor, false, false, true);

    inputGainSmoothed.reset (osSampleRate, 0.020);
    outputGainSmoothed.reset (osSampleRate, 0.020);
    mixSmoothed.reset (osSampleRate, 0.025);
    widthSmoothed.reset (osSampleRate, 0.025);

    inputGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (inputGainParam->load()));
    outputGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (outputGainParam->load()));
    mixSmoothed.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : mixParam->load() * 0.01f);
    widthSmoothed.setCurrentAndTargetValue (widthParam->load() * 0.01f);

    inputLevel.store (0.0f, std::memory_order_relaxed);
    outputLevel.store (0.0f, std::memory_order_relaxed);
    outputLevelLeft.store (0.0f, std::memory_order_relaxed);
    outputLevelRight.store (0.0f, std::memory_order_relaxed);
    gainReductionMid.store (0.0f, std::memory_order_relaxed);
    gainReductionSide.store (0.0f, std::memory_order_relaxed);

    // Keep roughly 750 waveform columns per second at any sample rate, which
    // gives the display a ~0.7 second window at its 512-column view.
    waveformSamplesPerColumn = juce::jmax (8, juce::roundToInt (currentSampleRate / 750.0));
    resetWaveformHistory();
}

void MidSide670AudioProcessor::resetWaveformHistory() noexcept
{
    for (auto& value : waveformMin)
        value.store (0.0f, std::memory_order_relaxed);

    for (auto& value : waveformMax)
        value.store (0.0f, std::memory_order_relaxed);

    waveformWriteCount.store (0, std::memory_order_relaxed);
    waveformAccumulatedSamples = 0;
    waveformAccumulatedMin = 0.0f;
    waveformAccumulatedMax = 0.0f;
}

void MidSide670AudioProcessor::captureWaveform (const juce::AudioBuffer<float>& buffer,
                                                int numSamples) noexcept
{
    const int columnSize = juce::jmax (1, waveformSamplesPerColumn);

    auto sanitise = [] (float value)
    {
        return std::isfinite (value) ? juce::jlimit (-1.0f, 1.0f, value) : 0.0f;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        // Follow the true stereo envelope, so anti-phase and side-only material
        // does not cancel out of the display the way a raw L+R sum would.
        const float left = sanitise (buffer.getSample (0, i));
        const float right = sanitise (buffer.getSample (1, i));
        const float columnMin = juce::jmin (left, right);
        const float columnMax = juce::jmax (left, right);

        if (waveformAccumulatedSamples == 0)
        {
            waveformAccumulatedMin = columnMin;
            waveformAccumulatedMax = columnMax;
        }
        else
        {
            waveformAccumulatedMin = juce::jmin (waveformAccumulatedMin, columnMin);
            waveformAccumulatedMax = juce::jmax (waveformAccumulatedMax, columnMax);
        }

        if (++waveformAccumulatedSamples < columnSize)
            continue;

        const auto writeIndex = waveformWriteCount.load (std::memory_order_relaxed);
        const auto slot = static_cast<size_t> (writeIndex % static_cast<juce::uint32> (waveformCapacity));
        waveformMin[slot].store (waveformAccumulatedMin, std::memory_order_relaxed);
        waveformMax[slot].store (waveformAccumulatedMax, std::memory_order_relaxed);
        waveformWriteCount.store (writeIndex + 1, std::memory_order_release);

        waveformAccumulatedSamples = 0;
    }
}

int MidSide670AudioProcessor::copyWaveform (float* minColumns, float* maxColumns,
                                            int maxColumnCount) const noexcept
{
    if (minColumns == nullptr || maxColumns == nullptr || maxColumnCount <= 0)
        return 0;

    const auto written = waveformWriteCount.load (std::memory_order_acquire);
    const int available = static_cast<int> (
        juce::jmin<juce::uint32> (written, static_cast<juce::uint32> (waveformCapacity)));
    const int count = juce::jmin (available, maxColumnCount);
    const auto start = written - static_cast<juce::uint32> (count);

    for (int i = 0; i < count; ++i)
    {
        const auto slot = static_cast<size_t> ((start + static_cast<juce::uint32> (i))
                                               % static_cast<juce::uint32> (waveformCapacity));
        minColumns[i] = waveformMin[slot].load (std::memory_order_relaxed);
        maxColumns[i] = waveformMax[slot].load (std::memory_order_relaxed);
    }

    return count;
}

void MidSide670AudioProcessor::releaseResources()
{
    compressor.reset();
    tubeLeft.reset();
    tubeRight.reset();
    inputGainSmoothed.reset (currentSampleRate);
    outputGainSmoothed.reset (currentSampleRate);
    mixSmoothed.reset (currentSampleRate);
    widthSmoothed.reset (currentSampleRate);
}

void MidSide670AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    if (numSamples == 0)
        return;

    if (buffer.getNumChannels() < processingChannels || numSamples > preparedBlockSize)
    {
        buffer.clear();
        return;
    }

    for (int ch = processingChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const float inputPeak = buffer.getMagnitude (0, numSamples);

    const juce::dsp::AudioBlock<const float> inputBlock (buffer.getArrayOfReadPointers(),
                                                         processingChannels,
                                                         static_cast<size_t> (numSamples));
    auto oversampledBlock = oversampler.processSamplesUp (inputBlock);

    const int osSamples = static_cast<int> (oversampledBlock.getNumSamples());

    if (osSamples > dryBuffer.getNumSamples())
    {
        // The host supplied a larger block than prepareToPlay announced. This is never
        // expected, but keep the audio thread allocation-free and fail silently.
        buffer.clear();
        return;
    }

    for (int ch = 0; ch < processingChannels; ++ch)
        dryBuffer.copyFrom (ch, 0, oversampledBlock.getChannelPointer (static_cast<size_t> (ch)), osSamples);

    auto* left = oversampledBlock.getChannelPointer (0);
    auto* right = oversampledBlock.getChannelPointer (1);

    const bool bypassed = bypassParam->load() > 0.5f;
    const bool midSide = modeParam->load() > 0.5f;

    const int tcPosition = juce::jlimit (1, 6, juce::roundToInt (timeConstantParam->load()));
    const auto timeConstant = midside670::dsp::getTimeConstant (tcPosition);

    midside670::dsp::StereoVariableMu::Parameters compressorParameters;
    compressorParameters.thresholdDb = thresholdParam->load();
    compressorParameters.ratio = ratioParam->load();
    compressorParameters.attackMs = timeConstant.attackMs;
    compressorParameters.releaseMs = timeConstant.releaseMs;
    compressorParameters.link = linkParam->load() * 0.01f;
    compressorParameters.midTrimDb = midTrimParam->load();
    compressorParameters.sideTrimDb = sideTrimParam->load();
    compressorParameters.sidechainHpfHz = sidechainHpfParam->load();
    compressorParameters.stereoLinked = ! midSide;
    compressor.setParameters (compressorParameters);

    tubeLeft.setDrive (tubeDriveParam->load() * 0.01f);
    tubeRight.setDrive (tubeDriveParam->load() * 0.01f);

    inputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (inputGainParam->load()));

    const float autoMakeupDb = autoMakeupParam->load() > 0.5f
                                   ? juce::jlimit (0.0f, 12.0f, -thresholdParam->load() * 0.25f)
                                   : 0.0f;
    outputGainSmoothed.setTargetValue (
        juce::Decibels::decibelsToGain (outputGainParam->load() + autoMakeupDb));
    mixSmoothed.setTargetValue (bypassed ? 0.0f : mixParam->load() * 0.01f);
    widthSmoothed.setTargetValue (widthParam->load() * 0.01f);

    float peakMidGr = 0.0f;
    float peakSideGr = 0.0f;
    float wetPeak = 0.0f;

    for (int i = 0; i < osSamples; ++i)
    {
        const float inputGain = inputGainSmoothed.getNextValue();
        const float outputGain = outputGainSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();
        const float width = widthSmoothed.getNextValue();

        float l = left[i] * inputGain;
        float r = right[i] * inputGain;

        l = tubeLeft.processSample (l);
        r = tubeRight.processSample (r);

        float mid = 0.0f;
        float side = 0.0f;
        midside670::dsp::encodeMidSide (l, r, mid, side);

        const auto reduction = compressor.processSample (mid, side);
        peakMidGr = juce::jmax (peakMidGr, reduction.midDb);
        peakSideGr = juce::jmax (peakSideGr, reduction.sideDb);

        side *= width;
        midside670::dsp::decodeMidSide (mid, side, l, r);

        l *= outputGain;
        r *= outputGain;

        wetPeak = juce::jmax (wetPeak, juce::jmax (std::abs (l), std::abs (r)));

        const float dryL = dryBuffer.getSample (0, i);
        const float dryR = dryBuffer.getSample (1, i);

        left[i] = dryL + mix * (l - dryL);
        right[i] = dryR + mix * (r - dryR);
    }

    juce::dsp::AudioBlock<float> outputBlock (buffer.getArrayOfWritePointers(),
                                                    processingChannels,
                                                    static_cast<size_t> (numSamples));
    oversampler.processSamplesDown (outputBlock);

    const float outputLeftPeak = buffer.getMagnitude (0, 0, numSamples);
    const float outputRightPeak = buffer.getMagnitude (1, 0, numSamples);
    const float outputPeak = juce::jmax (outputLeftPeak, outputRightPeak);

    if (bypassed)
    {
        peakMidGr = 0.0f;
        peakSideGr = 0.0f;
    }

    inputLevel.store (std::isfinite (inputPeak) ? inputPeak : 0.0f, std::memory_order_relaxed);
    outputLevel.store (std::isfinite (outputPeak) ? outputPeak : wetPeak, std::memory_order_relaxed);
    outputLevelLeft.store (std::isfinite (outputLeftPeak) ? outputLeftPeak : 0.0f, std::memory_order_relaxed);
    outputLevelRight.store (std::isfinite (outputRightPeak) ? outputRightPeak : 0.0f, std::memory_order_relaxed);
    gainReductionMid.store (std::isfinite (peakMidGr) ? peakMidGr : 0.0f, std::memory_order_relaxed);
    gainReductionSide.store (std::isfinite (peakSideGr) ? peakSideGr : 0.0f, std::memory_order_relaxed);

    captureWaveform (buffer, numSamples);
}

juce::AudioProcessorEditor* MidSide670AudioProcessor::createEditor()
{
    return new MidSide670AudioProcessorEditor (*this);
}

int MidSide670AudioProcessor::getNumPrograms()
{
    return midside670::getNumFactoryPresets();
}

int MidSide670AudioProcessor::getCurrentProgram()
{
    return currentPreset.load (std::memory_order_relaxed);
}

void MidSide670AudioProcessor::setCurrentProgram (int index)
{
    loadFactoryPreset (index);
}

const juce::String MidSide670AudioProcessor::getProgramName (int index)
{
    return midside670::getFactoryPresetName (index);
}

void MidSide670AudioProcessor::changeProgramName (int, const juce::String&)
{
}

void MidSide670AudioProcessor::loadFactoryPreset (int index)
{
    applyPreset (index);
}

void MidSide670AudioProcessor::applyPreset (int index)
{
    const auto& presets = midside670::getFactoryPresets();

    if (index < 0 || index >= static_cast<int> (presets.size()))
        return;

    applyingPreset.store (true, std::memory_order_relaxed);

    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& value : presets[static_cast<size_t> (index)].overrides)
    {
        if (auto* parameter = apvts.getParameter (value.id))
        {
            const auto& range = parameter->getNormalisableRange();
            const float deNormalised = juce::jlimit (range.start, range.end, value.value);
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (deNormalised));
        }
    }

    currentPreset.store (index, std::memory_order_relaxed);
    presetDirty.store (false, std::memory_order_relaxed);
    applyingPreset.store (false, std::memory_order_relaxed);
}

void MidSide670AudioProcessor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load (std::memory_order_relaxed))
        presetDirty.store (true, std::memory_order_relaxed);
}

void MidSide670AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPreset.load (std::memory_order_relaxed), nullptr);
    state.setProperty ("presetDirty", presetDirty.load (std::memory_order_relaxed), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MidSide670AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);

    if (! tree.isValid())
        return;

    const int presetIndex = static_cast<int> (tree.getProperty ("presetIndex", 0));
    const bool dirty = static_cast<bool> (tree.getProperty ("presetDirty", false));

    applyingPreset.store (true, std::memory_order_relaxed);
    apvts.replaceState (tree);
    applyingPreset.store (false, std::memory_order_relaxed);

    currentPreset.store (juce::jlimit (0, midside670::getNumFactoryPresets() - 1, presetIndex),
                         std::memory_order_relaxed);
    presetDirty.store (dirty, std::memory_order_relaxed);
}

bool MidSide670AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidSide670AudioProcessor();
}
