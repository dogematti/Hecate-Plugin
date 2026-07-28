#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

// Noise gate with hysteresis and hold: opens at the threshold, only closes
// 4 dB below it, and waits 20 ms before releasing so staccato riffs don't
// chatter. The gain envelope is separate from the (much faster) detector.
class GateProcessor
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // thresholdDb is the OPEN threshold; returns true if the gate is open at block end
    bool process(juce::AudioBuffer<float>& buffer, float thresholdDb);

private:
    static constexpr float detectorAttackSeconds = 0.0001f;
    static constexpr float detectorReleaseSeconds = 0.005f;
    static constexpr float hysteresisDb = 4.0f;
    static constexpr float holdSeconds = 0.02f;
    static constexpr float gainAttackSeconds = 0.0005f;
    static constexpr float gainReleaseSeconds = 0.05f;
    static constexpr float floorGain = 0.0001f; // -80 dB, not pure zero

    float detectorAttackCoeff = 0.0f;
    float detectorReleaseCoeff = 0.0f;
    float gainAttackCoeff = 0.0f;
    float gainReleaseCoeff = 0.0f;
    int holdSamples = 0;
    int holdCounter = 0;
    float detector = 0.0f;
    float gain = floorGain;
    bool open = false;
};
