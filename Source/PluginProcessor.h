#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/GateProcessor.h"
#include "DSP/Octaver.h"
#include "DSP/Saturator.h"
#include "DSP/Equalizer.h"
#include "DSP/Compressor.h"
#include "DSP/PowerAmp.h"
#include "DSP/CabinetSimulator.h"
#include "DSP/Doubler.h"
#include "DSP/StereoDelay.h"
#include "DSP/StereoReverb.h"

// Signal chain: input trim -> gate -> octaver -> compressor (sustain, pre-
// drive) -> saturator (tight/boost/gain/tone, split-band, 4x oversampled)
// -> EQ (bass / semi-parametric mid / treble) -> power amp (sag, presence,
// depth) -> dual cabinet IR with trim filters -> chorus -> doubler -> delay
// -> reverb -> output gain -> safety limiter.
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
    bool acceptsMidi() const override { return true; }
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

    void loadImpulseResponse(const juce::File& file, int slot);
    void clearImpulseResponse(int slot);
    bool isUserImpulseResponseLoaded(int slot) const { return cabinet.isUserLoaded(slot); }
    juce::String getImpulseResponseName(int slot) const { return cabinet.getName(slot); }

    // Re-loads both IR slots from the "irPath"/"irPath2" state properties
    void reloadImpulseResponsesFromState();

    // Editor support: built-in cab IR for the response display, and a tap of
    // the raw (post-trim) input for the tuner
    const juce::AudioBuffer<float>& getDefaultCabImpulse() const { return cabinet.getDefaultImpulse(); }
    void readTunerBuffer(float* dest, int numSamples) const;

    // Meter values for the editor (updated every block)
    float getInputLevel() const { return meterInput.load(); }
    float getOutputLevel() const { return meterOutput.load(); }
    float getGainReductionDb() const { return compressor.getGainReductionDb(); }
    bool isGateOpen() const { return meterGateOpen.load(); }

    juce::UndoManager& getUndoManager() { return undoManager; }

    // Declared before apvts, which is constructed with a pointer to it
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

private:
    float resolveDelayTimeMs();

    // Raw atomic values cached once so processBlock never does string lookups
    struct ParameterValues
    {
        std::atomic<float>* inputTrim = nullptr;
        std::atomic<float>* octaveDirect = nullptr;
        std::atomic<float>* octaveLevel = nullptr;
        std::atomic<float>* gateOn = nullptr;
        std::atomic<float>* gateThreshold = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* tight = nullptr;
        std::atomic<float>* boost = nullptr;
        std::atomic<float>* clipMode = nullptr;
        std::atomic<float>* tone = nullptr;
        std::atomic<float>* bass = nullptr;
        std::atomic<float>* mid = nullptr;
        std::atomic<float>* midFreq = nullptr;
        std::atomic<float>* treble = nullptr;
        std::atomic<float>* presence = nullptr;
        std::atomic<float>* sag = nullptr;
        std::atomic<float>* depth = nullptr;
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
        std::atomic<float>* irBlend = nullptr;
        std::atomic<float>* cabLowCut = nullptr;
        std::atomic<float>* cabHighCut = nullptr;
        std::atomic<float>* doubler = nullptr;
        std::atomic<float>* outputGain = nullptr;
    };

    ParameterValues params;

    GateProcessor gate;
    Octaver octaver;
    Compressor compressor;
    Saturator saturator;
    Equalizer equalizer;
    PowerAmp powerAmp;
    CabinetSimulator cabinet;
    juce::dsp::Chorus<float> chorus;
    Doubler doublerFx;
    StereoDelay delay;
    StereoReverb reverb;
    juce::dsp::Limiter<float> limiter;

    // Previous block's gains, for click-free ramping
    float lastTrimGain = 1.0f;
    float lastOutputGain = 1.0f;

    int currentProgram = 0;
    double currentSampleRate = 44100.0;

    // Which files are actually loaded per cab slot, so redundant reloads
    // (e.g. every A/B toggle) can be skipped
    juce::String loadedIrPaths[2];

    std::atomic<float> meterInput{0.0f};
    std::atomic<float> meterOutput{0.0f};
    std::atomic<bool> meterGateOpen{true};

    // Single-writer ring buffer feeding the editor's tuner
    std::vector<float> tunerRing = std::vector<float>(8192, 0.0f);
    std::atomic<int> tunerWritePos{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessor)
};
