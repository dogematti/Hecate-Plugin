#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// High-gain waveshaping stage. A fixed 80 Hz high-pass tightens the low end
// before the drive (essential for palm-muted chugging), the shaper runs at
// 2x oversampling to keep aliasing down, and the tone control is a one-pole
// low-pass after the drive.
class Saturator
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // drive and tone are both 0..1
    void process(juce::AudioBuffer<float>& buffer, float drive, float tone);

    int getLatencySamples() const;

private:
    static constexpr float maxDriveDb = 36.0f;
    static constexpr float tightHz = 80.0f;
    static constexpr float toneMinHz = 800.0f;
    static constexpr float toneMaxHz = 12000.0f;

    double sampleRate = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> tightFilter;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    float toneState[2] = {0.0f, 0.0f};
};
