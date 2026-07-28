#include "Equalizer.h"

void Equalizer::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;
    chain.prepare(spec);

    // Force a coefficient rebuild on the next process call
    lastBass = lastMid = lastMidFreq = lastTreble = 1.0e9f;
}

void Equalizer::reset()
{
    chain.reset();
}

void Equalizer::process(juce::AudioBuffer<float>& buffer, float bass, float mid, float midFreqHz, float treble)
{
    updateCoefficients(bass, mid, midFreqHz, treble);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chain.process(context);
}

void Equalizer::updateCoefficients(float bass, float mid, float midFreqHz, float treble)
{
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    if (bass != lastBass)
    {
        *chain.get<0>().state = *Coefficients::makeLowShelf(
            sampleRate, bassHz, 0.707f, juce::Decibels::decibelsToGain(bass));
        lastBass = bass;
    }

    if (mid != lastMid || midFreqHz != lastMidFreq)
    {
        *chain.get<1>().state = *Coefficients::makePeakFilter(
            sampleRate, juce::jlimit(100.0f, 4000.0f, midFreqHz), midQ,
            juce::Decibels::decibelsToGain(mid));
        lastMid = mid;
        lastMidFreq = midFreqHz;
    }

    if (treble != lastTreble)
    {
        *chain.get<2>().state = *Coefficients::makeHighShelf(
            sampleRate, trebleHz, 0.707f, juce::Decibels::decibelsToGain(treble));
        lastTreble = treble;
    }
}
