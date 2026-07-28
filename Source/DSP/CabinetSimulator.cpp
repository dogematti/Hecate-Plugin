#include "CabinetSimulator.h"
#include "BinaryData.h"

#include <juce_audio_formats/juce_audio_formats.h>

void CabinetSimulator::prepare(double newSampleRate, int maxBlockSize, int numChannels)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = (juce::uint32)numChannels;
    convolutionA.prepare(spec);
    convolutionB.prepare(spec);

    scratch.setSize(numChannels, maxBlockSize);

    highPass.prepare(spec);
    lowPass.prepare(spec);
    highPassEnabled = false;
    lowPassEnabled = false;
    cachedLowCut = -1.0f;
    cachedHighCut = -1.0f;

    // Rebuild the built-in IR at the current sample rate, or re-load the
    // user's files (Convolution resamples files internally)
    if (userLoadedA.load() && userFileA.existsAsFile())
        loadImpulseResponse(userFileA, 0);
    else
        loadDefaultCabinet();

    if (userLoadedB.load() && userFileB.existsAsFile())
        loadImpulseResponse(userFileB, 1);
}

void CabinetSimulator::reset()
{
    convolutionA.reset();
    convolutionB.reset();
    highPass.reset();
    lowPass.reset();
}

void CabinetSimulator::process(juce::AudioBuffer<float>& buffer, float blend, float lowCutHz, float highCutHz)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const bool slotBActive = userLoadedB.load();

    if (slotBActive)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            scratch.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        convolutionA.process(context);
    }

    if (slotBActive)
    {
        juce::dsp::AudioBlock<float> scratchBlock(scratch);
        auto subBlock = scratchBlock.getSubBlock(0, (size_t)numSamples);
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        convolutionB.process(context);

        // Equal-power crossfade between the two cabinets
        const float gainA = std::cos(blend * juce::MathConstants<float>::halfPi);
        const float gainB = std::sin(blend * juce::MathConstants<float>::halfPi);
        buffer.applyGain(gainA);
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, 0, scratch, ch, 0, numSamples, gainB);
    }

    updateTrimFilters(lowCutHz, highCutHz);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    if (highPassEnabled)
        highPass.process(context);
    if (lowPassEnabled)
        lowPass.process(context);
}

void CabinetSimulator::updateTrimFilters(float lowCutHz, float highCutHz)
{
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    const bool wantHighPass = lowCutHz > 21.0f;
    if (wantHighPass)
    {
        if (!highPassEnabled)
        {
            highPass.reset();   // clear stale state so enabling doesn't click
            highPassEnabled = true;
            cachedLowCut = -1.0f;
        }
        if (lowCutHz != cachedLowCut)
        {
            *highPass.state = *Coefficients::makeHighPass(sampleRate, lowCutHz, 0.707f);
            cachedLowCut = lowCutHz;
        }
    }
    else
    {
        highPassEnabled = false;
    }

    const bool wantLowPass = highCutHz < 19999.0f;
    if (wantLowPass)
    {
        if (!lowPassEnabled)
        {
            lowPass.reset();
            lowPassEnabled = true;
            cachedHighCut = -1.0f;
        }
        if (highCutHz != cachedHighCut)
        {
            *lowPass.state = *Coefficients::makeLowPass(sampleRate, highCutHz, 0.707f);
            cachedHighCut = highCutHz;
        }
    }
    else
    {
        lowPassEnabled = false;
    }
}

void CabinetSimulator::loadImpulseResponse(const juce::File& file, int slot)
{
    if (!file.existsAsFile() || (slot != 0 && slot != 1))
        return;

    auto& convolution = (slot == 0) ? convolutionA : convolutionB;
    convolution.loadImpulseResponse(file,
                                    juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::yes,
                                    0,
                                    juce::dsp::Convolution::Normalise::yes);
    if (slot == 0)
    {
        nameA = file.getFileName();
        userFileA = file;
        userLoadedA.store(true);
    }
    else
    {
        nameB = file.getFileName();
        userFileB = file;
        userLoadedB.store(true);
    }
}

void CabinetSimulator::clearSlot(int slot)
{
    if (slot == 0)
    {
        userLoadedA.store(false);
        userFileA = juce::File();
        loadDefaultCabinet();
    }
    else if (slot == 1)
    {
        // process() stops touching B immediately; no reset() here — it isn't
        // safe to call concurrently with the audio thread, the stale tail is
        // never rendered again, and prepare() resets the engine anyway
        userLoadedB.store(false);
        userFileB = juce::File();
        nameB.clear();
    }
}

bool CabinetSimulator::isUserLoaded(int slot) const
{
    return slot == 0 ? userLoadedA.load() : userLoadedB.load();
}

juce::String CabinetSimulator::getName(int slot) const
{
    return slot == 0 ? nameA : nameB;
}

// The built-in cab is a real IR (Assets/default-cab.wav, captured by the
// project's author) embedded in the binary, so the amp sounds like a miked
// cabinet with zero setup. Convolution resamples it to the session rate.
void CabinetSimulator::loadDefaultCabinet()
{
    // Decode a copy for the editor's response display
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(
        std::make_unique<juce::MemoryInputStream>(BinaryData::defaultcab_wav,
                                                  (size_t)BinaryData::defaultcab_wavSize, false)));
    if (reader != nullptr)
    {
        const int numSamples = (int)std::min<juce::int64>(reader->lengthInSamples, 1 << 16);
        defaultIrCopy.setSize((int)reader->numChannels, numSamples);
        reader->read(&defaultIrCopy, 0, numSamples, 0, true, reader->numChannels > 1);
        defaultIrSampleRate = reader->sampleRate;
    }

    convolutionA.loadImpulseResponse(BinaryData::defaultcab_wav,
                                     (size_t)BinaryData::defaultcab_wavSize,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::yes,
                                     0,
                                     juce::dsp::Convolution::Normalise::yes);
    nameA = "Hecate Cab";
}
