#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Four-band tone stack: bass/treble shelves, mid/presence bells.
// Coefficients are only rebuilt when a gain actually changes, so the
// audio thread normally does no allocation.
class Equalizer
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // All gains in dB
    void process(juce::AudioBuffer<float>& buffer, float bass, float mid, float treble, float presence);

private:
    static constexpr float bassHz = 100.0f;
    static constexpr float midHz = 1000.0f;
    static constexpr float presenceHz = 4000.0f;
    static constexpr float trebleHz = 8000.0f;

    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter> chain;
    double sampleRate = 44100.0;
    float lastBass = 1.0e9f, lastMid = 1.0e9f, lastTreble = 1.0e9f, lastPresence = 1.0e9f;

    void updateCoefficients(float bass, float mid, float treble, float presence);
};
