#include "CabinetSimulator.h"

void CabinetSimulator::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = (juce::uint32)numChannels;
    convolution.prepare(spec);
}

void CabinetSimulator::reset()
{
    convolution.reset();
}

void CabinetSimulator::process(juce::AudioBuffer<float>& buffer)
{
    if (!loaded.load())
        return;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    convolution.process(context);
}

void CabinetSimulator::loadImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    convolution.loadImpulseResponse(file,
                                    juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::yes,
                                    0,
                                    juce::dsp::Convolution::Normalise::yes);
    name = file.getFileName();
    loaded.store(true);
}

void CabinetSimulator::clear()
{
    loaded.store(false);
    name.clear();
}
