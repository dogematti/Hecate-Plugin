#include "PowerAmp.h"

void PowerAmp::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;

    presenceFilter.prepare(spec);
    depthFilter.prepare(spec);

    attackCoeff = std::exp(-1.0f / (sagAttackSeconds * (float)sampleRate));
    releaseCoeff = std::exp(-1.0f / (sagReleaseSeconds * (float)sampleRate));

    sagGains.setSize(1, maxBlockSize);

    lastPresenceDb = 1e9f;
    lastDepthDb = 1e9f;
    reset();
}

void PowerAmp::reset()
{
    presenceFilter.reset();
    depthFilter.reset();
    envelope = 0.0f;
}

void PowerAmp::process(juce::AudioBuffer<float>& buffer, float sag, float presenceDb, float depthDb)
{
    // Rebuild coefficients only when the knob actually moves
    if (presenceDb != lastPresenceDb)
    {
        *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, presenceHz, presenceQ, juce::Decibels::decibelsToGain(presenceDb));
        lastPresenceDb = presenceDb;
    }

    if (depthDb != lastDepthDb)
    {
        *depthFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, depthHz, depthQ, juce::Decibels::decibelsToGain(depthDb));
        lastDepthDb = depthDb;
    }

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // Sag envelope follows the raw input peak; the dip is applied post-filter
    auto* gains = sagGains.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax(peak, std::abs(buffer.getReadPointer(ch)[i]));

        const float coeff = peak > envelope ? attackCoeff : releaseCoeff;
        envelope = peak + coeff * (envelope - peak);
        gains[i] = juce::Decibels::decibelsToGain(maxSagDb * sag * juce::jmin(envelope, 1.0f));
    }

    juce::dsp::AudioBlock<float> block(buffer);
    auto stereoBlock = block.getSubsetChannelBlock(0, (size_t)numChannels);
    juce::dsp::ProcessContextReplacing<float> context(stereoBlock);
    presenceFilter.process(context);
    depthFilter.process(context);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            samples[i] = std::tanh(samples[i] * driveGain) / driveGain * gains[i];
    }
}
