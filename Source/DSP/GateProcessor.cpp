#include "GateProcessor.h"

void GateProcessor::prepare(double sampleRate, int)
{
    detectorAttackCoeff = std::exp(-1.0f / (detectorAttackSeconds * (float)sampleRate));
    detectorReleaseCoeff = std::exp(-1.0f / (detectorReleaseSeconds * (float)sampleRate));
    gainAttackCoeff = std::exp(-1.0f / (gainAttackSeconds * (float)sampleRate));
    gainReleaseCoeff = std::exp(-1.0f / (gainReleaseSeconds * (float)sampleRate));
    holdSamples = (int)(holdSeconds * sampleRate);
    reset();
}

void GateProcessor::reset()
{
    detector = 0.0f;
    gain = floorGain;
    holdCounter = 0;
    open = false;
}

bool GateProcessor::process(juce::AudioBuffer<float>& buffer, float thresholdDb)
{
    const float openThreshold = juce::Decibels::decibelsToGain(thresholdDb);
    const float closeThreshold = juce::Decibels::decibelsToGain(thresholdDb - hysteresisDb);
    const int numChannels = buffer.getNumChannels();
    const int detectChannels = juce::jmin(numChannels, 2);
    const int numSamples = buffer.getNumSamples();
    auto* const* channels = buffer.getArrayOfWritePointers();

    for (int i = 0; i < numSamples; ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < detectChannels; ++ch)
            peak = juce::jmax(peak, std::abs(channels[ch][i]));

        const float detectorCoeff = peak > detector ? detectorAttackCoeff : detectorReleaseCoeff;
        detector = peak + detectorCoeff * (detector - peak);

        if (detector >= openThreshold)
        {
            open = true;
            holdCounter = holdSamples;
        }
        else if (open)
        {
            if (detector >= closeThreshold)
                holdCounter = holdSamples; // inside the hysteresis band: stay armed
            else if (holdCounter > 0)
                --holdCounter; // below close threshold: hold before releasing
            else
                open = false;
        }

        const float target = open ? 1.0f : floorGain;
        const float gainCoeff = open ? gainAttackCoeff : gainReleaseCoeff;
        gain = target + gainCoeff * (gain - target);

        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][i] *= gain;
    }

    return gain > 0.5f;
}
