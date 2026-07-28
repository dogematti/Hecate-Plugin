#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Cabinet impulse response via convolution. When no IR is loaded the stage
// is bypassed entirely. Loading is safe from the message thread while audio
// runs; juce::dsp::Convolution swaps the IR in on a background thread.
class CabinetSimulator
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    void loadImpulseResponse(const juce::File& file);
    void clear();

    bool isLoaded() const { return loaded.load(); }
    juce::String getName() const { return name; }

private:
    juce::dsp::Convolution convolution;
    std::atomic<bool> loaded{false};
    juce::String name;
};
