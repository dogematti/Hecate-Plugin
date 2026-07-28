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

void StereoReverb::prepare(double newSampleRate, int)
{
    sampleRate = newSampleRate;
    const double scale = sampleRate / 44100.0;

    preDelayBuffer.assign((size_t)std::max(1, (int)(sampleRate * maxPreDelaySeconds)), 0.0f);
    preDelayWriteIndex = 0;

    for (auto* smoothed : {&wetSmoothed, &drySmoothed, &widthSmoothed})
        smoothed->reset(sampleRate, 0.05);

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

    std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
    preDelayWriteIndex = 0;
}

void StereoReverb::process(juce::AudioBuffer<float>& buffer, float roomSize, float width,
                           float damping, float preDelayMs, float wet, float dry)
{
    if (combsLeft.empty() || preDelayBuffer.empty())
        return;

    roomSize = juce::jlimit(0.0f, 1.0f, roomSize);
    damping = juce::jlimit(0.0f, 1.0f, damping);
    widthSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, width));
    wetSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, wet));
    drySmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, dry));

    const float feedback = juce::jmin(0.98f, roomSize * 0.28f + 0.7f);
    const float combDamping = damping * 0.5f;   // Freeverb-style scaling; 1.0 would choke the combs
    const int preDelaySamples = juce::jlimit(0, (int)preDelayBuffer.size() - 1,
                                             (int)(preDelayMs * 0.001 * sampleRate));
    const bool stereo = buffer.getNumChannels() >= 2;
    const int numSamples = buffer.getNumSamples();

    auto* left = buffer.getWritePointer(0);
    auto* right = stereo ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Sanitize before anything enters the feedback loops: one NaN in the
        // combs would circulate forever and mute the plugin until prepare()
        float inL = left[i];
        if (!std::isfinite(inL))
            inL = 0.0f;
        float inR = stereo ? right[i] : inL;
        if (!std::isfinite(inR))
            inR = 0.0f;

        // Pre-delay pushes the tail behind the riff
        preDelayBuffer[(size_t)preDelayWriteIndex] = (inL + inR) * 0.5f * inputGain;
        int readIndex = preDelayWriteIndex - preDelaySamples;
        if (readIndex < 0)
            readIndex += (int)preDelayBuffer.size();
        const float reverbInput = preDelayBuffer[(size_t)readIndex];
        if (++preDelayWriteIndex >= (int)preDelayBuffer.size())
            preDelayWriteIndex = 0;

        float tailL = 0.0f, tailR = 0.0f;
        for (size_t c = 0; c < combsLeft.size(); ++c)
        {
            tailL += combsLeft[c].process(reverbInput, feedback, combDamping);
            tailR += combsRight[c].process(reverbInput, feedback, combDamping);
        }

        for (size_t a = 0; a < allpassesLeft.size(); ++a)
        {
            tailL = allpassesLeft[a].process(tailL);
            tailR = allpassesRight[a].process(tailR);
        }

        // Freeverb width law: wet1 scales the own-side tail, wet2 the opposite side
        const float wetNow = wetSmoothed.getNextValue();
        const float dryNow = drySmoothed.getNextValue();
        const float widthNow = widthSmoothed.getNextValue();
        const float wet1 = wetNow * (widthNow * 0.5f + 0.5f);
        const float wet2 = wetNow * ((1.0f - widthNow) * 0.5f);

        float outL = inL * dryNow + tailL * wet1 + tailR * wet2;
        if (!std::isfinite(outL))
            outL = 0.0f;
        left[i] = juce::jlimit(-2.0f, 2.0f, outL);

        if (stereo)
        {
            float outR = inR * dryNow + tailR * wet1 + tailL * wet2;
            if (!std::isfinite(outR))
                outR = 0.0f;
            right[i] = juce::jlimit(-2.0f, 2.0f, outR);
        }
    }
}
