#include "Compressor.h"

void Compressor::prepare(double sampleRate, int)
{
    attackCoeff = std::exp(-1.0f / (attackSeconds * (float)sampleRate));
    releaseCoeff = std::exp(-1.0f / (releaseSeconds * (float)sampleRate));
    reset();
}

void Compressor::reset()
{
    envelope = 0.0f;
}

void Compressor::process(juce::AudioBuffer<float>& buffer, float thresholdDb, float ratio)
{
    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    auto* const* channels = buffer.getArrayOfWritePointers();
    float minGain = 1.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax(peak, std::abs(channels[ch][i]));

        const float coeff = peak > envelope ? attackCoeff : releaseCoeff;
        envelope = peak + coeff * (envelope - peak);

        float gain = 1.0f;
        if (envelope > thresholdLinear)
        {
            const float dbOver = juce::Decibels::gainToDecibels(envelope / thresholdLinear);
            gain = juce::Decibels::decibelsToGain(-dbOver * (1.0f - 1.0f / ratio));
        }

        minGain = juce::jmin(minGain, gain);

        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= gain;
    }

    gainReductionDb.store(-juce::Decibels::gainToDecibels(minGain));
}
