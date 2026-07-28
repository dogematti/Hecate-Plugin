#include "StereoReverb.h"

#include <cmath>

void StereoReverb::CombFilter::prepare(int delaySamples)
{
    buffer.assign((size_t)std::max(1, delaySamples), 0.0f);
    filterStore = 0.0f;
    index = 0;
}

void StereoReverb::CombFilter::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    filterStore = 0.0f;
    index = 0;
}

void StereoReverb::AllpassFilter::prepare(int delaySamples)
{
    buffer.assign((size_t)std::max(1, delaySamples), 0.0f);
    index = 0;
}

void StereoReverb::AllpassFilter::reset()
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    index = 0;
}

void StereoReverb::prepare(double sampleRate, int)
{
    const double scale = sampleRate / 44100.0;

    combsLeft.clear();
    combsRight.clear();
    allpassesLeft.clear();
    allpassesRight.clear();

    for (int tuning : combTunings)
    {
        combsLeft.emplace_back();
        combsLeft.back().prepare((int)std::lround(tuning * scale));
        combsRight.emplace_back();
        combsRight.back().prepare((int)std::lround((tuning + stereoSpread) * scale));
    }

    for (int tuning : allpassTunings)
    {
        allpassesLeft.emplace_back();
        allpassesLeft.back().prepare((int)std::lround(tuning * scale));
        allpassesRight.emplace_back();
        allpassesRight.back().prepare((int)std::lround((tuning + stereoSpread) * scale));
    }
}

void StereoReverb::reset()
{
    for (auto& f : combsLeft) f.reset();
    for (auto& f : combsRight) f.reset();
    for (auto& f : allpassesLeft) f.reset();
    for (auto& f : allpassesRight) f.reset();
}

void StereoReverb::process(juce::AudioBuffer<float>& buffer, float roomSize, float width, float wet, float dry)
{
    if (combsLeft.empty())
        return;

    roomSize = juce::jlimit(0.0f, 1.0f, roomSize);
    width = juce::jlimit(0.0f, 1.0f, width);
    wet = juce::jlimit(0.0f, 1.0f, wet);
    dry = juce::jlimit(0.0f, 1.0f, dry);

    const float feedback = juce::jmin(0.98f, roomSize * 0.28f + 0.7f);
    const bool stereo = buffer.getNumChannels() >= 2;
    const int numSamples = buffer.getNumSamples();

    auto* left = buffer.getWritePointer(0);
    auto* right = stereo ? buffer.getWritePointer(1) : nullptr;

    // Freeverb width law: wet1 scales the own-side tail, wet2 the opposite side
    const float wet1 = wet * (width * 0.5f + 0.5f);
    const float wet2 = wet * ((1.0f - width) * 0.5f);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = left[i];
        const float inR = stereo ? right[i] : inL;
        const float reverbInput = (inL + inR) * 0.5f * inputGain;

        float tailL = 0.0f, tailR = 0.0f;
        for (size_t c = 0; c < combsLeft.size(); ++c)
        {
            tailL += combsLeft[c].process(reverbInput, feedback, damping);
            tailR += combsRight[c].process(reverbInput, feedback, damping);
        }

        for (size_t a = 0; a < allpassesLeft.size(); ++a)
        {
            tailL = allpassesLeft[a].process(tailL);
            tailR = allpassesRight[a].process(tailR);
        }

        float outL = inL * dry + tailL * wet1 + tailR * wet2;
        if (!std::isfinite(outL))
            outL = 0.0f;
        left[i] = juce::jlimit(-2.0f, 2.0f, outL);

        if (stereo)
        {
            float outR = inR * dry + tailR * wet1 + tailL * wet2;
            if (!std::isfinite(outR))
                outR = 0.0f;
            right[i] = juce::jlimit(-2.0f, 2.0f, outR);
        }
    }
}
