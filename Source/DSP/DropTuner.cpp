#include "DropTuner.h"

void DropTuner::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    // Default preset: ~120ms block / ~30ms interval, configured for stereo
    stretch.presetDefault(numShifterChannels, (float) sampleRate);

    // Pre-allocated output scratch so process() never allocates
    scratch.setSize(numShifterChannels, maxBlockSize);

    active = false;
    reset();
}

void DropTuner::reset()
{
    stretch.reset();
    scratch.clear();
    active = false;
}

void DropTuner::process(juce::AudioBuffer<float>& buffer, int semitonesDown)
{
    if (semitonesDown <= 0)
    {
        // True bypass: leave the buffer untouched. Drop the shifter state so
        // nothing stale plays back when we re-engage later.
        if (active)
            stretch.reset();
        active = false;
        return;
    }

    if (!active)
    {
        // Coming out of bypass: start from clean state so the first block
        // doesn't burp out stale spectra.
        stretch.reset();
        active = true;
    }

    stretch.setTransposeSemitones((float) -semitonesDown);

    const int numSamples = juce::jmin(buffer.getNumSamples(), scratch.getNumSamples());
    const int numChannels = juce::jmin(buffer.getNumChannels(), numShifterChannels);
    jassert(numChannels > 0 && buffer.getNumSamples() <= scratch.getNumSamples());

    // The stretch is configured for exactly 2 channels and always reads/writes
    // both, so build 2-entry pointer arrays; mono buffers feed the same read
    // pointer to both inputs, and each shifter channel gets its own scratch
    // channel for output.
    const float* inputs[numShifterChannels];
    float* outputs[numShifterChannels];
    for (int ch = 0; ch < numShifterChannels; ++ch)
    {
        inputs[ch] = buffer.getReadPointer(juce::jmin(ch, numChannels - 1));
        outputs[ch] = scratch.getWritePointer(ch);
    }

    // Equal input/output sample counts: no time-stretch, pitch-shift only
    stretch.process(inputs, numSamples, outputs, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        buffer.copyFrom(ch, 0, scratch, ch, 0, numSamples);
}

// Latency note: while active the shifter delays the signal by
// inputLatency() + outputLatency() samples; the processor polls this and
// updates the host's reported latency whenever it changes. Bypassed = 0.
int DropTuner::getLatencySamples() const
{
    if (!active)
        return 0;

    return stretch.inputLatency() + stretch.outputLatency();
}
