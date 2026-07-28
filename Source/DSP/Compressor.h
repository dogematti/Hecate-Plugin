#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Feed-forward compressor with a stereo-linked peak detector: one envelope
// drives the same gain on every channel, so the stereo image never shifts.
class Compressor
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // threshold in dB, ratio as N (for N:1)
    void process(juce::AudioBuffer<float>& buffer, float thresholdDb, float ratio);

private:
    static constexpr float attackSeconds = 0.005f;
    static constexpr float releaseSeconds = 0.1f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
};
