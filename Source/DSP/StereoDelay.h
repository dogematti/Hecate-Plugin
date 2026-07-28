#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Stereo delay with feedback. The delay time is smoothed so moving the
// knob sweeps tape-style instead of clicking, the feedback path is
// low-passed (Damping knob) so repeats darken naturally, and ping-pong
// mode cross-feeds the channels so repeats bounce left-right.
class StereoDelay
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // timeMs 50..1000, feedback 0..0.9, dampingHz 1k..12k, mix 0..1
    void process(juce::AudioBuffer<float>& buffer, float timeMs, float feedback,
                 float dampingHz, bool pingPong, float mix);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamples;
    float dampingState[2] = {0.0f, 0.0f};
    float dampingCoeff = 0.5f;
    double sampleRate = 44100.0;
    bool initialised = false;
};
