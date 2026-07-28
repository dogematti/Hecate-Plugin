#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/Octaver.h"
#include "DSP/Saturator.h"
#include "DSP/Equalizer.h"
#include "DSP/Compressor.h"
#include "DSP/CabinetSimulator.h"
#include "DSP/StereoDelay.h"
#include "DSP/StereoReverb.h"

// Signal chain: input gain -> octaver -> gate -> saturator -> compressor
// -> EQ -> cabinet IR -> chorus -> delay -> reverb -> output gain.
class HecateAudioProcessor : public juce::AudioProcessor
{
public:
    HecateAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Hecate"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadImpulseResponse(const juce::File& file);
    void clearImpulseResponse();
    juce::String getImpulseResponseName() const { return cabinet.getName(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    // Raw atomic values cached once so processBlock never does string lookups
    struct ParameterValues
    {
        std::atomic<float>* inputGain = nullptr;
        std::atomic<float>* octaveMix = nullptr;
        std::atomic<float>* gateThreshold = nullptr;
        std::atomic<float>* drive = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* bass = nullptr;
        std::atomic<float>* mid = nullptr;
        std::atomic<float>* treble = nullptr;
        std::atomic<float>* presence = nullptr;
        std::atomic<float>* compThreshold = nullptr;
        std::atomic<float>* compRatio = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* reverbRoom = nullptr;
        std::atomic<float>* reverbWidth = nullptr;
        std::atomic<float>* reverbWet = nullptr;
        std::atomic<float>* reverbDry = nullptr;
        std::atomic<float>* outputGain = nullptr;
    };

    ParameterValues params;

    Octaver octaver;
    juce::dsp::NoiseGate<float> gate;
    Saturator saturator;
    Compressor compressor;
    Equalizer equalizer;
    CabinetSimulator cabinet;
    juce::dsp::Chorus<float> chorus;
    StereoDelay delay;
    StereoReverb reverb;

    // Previous block's gains, for click-free ramping
    float lastInputGain = 1.0f;
    float lastOutputGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessor)
};
