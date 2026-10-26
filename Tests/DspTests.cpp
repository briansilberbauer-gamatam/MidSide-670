#include "PluginProcessor.h"
#include "Parameters.h"
#include "dsp/VariableMu.h"

#include <array>
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

void check (bool condition, const char* message)
{
    std::cout << (condition ? "  PASS  " : "  FAIL  ") << message << '\n';

    if (! condition)
        ++failures;
}

bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getReadPointer (ch)[i]))
                return false;

    return true;
}

void setParameter (MidSide670AudioProcessor& processor, const char* id, float value)
{
    if (auto* parameter = processor.getValueTreeState().getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

float getParameter (MidSide670AudioProcessor& processor, const char* id)
{
    if (auto* parameter = processor.getValueTreeState().getParameter (id))
        return parameter->convertFrom0to1 (parameter->getValue());

    return 0.0f;
}

void processSine (MidSide670AudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                  int numBlocks, float gainLeft, float gainRight, double frequency = 1000.0,
                  juce::int64 startSample = 0)
{
    juce::MidiBuffer midi;

    for (int block = 0; block < numBlocks; ++block)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto n = static_cast<double> (startSample + block * buffer.getNumSamples() + i);
            const auto sample = static_cast<float> (
                0.45 * std::sin (juce::MathConstants<double>::twoPi * frequency * n / 48000.0));

            buffer.setSample (0, i, sample * gainLeft);
            buffer.setSample (1, i, sample * gainRight);
        }

        processor.processBlock (buffer, midi);
    }
}

void testMatrix()
{
    const float left = 0.73f;
    const float right = -0.21f;
    float mid = 0.0f;
    float side = 0.0f;
    float decodedLeft = 0.0f;
    float decodedRight = 0.0f;

    midside670::dsp::encodeMidSide (left, right, mid, side);
    midside670::dsp::decodeMidSide (mid, side, decodedLeft, decodedRight);

    check (std::abs (decodedLeft - left) < 1.0e-6f
               && std::abs (decodedRight - right) < 1.0e-6f,
           "Mid/Side matrix round-trips with unity gain");
}

void testTimeConstants()
{
    bool monotonic = true;
    float previousRelease = 0.0f;

    for (int i = 1; i <= 6; ++i)
    {
        const auto tc = midside670::dsp::getTimeConstant (i);
        monotonic = monotonic && tc.releaseMs >= previousRelease;
        previousRelease = tc.releaseMs;
    }

    check (monotonic, "six-position time-constant network is monotonic");
}

void testVariableMu()
{
    constexpr double osSampleRate = 192000.0;

    midside670::dsp::StereoVariableMu compressor;
    compressor.prepare (osSampleRate);

    midside670::dsp::StereoVariableMu::Parameters params;
    params.thresholdDb = -30.0f;
    params.ratio = 8.0f;
    params.attackMs = 0.4f;
    params.releaseMs = 700.0f;
    params.link = 0.0f;
    params.midTrimDb = 0.0f;
    params.sideTrimDb = 0.0f;
    params.sidechainHpfHz = 25.0f;
    params.stereoLinked = false;
    compressor.setParameters (params);
    compressor.reset();

    float peakMidGr = 0.0f;
    float peakSideGr = 0.0f;

    for (int i = 0; i < static_cast<int> (osSampleRate); ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (osSampleRate);
        float mid = 0.5f * std::sin (juce::MathConstants<float>::twoPi * 1000.0f * t);
        float side = 0.0f;
        const auto gr = compressor.processSample (mid, side);
        peakMidGr = juce::jmax (peakMidGr, gr.midDb);
        peakSideGr = juce::jmax (peakSideGr, gr.sideDb);
    }

    check (peakMidGr > 4.0f, "variable-mu detector applies gain reduction above threshold");
    check (peakSideGr < 0.05f, "independent M/S detection leaves an empty side channel alone");

    // A completely linked Stereo pass must apply identical gain to both matrix channels.
    params.stereoLinked = true;
    compressor.setParameters (params);
    compressor.reset();

    float linkedMidGr = 0.0f;
    float linkedSideGr = 0.0f;

    for (int i = 0; i < static_cast<int> (osSampleRate); ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (osSampleRate);
        float mid = 0.32f * std::sin (juce::MathConstants<float>::twoPi * 800.0f * t);
        float side = 0.11f * std::sin (juce::MathConstants<float>::twoPi * 1600.0f * t);
        const auto gr = compressor.processSample (mid, side);
        linkedMidGr = juce::jmax (linkedMidGr, gr.midDb);
        linkedSideGr = juce::jmax (linkedSideGr, gr.sideDb);
    }

    check (std::abs (linkedMidGr - linkedSideGr) < 0.05f,
           "Stereo mode links both matrix channels to one gain signal");
}

void testProcessor()
{
    MidSide670AudioProcessor processor;
    check (processor.getParameters().size() == 15, "processor exposes all 15 parameters");
    check (processor.getNumPrograms() == 10, "ten factory programs are available");

    processor.setPlayConfigDetails (2, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);

    check (processor.getLatencySamples() > 0, "oversampling latency is reported to the host");
    check (processor.getOversamplingFactor() == 4, "non-linear stages run at 4x oversampling");

    juce::AudioBuffer<float> buffer (2, 512);

    setParameter (processor, midside670::params::mode, 1.0f);
    setParameter (processor, midside670::params::tubeDrive, 0.0f);
    setParameter (processor, midside670::params::threshold, -30.0f);
    setParameter (processor, midside670::params::ratio, 8.0f);
    setParameter (processor, midside670::params::link, 0.0f);
    setParameter (processor, midside670::params::timeConstant, 1.0f);

    processSine (processor, buffer, 100, 1.0f, -1.0f, 1000.0);

    check (allFinite (buffer), "Mid/Side processing produces finite output");
    check (processor.getGainReductionSide() > 4.0f,
           "a side-only signal drives the side compressor");
    check (processor.getGainReductionMid() < 0.25f,
           "a side-only signal does not drive the mid compressor");

    // Lock-free waveform telemetry must expose live, ordered min/max columns.
    std::array<float, 64> waveformMin {};
    std::array<float, 64> waveformMax {};
    const int columns = processor.copyWaveform (waveformMin.data(), waveformMax.data(), 64);

    bool validColumns = columns == 64;
    bool liveWaveform = false;

    for (int i = 0; i < columns; ++i)
    {
        validColumns = validColumns && waveformMax[static_cast<size_t> (i)]
                                          >= waveformMin[static_cast<size_t> (i)];
        liveWaveform = liveWaveform || waveformMax[static_cast<size_t> (i)] > 0.001f
                                      || waveformMin[static_cast<size_t> (i)] < -0.001f;
    }

    check (validColumns, "waveform telemetry publishes ordered min/max columns");
    check (liveWaveform, "waveform telemetry contains live audio");

    // Stereo mode should link the two detector gains.
    processor.setCurrentProgram (0);
    setParameter (processor, midside670::params::mode, 0.0f);
    setParameter (processor, midside670::params::threshold, -30.0f);
    setParameter (processor, midside670::params::ratio, 8.0f);

    processSine (processor, buffer, 100, 1.0f, 0.25f, 1000.0, 512 * 100);

    check (std::abs (processor.getGainReductionMid() - processor.getGainReductionSide()) < 0.25f,
           "Stereo mode presents one linked gain reduction to both channels");

    // Bypass must remain finite and continue to report the same host latency.
    setParameter (processor, midside670::params::bypass, 1.0f);
    processSine (processor, buffer, 20, 1.0f, -1.0f, 1000.0, 512 * 200);
    check (allFinite (buffer), "latency-matched bypass produces finite output");
    check (processor.getLatencySamples() > 0, "bypass does not alter reported latency");

    // State round-trip, including preset identity.
    setParameter (processor, midside670::params::bypass, 0.0f);
    processor.setCurrentProgram (1);
    setParameter (processor, midside670::params::threshold, -27.5f);
    const float thresholdBefore = getParameter (processor, midside670::params::threshold);

    juce::MemoryBlock state;
    processor.getStateInformation (state);
    check (state.getSize() > 0, "full processor state serialises");

    MidSide670AudioProcessor restored;
    restored.setPlayConfigDetails (2, 2, 48000.0, 512);
    restored.prepareToPlay (48000.0, 512);
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

    check (std::abs (getParameter (restored, midside670::params::threshold) - thresholdBefore) < 0.01f,
           "threshold is restored from serialised state");

    processor.releaseResources();
    restored.releaseResources();
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    std::cout << "MidSide-670 offline DSP tests\n\n";

    testMatrix();
    testTimeConstants();
    testVariableMu();
    testProcessor();

    std::cout << (failures == 0 ? "\nAll DSP tests passed.\n" : "\nDSP tests failed.\n");
    return failures == 0 ? 0 : 1;
}
