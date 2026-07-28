#include "Saturator.h"

namespace
{
    float onePoleCoeff(float cutoffHz, double sampleRate)
    {
        return 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoffHz / (float)sampleRate);
    }
}

void Saturator::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;

    tightFilter.prepare(spec);
    tightFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);

    boostFilter.prepare(spec);
    *boostFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, 750.0f, 0.7f, juce::Decibels::decibelsToGain(8.0f));

    toneFilter.prepare(spec);
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    oversampler->initProcessing((size_t)maxBlockSize);

    interstageCoeff = onePoleCoeff(30.0f, sampleRate * 2.0);
    dcCoeff = onePoleCoeff(15.0f, sampleRate);

    reset();
}

void Saturator::reset()
{
    tightFilter.reset();
    boostFilter.reset();
    toneFilter.reset();

    if (oversampler != nullptr)
        oversampler->reset();

    interstageState[0] = interstageState[1] = 0.0f;
    dcState[0] = dcState[1] = 0.0f;
}

int Saturator::getLatencySamples() const
{
    return oversampler != nullptr ? (int)std::ceil(oversampler->getLatencyInSamples()) : 0;
}

void Saturator::process(juce::AudioBuffer<float>& buffer, float gain, float tightHz, float tone, bool boost)
{
    if (oversampler == nullptr)
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    tightFilter.setCutoffFrequency(juce::jlimit(20.0f, 400.0f, tightHz));
    tightFilter.process(context);

    if (boost)
    {
        boostFilter.process(context);
        block.multiplyBy(juce::Decibels::decibelsToGain(boostLevelDb));
    }

    const float driveGain = juce::Decibels::decibelsToGain(minDriveDb + gain * (maxDriveDb - minDriveDb));
    const float biasOffset = std::tanh(stage2Bias);

    auto upsampled = oversampler->processSamplesUp(block);

    for (size_t ch = 0; ch < upsampled.getNumChannels(); ++ch)
    {
        auto* samples = upsampled.getChannelPointer(ch);
        float hpState = interstageState[ch];

        for (size_t i = 0; i < upsampled.getNumSamples(); ++i)
        {
            // Stage 1: main gain stage
            float x = std::tanh(samples[i] * driveGain * 0.5f);

            // Interstage high-pass keeps the low end from turning to mud
            hpState += interstageCoeff * (x - hpState);
            x -= hpState;

            // Stage 2: asymmetric (even harmonics, tube-like feel)
            x = std::tanh(1.8f * x + stage2Bias) - biasOffset;

            // Stage 3: final rounding
            samples[i] = std::tanh(1.3f * x);
        }

        interstageState[ch] = hpState;
    }

    oversampler->processSamplesDown(block);

    // Remove the residual DC offset the asymmetric stage introduces
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        float state = dcState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            state += dcCoeff * (samples[i] - state);
            samples[i] -= state;
        }

        dcState[ch] = state;
    }

    // Tone: 2nd-order low-pass swept exponentially from dark to bright
    toneFilter.setCutoffFrequency(toneMinHz * std::pow(toneMaxHz / toneMinHz, tone));
    toneFilter.process(context);
}
