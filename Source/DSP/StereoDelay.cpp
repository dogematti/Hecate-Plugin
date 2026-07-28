#include "StereoDelay.h"

void StereoDelay::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;

    delayLine.setMaximumDelayInSamples((int)(sampleRate * 1.2));
    delayLine.prepare(spec);

    delaySamples.reset(sampleRate, 0.1);
    initialised = false;
    reset();
}

void StereoDelay::reset()
{
    delayLine.reset();
    dampingState[0] = dampingState[1] = 0.0f;
    initialised = false;
}

void StereoDelay::process(juce::AudioBuffer<float>& buffer, float timeMs, float feedback,
                          float dampingHz, bool pingPong, float mix)
{
    const float targetSamples = (float)(timeMs * 0.001 * sampleRate);

    if (!initialised)
    {
        delaySamples.setCurrentAndTargetValue(targetSamples);
        initialised = true;
    }
    else
    {
        delaySamples.setTargetValue(targetSamples);
    }

    dampingCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi
                                   * juce::jlimit(500.0f, 15000.0f, dampingHz) / (float)sampleRate);

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    const bool crossFeed = pingPong && numChannels >= 2;

    for (int i = 0; i < numSamples; ++i)
    {
        const float delay = delaySamples.getNextValue();

        float delayed[2] = {0.0f, 0.0f};
        for (int ch = 0; ch < numChannels; ++ch)
        {
            delayed[ch] = delayLine.popSample(ch, delay);
            // Low-pass in the feedback loop so repeats darken like tape
            dampingState[ch] += dampingCoeff * (delayed[ch] - dampingState[ch]);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            // Ping-pong: each channel's repeats feed the opposite side
            const float feedbackSource = crossFeed ? dampingState[1 - ch] : dampingState[ch];
            delayLine.pushSample(ch, samples[i] + feedbackSource * feedback);
            samples[i] += delayed[ch] * mix;
        }
    }
}
