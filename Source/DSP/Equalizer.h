#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Three-band tone stack: bass/treble shelves and a semi-parametric mid bell
// whose centre sweeps 250 Hz - 2 kHz (metal scoops live at 350-700 Hz).
// Presence lives in the PowerAmp stage. Coefficients are only rebuilt when
// a value actually changes, so the audio thread normally does no allocation.
class Equalizer
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // bass/mid/treble in dB, midFreqHz 250..2000
    void process(juce::AudioBuffer<float>& buffer, float bass, float mid, float midFreqHz, float treble);

private:
    static constexpr float bassHz = 100.0f;
    static constexpr float trebleHz = 8000.0f;
    static constexpr float midQ = 1.0f;

    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::ProcessorChain<Filter, Filter, Filter> chain;
    double sampleRate = 44100.0;
    float lastBass = 1.0e9f, lastMid = 1.0e9f, lastMidFreq = 1.0e9f, lastTreble = 1.0e9f;

    void updateCoefficients(float bass, float mid, float midFreqHz, float treble);
};
