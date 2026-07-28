#include "Doubler.h"

void Doubler::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;   // channel 0 = voice L, channel 1 = voice R

    const double maxDelayMs = baseDelayRightMs + modDepthMs + 1.0;
    delayLine.setMaximumDelayInSamples((int)std::ceil(sampleRate * maxDelayMs * 0.001));
    delayLine.prepare(spec);

    const float twoPi = juce::MathConstants<float>::twoPi;
    lfoIncrementLeft = twoPi * lfoRateLeftHz / (float)sampleRate;
    lfoIncrementRight = twoPi * lfoRateRightHz / (float)sampleRate;

    reset();
}

void Doubler::reset()
{
    delayLine.reset();
    lfoPhaseLeft = 0.0f;
    lfoPhaseRight = juce::MathConstants<float>::pi;
}

void Doubler::process(juce::AudioBuffer<float>& buffer, float amount)
{
    if (buffer.getNumChannels() < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    const float msToSamples = (float)(sampleRate * 0.001);
    const float dryGain = 1.0f - amount * dryDuck;
    const float wetGain = amount * voiceGain;
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mono = 0.5f * (left[i] + right[i]);
        delayLine.pushSample(0, mono);
        delayLine.pushSample(1, mono);

        const float delayLeft = (baseDelayLeftMs + modDepthMs * std::sin(lfoPhaseLeft)) * msToSamples;
        const float delayRight = (baseDelayRightMs + modDepthMs * std::sin(lfoPhaseRight)) * msToSamples;
        const float voiceLeft = delayLine.popSample(0, delayLeft);
        const float voiceRight = delayLine.popSample(1, delayRight);

        left[i] = left[i] * dryGain + voiceLeft * wetGain;
        right[i] = right[i] * dryGain + voiceRight * wetGain;

        lfoPhaseLeft += lfoIncrementLeft;
        if (lfoPhaseLeft >= twoPi)
            lfoPhaseLeft -= twoPi;
        lfoPhaseRight += lfoIncrementRight;
        if (lfoPhaseRight >= twoPi)
            lfoPhaseRight -= twoPi;
    }
}
