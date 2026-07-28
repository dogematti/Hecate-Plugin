#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Stereo delay with feedback. The delay time is smoothed so moving the
// knob sweeps tape-style instead of clicking, and the feedback path is
// low-passed so repeats darken naturally.
class StereoDelay
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // timeMs 50..1000, feedback 0..0.9, mix 0..1
    void process(juce::AudioBuffer<float>& buffer, float timeMs, float feedback, float mix);

private:
    static constexpr float feedbackDampingHz = 4500.0f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamples;
    float dampingState[2] = {0.0f, 0.0f};
    float dampingCoeff = 0.5f;
    double sampleRate = 44100.0;
    bool initialised = false;
};
