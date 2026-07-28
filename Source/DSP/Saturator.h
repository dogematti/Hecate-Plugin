#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// High-gain preamp. The chain mimics a cascaded tube front end:
//
//   variable "tight" high-pass -> optional screamer-style mid boost
//   -> [2x oversampled: stage 1 tanh -> interstage high-pass -> stage 2
//      asymmetric tanh -> stage 3 tanh] -> DC blocker -> 2nd-order tone LP
//
// Gain 0..1 sweeps +6..+48 dB into the first stage; the boost adds a
// +8 dB mid hump at 750 Hz plus +6 dB of level, the classic metal recipe.
class Saturator
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // gain and tone 0..1, tightHz 40..300
    void process(juce::AudioBuffer<float>& buffer, float gain, float tightHz, float tone, bool boost);

    int getLatencySamples() const;

private:
    static constexpr float minDriveDb = 6.0f;
    static constexpr float maxDriveDb = 48.0f;
    static constexpr float boostLevelDb = 6.0f;
    static constexpr float toneMinHz = 800.0f;
    static constexpr float toneMaxHz = 12000.0f;
    static constexpr float stage2Bias = 0.12f;

    using FixedFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                       juce::dsp::IIR::Coefficients<float>>;

    double sampleRate = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> tightFilter;
    FixedFilter boostFilter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // Per-channel one-pole states: interstage HP (at the oversampled rate),
    // DC blocker and none for tone (handled by toneFilter below)
    float interstageState[2] = {0.0f, 0.0f};
    float dcState[2] = {0.0f, 0.0f};
    float interstageCoeff = 0.0f;
    float dcCoeff = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> toneFilter;
};
