#include "CabinetSimulator.h"

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

// Approximates a miked 4x12: steep low cut ~62 Hz, a low thump at 105 Hz,
// scoops at 240 Hz and 1.4 kHz, a presence peak at 3.3 kHz, then a steep
// 4th-order speaker roll-off from 5.4 kHz. Built by running a unit impulse
// through IIR filters, adding two short early-reflection taps for comb
// texture (cone/cab reflections) and windowing the tail.
void CabinetSimulator::loadDefaultCabinet()
{
    const int length = juce::jmax(512, (int)(sampleRate * 0.06));
    juce::AudioBuffer<float> ir(2, length);
    ir.clear();
    ir.setSample(0, 0, 1.0f);

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    const std::vector<juce::dsp::IIR::Coefficients<float>::Ptr> stages = {
        Coefficients::makeHighPass(sampleRate, 62.0f, 1.1f),
        Coefficients::makePeakFilter(sampleRate, 105.0f, 1.3f, juce::Decibels::decibelsToGain(6.0f)),
        Coefficients::makePeakFilter(sampleRate, 240.0f, 2.0f, juce::Decibels::decibelsToGain(-4.0f)),
        Coefficients::makePeakFilter(sampleRate, 1400.0f, 2.0f, juce::Decibels::decibelsToGain(-3.0f)),
        Coefficients::makePeakFilter(sampleRate, 3300.0f, 1.4f, juce::Decibels::decibelsToGain(4.0f)),
        Coefficients::makeLowPass(sampleRate, 5400.0f, 0.541f),   // 4th-order Butterworth pair
        Coefficients::makeLowPass(sampleRate, 5400.0f, 1.307f),
        Coefficients::makeLowPass(sampleRate, 6200.0f, 0.7f),
    };

    auto* samples = ir.getWritePointer(0);
    for (const auto& coeffs : stages)
    {
        juce::dsp::IIR::Filter<float> filter(coeffs);
        filter.reset();
        for (int i = 0; i < length; ++i)
            samples[i] = filter.processSample(samples[i]);
    }

    // Early-reflection comb texture: two short delayed taps of the filtered
    // impulse. Iterate backwards so each tap reads the unmodified signal.
    struct Tap { float delayMs, gainDb, polarity; };
    const Tap taps[] = { { 0.35f, -13.0f, -1.0f },
                         { 0.90f, -18.0f,  1.0f } };
    for (const auto& tap : taps)
    {
        const int offset = juce::jmax(1, (int)std::round(sampleRate * tap.delayMs * 0.001));
        const float gain = tap.polarity * juce::Decibels::decibelsToGain(tap.gainDb);
        for (int i = length - 1; i >= offset; --i)
            samples[i] += gain * samples[i - offset];
    }

    // Fade the tail so the IIR truncation doesn't click
    const int fadeStart = length / 4;
    for (int i = fadeStart; i < length; ++i)
    {
        const float t = (float)(i - fadeStart) / (float)(length - fadeStart);
        samples[i] *= std::exp(-5.0f * t);
    }

    ir.copyFrom(1, 0, ir, 0, 0, length);

    defaultIrCopy.makeCopyOf(ir);

    convolutionA.loadImpulseResponse(std::move(ir), sampleRate,
                                     juce::dsp::Convolution::Stereo::yes,
                                     juce::dsp::Convolution::Trim::no,
                                     juce::dsp::Convolution::Normalise::yes);
    nameA = "Built-in 4x12";
}
