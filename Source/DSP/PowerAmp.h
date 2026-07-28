#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Tube power-amp stage: supply sag (envelope-driven gain dip), presence and
// depth shelving, then a fixed-drive tanh output stage for valve colour.
class PowerAmp
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // sag 0..1, presenceDb -12..12, depthDb 0..6
    void process(juce::AudioBuffer<float>& buffer, float sag, float presenceDb, float depthDb);

private:
    static constexpr float sagAttackSeconds = 0.02f;
    static constexpr float sagReleaseSeconds = 0.15f;
    static constexpr float maxSagDb = -3.0f;
    static constexpr float presenceHz = 3500.0f;
    static constexpr float presenceQ = 0.8f;
    static constexpr float depthHz = 100.0f;
    static constexpr float depthQ = 0.8f;
    static constexpr float driveGain = 2.51f; // fixed +8 dB into the tanh stage

    using StereoFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                        juce::dsp::IIR::Coefficients<float>>;
    StereoFilter presenceFilter;
    StereoFilter depthFilter;
    float lastPresenceDb = 1e9f;
    float lastDepthDb = 1e9f;

    // Per-sample sag gains from the raw input, applied after the filters —
    // preallocated so process() never allocates
    juce::AudioBuffer<float> sagGains;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;
    double sampleRate = 44100.0;
};
