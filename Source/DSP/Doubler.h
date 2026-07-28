#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Quad-track-style widener. Two "ghost takes" of the mono sum are delayed by
// ~14 ms (left) and ~19 ms (right), each drifting slowly in delay time via
// its own LFO so they detune by a few cents like real double-tracked takes.
// The dry signal is pulled back slightly as the doubles come up so engaging
// the effect widens rather than just gets louder.
class Doubler
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float amount);   // 0..1, 0 = fully dry

private:
    static constexpr float baseDelayLeftMs = 14.0f;
    static constexpr float baseDelayRightMs = 19.0f;
    static constexpr float modDepthMs = 0.4f;
    static constexpr float lfoRateLeftHz = 0.13f;
    static constexpr float lfoRateRightHz = 0.17f;
    static constexpr float voiceGain = 0.7f;
    static constexpr float dryDuck = 0.25f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    float lfoPhaseLeft = 0.0f;
    float lfoPhaseRight = juce::MathConstants<float>::pi;   // opposite starting phase
    float lfoIncrementLeft = 0.0f;
    float lfoIncrementRight = 0.0f;
    double sampleRate = 44100.0;
};
