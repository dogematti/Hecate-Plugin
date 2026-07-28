#include "Octaver.h"

#include <cmath>

void Octaver::prepare(double sampleRate, int)
{
    windowSamples = std::max(64, (int)(sampleRate * windowSeconds));
    bufferSize = windowSamples * 2;

    for (auto& buffer : delayBuffer)
        buffer.assign((size_t)bufferSize, 0.0f);

    lowpassCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi
                                   * wetLowpassHz / (float)sampleRate);
    reset();
}

void Octaver::reset()
{
    for (auto& buffer : delayBuffer)
        std::fill(buffer.begin(), buffer.end(), 0.0f);

    lowpassState[0] = lowpassState[1] = 0.0f;
    writeIndex = 0;
    phase = 0.0f;
}

float Octaver::readTap(int channel, float delaySamples) const
{
    float position = (float)writeIndex - delaySamples;
    if (position < 0.0f)
        position += (float)bufferSize;

    const int index0 = (int)position;
    const int index1 = (index0 + 1) % bufferSize;
    const float frac = position - (float)index0;
    const auto& buffer = delayBuffer[channel];

    return buffer[(size_t)index0] + frac * (buffer[(size_t)index1] - buffer[(size_t)index0]);
}

void Octaver::process(juce::AudioBuffer<float>& buffer, float mix)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // Reading at half the write speed halves the pitch; the read head slips
    // one window per cycle, so a second tap half a window behind crossfades
    // over the discontinuity.
    const float phaseIncrement = 0.5f / (float)windowSamples;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            delayBuffer[ch][(size_t)writeIndex] = buffer.getReadPointer(ch)[i];

        float phase2 = phase + 0.5f;
        if (phase2 >= 1.0f)
            phase2 -= 1.0f;

        const float delay1 = phase * (float)windowSamples;
        const float delay2 = phase2 * (float)windowSamples;
        const float gain1 = 1.0f - std::abs(2.0f * phase - 1.0f);
        const float gain2 = 1.0f - gain1;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float wet = readTap(ch, delay1) * gain1 + readTap(ch, delay2) * gain2;

            lowpassState[ch] += lowpassCoeff * (wet - lowpassState[ch]);

            auto* samples = buffer.getWritePointer(ch);
            samples[i] = samples[i] * (1.0f - mix) + lowpassState[ch] * mix;
        }

        if (++writeIndex >= bufferSize)
            writeIndex = 0;

        phase += phaseIncrement;
        if (phase >= 1.0f)
            phase -= 1.0f;
    }
}
