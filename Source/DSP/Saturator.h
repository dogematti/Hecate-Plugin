#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// High-gain preamp with split-band drive and per-channel voicings. Topology:
//
//   variable "tight" high-pass
//   -> split at 120 Hz: LOW band bypasses the drive (gentle tanh only,
//      delayed to match the oversampler), HIGH band continues:
//        optional screamer boost (voiced HP -> 3.5 kHz LP -> +9 dB)
//        -> bright-cap pre-emphasis (voiced shelf @ 1.4 kHz)
//        -> [4x oversampled: stage 1 tanh -> interstage 10 Hz HP + voiced LP
//           + voiced pad -> stage 2 tanh with envelope-driven dynamic bias
//           -> 12 kHz LP -> stage 3 clip (voiced gain, tanh or fuzz)]
//        -> bright-cap de-emphasis
//        -> DC blocker (5 Hz)
//   -> recombine bands -> 1st-order tone low-pass (2 kHz..16 kHz)
//
// Gain 0..1 sweeps +18..+72 dB of drive. The CHANNEL selects a full voicing
// table row (Clean / Rhythm / Lead / Thall / Doom) — stage gains, interstage
// filtering, asymmetry, clip curve and boost corner all change together, so
// channels are genuinely different amps, and voicing an amp by ear means
// editing one row in kVoicings (Saturator.cpp).
class Saturator
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // gain and tone 0..1, tightHz 40..300, channel indexes param::channelChoices
    void process(juce::AudioBuffer<float>& buffer, float gain, float tightHz,
                 float tone, bool boost, int channel);

    int getLatencySamples() const;

private:
    void applyChannel(int channel);

    static constexpr float minDriveDb = 18.0f;
    static constexpr float driveRangeDb = 54.0f;

    static constexpr float splitCrossoverHz = 120.0f;
    static constexpr float lowBandDrive = 3.0f;
    static constexpr float lowBandMakeup = 1.2f;

    static constexpr float screamerLowPassHz = 3500.0f;
    static constexpr float screamerBoostDb = 9.0f;

    static constexpr float brightShelfHz = 1400.0f;
    static constexpr float brightShelfQ = 0.7f;

    static constexpr float interstageHighPassHz = 10.0f;

    static constexpr float envAttackMs = 0.5f;
    static constexpr float envReleaseMs = 40.0f;

    static constexpr float stageLowPassHz = 12000.0f;
    static constexpr float fuzzClipLevel = 0.8f;
    static constexpr float fuzzMakeup = 1.1f;

    static constexpr float dcBlockerHz = 5.0f;

    static constexpr float toneBaseHz = 2000.0f;
    static constexpr float toneRatio = 8.0f;

    using FixedFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                       juce::dsp::IIR::Coefficients<float>>;

    double sampleRate = 44100.0;
    double oversampledRate = 176400.0;
    juce::dsp::StateVariableTPTFilter<float> tightFilter;
    FixedFilter preEmphasisFilter;
    FixedFilter deEmphasisFilter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Pre-allocated in prepare; process() must never allocate
    juce::AudioBuffer<float> lowBandBuffer;

    // The high band picks up the oversampler's (fractional) latency; the low
    // band is delayed to match so the two stay time-aligned when recombined
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lowBandDelay;
    float lowBandDelaySamples = 0.0f;

    // Active voicing (loaded from kVoicings by applyChannel)
    int currentChannel = -1;
    float vDriveScale = 1.0f;
    float vPadGain = 0.25f;
    float vStage2Gain = 2.0f;
    float vBiasBase = 0.15f;
    float vBiasDepth = 0.35f;
    float vClipGain = 1.6f;
    bool vFuzzClip = false;

    // Per-channel one-pole states at the base rate
    float splitState[2] = {0.0f, 0.0f};
    float screamerHpState[2] = {0.0f, 0.0f};
    float screamerLpState[2] = {0.0f, 0.0f};
    float dcState[2] = {0.0f, 0.0f};
    float toneState[2] = {0.0f, 0.0f};

    // Per-channel one-pole and envelope states at the 4x oversampled rate
    float interstageHpState[2] = {0.0f, 0.0f};
    float interstageLpState[2] = {0.0f, 0.0f};
    float stageLpState[2] = {0.0f, 0.0f};
    float envState[2] = {0.0f, 0.0f};

    // Coefficients fixed in prepare (voiced ones rebuilt in applyChannel,
    // toneCoeff recomputed per block)
    float splitCoeff = 0.0f;
    float screamerHpCoeff = 0.0f;
    float screamerLpCoeff = 0.0f;
    float dcCoeff = 0.0f;
    float interstageHpCoeff = 0.0f;
    float interstageLpCoeff = 0.0f;
    float stageLpCoeff = 0.0f;
    float envAttackCoeff = 0.0f;
    float envReleaseCoeff = 0.0f;
};
