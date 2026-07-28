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
    dampingCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi
                                   * feedbackDampingHz / (float)sampleRate);
    initialised = false;
    reset();
}

void StereoDelay::reset()
{
    delayLine.reset();
    dampingState[0] = dampingState[1] = 0.0f;
    initialised = false;
}

void StereoDelay::process(juce::AudioBuffer<float>& buffer, float timeMs, float feedback, float mix)
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

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        const float delay = delaySamples.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            const float delayed = delayLine.popSample(ch, delay);

            // Low-pass in the feedback loop so repeats darken like tape
            dampingState[ch] += dampingCoeff * (delayed - dampingState[ch]);
            delayLine.pushSample(ch, samples[i] + dampingState[ch] * feedback);

            samples[i] += delayed * mix;
        }
    }
}
