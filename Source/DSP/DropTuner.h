#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <signalsmith-stretch.h>

// Drop tuner: pitch-shifts the whole signal down by 0..4 semitones using
// Signalsmith Stretch. 0 semitones is a true bypass (no processing, no
// latency); anything above engages the shifter, which adds latency that the
// processor reports to the host via getLatencySamples().
class DropTuner
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // semitonesDown 0..4; 0 is a true bypass (no processing, no latency)
    void process(juce::AudioBuffer<float>& buffer, int semitonesDown);

    int getLatencySamples() const;   // 0 when bypassed, the shifter's total latency when active

private:
    static constexpr int numShifterChannels = 2;

    signalsmith::stretch::SignalsmithStretch<float> stretch;
    juce::AudioBuffer<float> scratch;
    double sampleRate = 44100.0;
    bool active = false;
};
