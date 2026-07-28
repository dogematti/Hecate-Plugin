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

// Signal chain: input gain -> gate -> octaver -> saturator (tight/boost/
// gain/tone) -> compressor -> EQ -> cabinet IR -> chorus -> delay -> reverb
// -> output gain.
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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadImpulseResponse(const juce::File& file);
    void clearImpulseResponse();
    bool isUserImpulseResponseLoaded() const { return cabinet.isUserLoaded(); }
    juce::String getImpulseResponseName() const { return cabinet.getName(); }

    // Meter values for the editor (updated every block)
    float getOutputLevel() const { return meterOutput.load(); }
    float getGainReductionDb() const { return compressor.getGainReductionDb(); }
    bool isGateOpen() const { return meterGateOpen.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    float resolveDelayTimeMs();

    // Raw atomic values cached once so processBlock never does string lookups
    struct ParameterValues
    {
        std::atomic<float>* octaveDirect = nullptr;
        std::atomic<float>* octaveLevel = nullptr;
        std::atomic<float>* gateOn = nullptr;
        std::atomic<float>* gateThreshold = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* tight = nullptr;
        std::atomic<float>* boost = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* bass = nullptr;
        std::atomic<float>* mid = nullptr;
        std::atomic<float>* treble = nullptr;
        std::atomic<float>* presence = nullptr;
        std::atomic<float>* compOn = nullptr;
        std::atomic<float>* compThreshold = nullptr;
        std::atomic<float>* compRatio = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delaySync = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* reverbRoom = nullptr;
        std::atomic<float>* reverbWidth = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbPreDelay = nullptr;
        std::atomic<float>* reverbWet = nullptr;
        std::atomic<float>* reverbDry = nullptr;
        std::atomic<float>* outputGain = nullptr;
    };

    ParameterValues params;

    juce::dsp::NoiseGate<float> gate;
    Octaver octaver;
    Saturator saturator;
    Compressor compressor;
    Equalizer equalizer;
    CabinetSimulator cabinet;
    juce::dsp::Chorus<float> chorus;
    StereoDelay delay;
    StereoReverb reverb;

    // Previous block's gain, for click-free ramping
    float lastOutputGain = 1.0f;

    int currentProgram = 0;

    std::atomic<float> meterOutput{0.0f};
    std::atomic<bool> meterGateOpen{true};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessor)
};
