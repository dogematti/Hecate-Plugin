#include "CabinetSimulator.h"

void CabinetSimulator::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = (juce::uint32)numChannels;
    convolution.prepare(spec);

    // Rebuild the built-in IR at the current sample rate, or re-load the
    // user's file (Convolution resamples files internally)
    if (userLoaded.load() && userFile.existsAsFile())
        loadImpulseResponse(userFile);
    else
        loadDefaultCabinet();
}

void CabinetSimulator::reset()
{
    convolution.reset();
}

void CabinetSimulator::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);
}

void CabinetSimulator::loadImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    convolution.loadImpulseResponse(file,
                                    juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::yes,
                                    0,
                                    juce::dsp::Convolution::Normalise::yes);
    name = file.getFileName();
    userFile = file;
    userLoaded.store(true);
}

void CabinetSimulator::clear()
{
    userLoaded.store(false);
    userFile = juce::File();
    loadDefaultCabinet();
}

// Approximates a miked 4x12: steep low cut ~78 Hz, speaker roll-off from
// ~5 kHz, a low thump at 120 Hz, an upper-mid scoop and a presence peak.
// Built by running a unit impulse through IIR filters and windowing the tail.
void CabinetSimulator::loadDefaultCabinet()
{
    const int length = juce::jmax(512, (int)(sampleRate * 0.06));
    juce::AudioBuffer<float> ir(2, length);
    ir.clear();
    ir.setSample(0, 0, 1.0f);

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    const std::vector<juce::dsp::IIR::Coefficients<float>::Ptr> stages = {
        Coefficients::makeHighPass(sampleRate, 78.0f, 0.9f),
        Coefficients::makeLowPass(sampleRate, 4800.0f, 0.9f),
        Coefficients::makeLowPass(sampleRate, 6200.0f, 0.7f),
        Coefficients::makePeakFilter(sampleRate, 120.0f, 1.2f, juce::Decibels::decibelsToGain(3.0f)),
        Coefficients::makePeakFilter(sampleRate, 900.0f, 1.4f, juce::Decibels::decibelsToGain(-4.0f)),
        Coefficients::makePeakFilter(sampleRate, 3300.0f, 1.4f, juce::Decibels::decibelsToGain(4.0f)),
    };

    auto* samples = ir.getWritePointer(0);
    for (const auto& coeffs : stages)
    {
        juce::dsp::IIR::Filter<float> filter(coeffs);
        filter.reset();
        for (int i = 0; i < length; ++i)
            samples[i] = filter.processSample(samples[i]);
    }

    // Fade the tail so the IIR truncation doesn't click
    const int fadeStart = length / 4;
    for (int i = fadeStart; i < length; ++i)
    {
        const float t = (float)(i - fadeStart) / (float)(length - fadeStart);
        samples[i] *= std::exp(-5.0f * t);
    }

    ir.copyFrom(1, 0, ir, 0, 0, length);

    convolution.loadImpulseResponse(std::move(ir), sampleRate,
                                    juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::no,
                                    juce::dsp::Convolution::Normalise::yes);
    name = "Built-in 4x12";
}
