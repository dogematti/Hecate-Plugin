#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Freeverb-style reverb: 8 damped comb filters in parallel into 4 allpass
// filters in series, one full bank per channel. The right bank's delays are
// offset by a fixed spread so the tail decorrelates, and the width control
// cross-mixes the two wet signals.
class StereoReverb
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // roomSize, width, wet, dry all 0..1
    void process(juce::AudioBuffer<float>& buffer, float roomSize, float width, float wet, float dry);

private:
    class CombFilter
    {
    public:
        void prepare(int delaySamples);
        void reset();

        float process(float input, float feedback, float damping)
        {
            const float output = buffer[(size_t)index];
            filterStore = output * (1.0f - damping) + filterStore * damping;
            buffer[(size_t)index] = input + filterStore * feedback;
            if (++index >= (int)buffer.size())
                index = 0;
            return output;
        }

    private:
        std::vector<float> buffer;
        float filterStore = 0.0f;
        int index = 0;
    };

    class AllpassFilter
    {
    public:
        void prepare(int delaySamples);
        void reset();

        float process(float input)
        {
            const float delayed = buffer[(size_t)index];
            buffer[(size_t)index] = input + delayed * 0.5f;
            if (++index >= (int)buffer.size())
                index = 0;
            return delayed - input;
        }

    private:
        std::vector<float> buffer;
        int index = 0;
    };

    // Freeverb tunings, in samples at 44.1 kHz
    static constexpr int combTunings[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static constexpr int allpassTunings[] = {556, 441, 341, 225};
    static constexpr int stereoSpread = 23;
    static constexpr float inputGain = 0.015f;
    static constexpr float damping = 0.25f;

    std::vector<CombFilter> combsLeft, combsRight;
    std::vector<AllpassFilter> allpassesLeft, allpassesRight;
};
