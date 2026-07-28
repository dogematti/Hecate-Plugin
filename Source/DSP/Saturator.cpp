#include "Saturator.h"

void Saturator::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;

    tightFilter.prepare(spec);
    tightFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    tightFilter.setCutoffFrequency(tightHz);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    oversampler->initProcessing((size_t)maxBlockSize);

    reset();
}

void Saturator::reset()
{
    tightFilter.reset();

    if (oversampler != nullptr)
        oversampler->reset();

    toneState[0] = toneState[1] = 0.0f;
}

int Saturator::getLatencySamples() const
{
    return oversampler != nullptr ? (int)std::ceil(oversampler->getLatencyInSamples()) : 0;
}

void Saturator::process(juce::AudioBuffer<float>& buffer, float drive, float tone)
{
    if (oversampler == nullptr)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> tightContext(block);
    tightFilter.process(tightContext);

    const float gain = juce::Decibels::decibelsToGain(drive * maxDriveDb);

    auto upsampled = oversampler->processSamplesUp(block);

    for (size_t ch = 0; ch < upsampled.getNumChannels(); ++ch)
    {
        auto* samples = upsampled.getChannelPointer(ch);

        for (size_t i = 0; i < upsampled.getNumSamples(); ++i)
            samples[i] = std::tanh(samples[i] * gain);
    }

    oversampler->processSamplesDown(block);

    // Tone: one-pole low-pass, cutoff swept exponentially from dark to bright
    const float cutoff = toneMinHz * std::pow(toneMaxHz / toneMinHz, tone);
    const float coeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / (float)sampleRate);

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        float state = toneState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            state += coeff * (samples[i] - state);
            samples[i] = state;
        }

        toneState[ch] = state;
    }
}
