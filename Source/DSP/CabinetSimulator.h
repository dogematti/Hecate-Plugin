#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Cabinet impulse response via convolution. A synthesised 4x12-style IR is
// loaded by default so the amp sounds finished with zero setup; loading a
// file replaces it and clear() brings it back. Loading is safe from the
// message thread while audio runs; juce::dsp::Convolution swaps the IR in
// on a background thread.
class CabinetSimulator
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    void loadImpulseResponse(const juce::File& file);
    void clear();   // reverts to the built-in cabinet

    bool isUserLoaded() const { return userLoaded.load(); }
    juce::String getName() const { return name; }

private:
    void loadDefaultCabinet();

    juce::dsp::Convolution convolution;
    std::atomic<bool> userLoaded{false};
    juce::String name;
    juce::File userFile;
    double sampleRate = 44100.0;
};
