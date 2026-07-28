#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Sub-octave generator: a delay-line pitch shifter reading at half speed
// through two crossfaded taps, with the wet signal low-passed so the sub
// stays smooth. Sits before the amp like an octave pedal.
class Octaver
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // mix 0..1 blends dry against the sub-octave
    void process(juce::AudioBuffer<float>& buffer, float mix);

private:
    static constexpr float windowSeconds = 0.05f;
    static constexpr float wetLowpassHz = 900.0f;

    float readTap(int channel, float delaySamples) const;

    std::vector<float> delayBuffer[2];
    float lowpassState[2] = {0.0f, 0.0f};
    float lowpassCoeff = 0.1f;
    int windowSamples = 2205;
    int bufferSize = 0;
    int writeIndex = 0;
    float phase = 0.0f;
};
