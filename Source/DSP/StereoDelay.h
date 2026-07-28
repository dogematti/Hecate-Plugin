#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Straight stereo delay with feedback. The delay time is smoothed so
// moving the knob sweeps tape-style instead of clicking.
class StereoDelay
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // timeMs 50..1000, feedback 0..0.9, mix 0..1
    void process(juce::AudioBuffer<float>& buffer, float timeMs, float feedback, float mix);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    juce::SmoothedValue<float> delaySamples;
    double sampleRate = 44100.0;
    bool initialised = false;
};
