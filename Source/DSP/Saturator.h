#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// High-gain preamp with split-band drive. Topology:
//
//   variable "tight" high-pass
//   -> split at 120 Hz: LOW band bypasses the drive (gentle tanh only),
//      HIGH band continues:
//        optional screamer boost (720 Hz HP -> 3.5 kHz LP -> +9 dB)
//        -> bright-cap pre-emphasis (+6 dB shelf @ 1.4 kHz)
//        -> [4x oversampled: stage 1 tanh -> interstage 10 Hz HP + 9.5 kHz LP
//           + -14 dB pad -> stage 2 tanh with envelope-driven dynamic bias
//           -> 12 kHz LP -> stage 3 clip (Tube / Modern / Fuzz)]
//        -> bright-cap de-emphasis (-6 dB shelf @ 1.4 kHz)
//        -> DC blocker (5 Hz)
//   -> recombine bands -> 1st-order tone low-pass (2 kHz..16 kHz)
//
// Gain 0..1 sweeps +18..+72 dB of drive, applied as half-gain into each of
// the first two stages. The dynamic bias shifts even-harmonic content with
// playing intensity, the touch-sensitive tube feel.
class Saturator
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // gain and tone 0..1, tightHz 40..300, clipMode: 0=Tube 1=Modern 2=Fuzz
    void process(juce::AudioBuffer<float>& buffer, float gain, float tightHz, float tone, bool boost, int clipMode);

    int getLatencySamples() const;

private:
    static constexpr float minDriveDb = 18.0f;
    static constexpr float driveRangeDb = 54.0f;

    static constexpr float splitCrossoverHz = 120.0f;
    static constexpr float lowBandDrive = 3.0f;
    static constexpr float lowBandMakeup = 1.2f;

    static constexpr float screamerHighPassHz = 720.0f;
    static constexpr float screamerLowPassHz = 3500.0f;
    static constexpr float screamerBoostDb = 9.0f;

    static constexpr float brightShelfHz = 1400.0f;
    static constexpr float brightShelfQ = 0.7f;
    static constexpr float brightShelfDb = 6.0f;

    static constexpr float interstageHighPassHz = 10.0f;
    static constexpr float interstageLowPassHz = 9500.0f;
    static constexpr float interstagePadDb = -12.0f;

    static constexpr float envAttackMs = 0.5f;
    static constexpr float envReleaseMs = 40.0f;
    static constexpr float biasBase = 0.15f;
    static constexpr float biasDepth = 0.35f;
    static constexpr float stage2Gain = 2.0f;

    static constexpr float stageLowPassHz = 12000.0f;

    static constexpr float tubeStageGain = 1.1f;
    static constexpr float modernStageGain = 1.6f;
    static constexpr float fuzzStageGain = 2.2f;
    static constexpr float fuzzClipLevel = 0.8f;
    static constexpr float fuzzMakeup = 1.1f;

    static constexpr float dcBlockerHz = 5.0f;

    static constexpr float toneBaseHz = 2000.0f;
    static constexpr float toneRatio = 8.0f;

    using FixedFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                       juce::dsp::IIR::Coefficients<float>>;

    double sampleRate = 44100.0;
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

    // Coefficients fixed in prepare (toneCoeff is recomputed per block)
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
