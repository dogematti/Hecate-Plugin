#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// Dual-slot cabinet impulse response stage. Slot A carries a synthesised
// 4x12-style IR by default so the amp sounds finished with zero setup;
// loading a file replaces it and clearSlot(0) brings it back. Slot B is
// empty until the user loads a second IR, at which point process() blends
// the two with an equal-power law. Post-blend high/low trim filters tame
// boomy or fizzy IRs. Loading is safe from the message thread while audio
// runs; juce::dsp::Convolution swaps IRs in on a background thread.
class CabinetSimulator
{
public:
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    // blend 0=slot A only .. 1=slot B only (equal-power; ignored when slot B is empty)
    // lowCutHz 20..300 (<=20 bypasses), highCutHz 3000..20000 (>=20000 bypasses)
    void process(juce::AudioBuffer<float>& buffer, float blend, float lowCutHz, float highCutHz);
    void loadImpulseResponse(const juce::File& file, int slot);   // slot 0 or 1
    void clearSlot(int slot);   // slot 0 reverts to the built-in cab, slot 1 goes empty
    bool isUserLoaded(int slot) const;
    juce::String getName(int slot) const;   // slot 1 returns empty string when empty

    // Copy of the built-in IR (and its native rate), for the editor's
    // response display
    const juce::AudioBuffer<float>& getDefaultImpulse() const { return defaultIrCopy; }
    double getDefaultImpulseSampleRate() const { return defaultIrSampleRate; }

private:
    juce::AudioBuffer<float> defaultIrCopy;
    double defaultIrSampleRate = 44100.0;
    void loadDefaultCabinet();
    void updateTrimFilters(float lowCutHz, float highCutHz);

    using TrimFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                      juce::dsp::IIR::Coefficients<float>>;

    juce::dsp::Convolution convolutionA;
    juce::dsp::Convolution convolutionB;
    juce::AudioBuffer<float> scratch;   // slot B render target, sized in prepare()

    TrimFilter highPass;
    TrimFilter lowPass;
    bool highPassEnabled = false;
    bool lowPassEnabled = false;
    float cachedLowCut = -1.0f;
    float cachedHighCut = -1.0f;

    std::atomic<bool> userLoadedA{false};
    std::atomic<bool> userLoadedB{false};
    juce::String nameA;
    juce::String nameB;
    juce::File userFileA;
    juce::File userFileB;
    double sampleRate = 44100.0;
};
