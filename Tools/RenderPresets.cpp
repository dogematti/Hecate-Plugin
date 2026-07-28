// Offline render/verification harness for CI.
//
// Renders every factory preset through the full Hecate signal chain using a
// deterministic synthetic DI (or a user-supplied wav), sanity-checks the
// output (finite, not silent, limiter intact) and writes one 24-bit wav per
// preset.
//
// Usage:
//   RenderPresets [outputDir]            render with the synthetic DI
//   RenderPresets input.wav [outputDir]  render with the first 10 s of a file
//
// Exit code 0 only if every preset passes.

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "../Source/PluginProcessor.h"

namespace
{

constexpr double defaultSampleRate = 48000.0;
constexpr int blockSize = 512;

// Band-limited sawtooth: sum of harmonics k = 1..N with amplitude 1/k,
// keeping every partial below 8 kHz
double bandlimitedSaw(double frequency, double t)
{
    double sum = 0.0;
    for (int k = 1; k * frequency < 8000.0; ++k)
        sum += std::sin(juce::MathConstants<double>::twoPi * (double)k * frequency * t) / (double)k;
    return sum;
}

// Fast attack, exponential decay
double envelope(double t, double attackSeconds, double decayTau)
{
    return (1.0 - std::exp(-t / attackSeconds)) * std::exp(-t / decayTau);
}

// 6 seconds of deterministic stereo test DI: 3 s of eighth-note palm-mute
// style low-B sawtooth bursts, then 3 s of a sustained B5 power chord.
// Peak-normalised to 0.3. No randomness, so renders are byte-stable.
juce::AudioBuffer<float> makeSyntheticDI(double sampleRate)
{
    constexpr double lowB = 61.735;
    constexpr double fifth = 92.5;

    const int burstLength = (int)(0.25 * sampleRate);
    const int halfLength = burstLength * 12; // 3 s of eighth notes
    const int totalLength = halfLength * 2;

    std::vector<double> mono((size_t)totalLength, 0.0);

    // One palm-mute burst, tiled 12 times (they are all identical)
    for (int i = 0; i < burstLength; ++i)
    {
        const double t = (double)i / sampleRate;
        mono[(size_t)i] = bandlimitedSaw(lowB, t) * envelope(t, 0.002, 0.09);
    }

    for (int note = 1; note < 12; ++note)
        for (int i = 0; i < burstLength; ++i)
            mono[(size_t)(note * burstLength + i)] = mono[(size_t)i];

    // Sustained power chord with a slow decay
    for (int i = 0; i < halfLength; ++i)
    {
        const double t = (double)i / sampleRate;
        mono[(size_t)(halfLength + i)] = (bandlimitedSaw(lowB, t) + bandlimitedSaw(fifth, t))
                                         * envelope(t, 0.002, 1.5);
    }

    double peak = 0.0;
    for (double v : mono)
        peak = std::max(peak, std::abs(v));

    const double gain = peak > 0.0 ? 0.3 / peak : 0.0;

    juce::AudioBuffer<float> buffer(2, totalLength);
    for (int i = 0; i < totalLength; ++i)
    {
        const float v = (float)(mono[(size_t)i] * gain);
        buffer.setSample(0, i, v);
        buffer.setSample(1, i, v);
    }
    return buffer;
}

// Reads up to the first 10 seconds of a wav file as a stereo buffer.
// Returns the file's sample rate via sampleRateOut, or false on failure.
bool loadFileDI(const juce::File& file, juce::AudioBuffer<float>& out, double& sampleRateOut)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0.0 || reader->lengthInSamples <= 0)
        return false;

    sampleRateOut = reader->sampleRate;
    const juce::int64 maxSamples = (juce::int64)(reader->sampleRate * 10.0);
    const int numSamples = (int)std::min(reader->lengthInSamples, maxSamples);
    const int fileChannels = (int)reader->numChannels;

    juce::AudioBuffer<float> temp(std::max(fileChannels, 1), numSamples);
    if (!reader->read(&temp, 0, numSamples, 0, true, true))
        return false;

    out.setSize(2, numSamples);
    out.copyFrom(0, 0, temp, 0, 0, numSamples);
    out.copyFrom(1, 0, temp, fileChannels > 1 ? 1 : 0, 0, numSamples);
    return true;
}

bool writeWav(const juce::File& file, const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    file.deleteFile();

    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(stream.get(), sampleRate,
                                  (unsigned int)buffer.getNumChannels(), 24, {}, 0));
    if (writer == nullptr)
        return false;

    stream.release(); // the writer owns the stream now
    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

} // namespace

int main(int argc, char* argv[])
{
    // Makes this thread the message thread, so setCurrentProgram applies
    // preset parameters synchronously
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Argument handling: an existing .wav as argv[1] switches to file-DI
    // mode and shifts the output dir to argv[2]
    juce::String outDirArg = "renders";
    juce::File inputFile;

    if (argc > 1)
    {
        const juce::File candidate =
            juce::File::getCurrentWorkingDirectory().getChildFile(juce::String(argv[1]));

        if (candidate.existsAsFile() && candidate.hasFileExtension("wav"))
        {
            inputFile = candidate;
            if (argc > 2)
                outDirArg = argv[2];
        }
        else
        {
            outDirArg = argv[1];
        }
    }

    const juce::File outputDir =
        juce::File::getCurrentWorkingDirectory().getChildFile(outDirArg);
    if (!outputDir.createDirectory())
    {
        std::printf("ERROR: could not create output directory %s\n",
                    outputDir.getFullPathName().toRawUTF8());
        return 1;
    }

    double sampleRate = defaultSampleRate;
    juce::AudioBuffer<float> di;

    if (inputFile != juce::File())
    {
        if (!loadFileDI(inputFile, di, sampleRate))
        {
            std::printf("ERROR: could not read input file %s\n",
                        inputFile.getFullPathName().toRawUTF8());
            return 1;
        }
        std::printf("DI: %s (%.0f Hz, %.2f s)\n", inputFile.getFullPathName().toRawUTF8(),
                    sampleRate, (double)di.getNumSamples() / sampleRate);
    }
    else
    {
        di = makeSyntheticDI(sampleRate);
        std::printf("DI: synthetic (48000 Hz, 6.00 s)\n");
    }

    const int totalSamples = di.getNumSamples();

    HecateAudioProcessor processor;
    const int numPresets = processor.getNumPrograms();

    juce::AudioBuffer<float> render(2, totalSamples);
    juce::AudioBuffer<float> chunk(2, blockSize);
    juce::MidiBuffer midi;

    int numPassed = 0;

    for (int presetIndex = 0; presetIndex < numPresets; ++presetIndex)
    {
        const juce::String name = processor.getProgramName(presetIndex);

        // Apply the preset first, then re-prepare so smoothers snap to the
        // new values and no tails leak between renders
        processor.setCurrentProgram(presetIndex);
        processor.prepareToPlay(sampleRate, blockSize);

        for (int position = 0; position < totalSamples; position += blockSize)
        {
            const int numThisBlock = std::min(blockSize, totalSamples - position);

            for (int ch = 0; ch < 2; ++ch)
                chunk.copyFrom(ch, 0, di, ch, position, numThisBlock);

            juce::AudioBuffer<float> block(chunk.getArrayOfWritePointers(), 2, 0, numThisBlock);
            midi.clear();
            processor.processBlock(block, midi);

            for (int ch = 0; ch < 2; ++ch)
                render.copyFrom(ch, position, chunk, ch, 0, numThisBlock);
        }

        // Verify: every sample finite, engine not silent, limiter intact
        bool finite = true;
        double sumSquares = 0.0;
        float peak = 0.0f;

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* samples = render.getReadPointer(ch);
            for (int i = 0; i < totalSamples; ++i)
            {
                const float v = samples[i];
                if (!std::isfinite(v))
                    finite = false;
                peak = std::max(peak, std::abs(v));
                sumSquares += (double)v * (double)v;
            }
        }

        const double rms = std::sqrt(sumSquares / (2.0 * (double)totalSamples));
        const float peakDb = juce::Decibels::gainToDecibels(peak, -120.0f);
        const float rmsDb = juce::Decibels::gainToDecibels((float)rms, -120.0f);

        bool passed = finite && rmsDb >= -50.0f && peakDb <= 0.1f;

        const juce::File outFile =
            outputDir.getChildFile(juce::String(presetIndex) + "-"
                                   + name.replaceCharacter(' ', '-') + ".wav");
        if (!writeWav(outFile, render, sampleRate))
        {
            std::printf("ERROR: could not write %s\n", outFile.getFullPathName().toRawUTF8());
            passed = false;
        }

        if (passed)
            ++numPassed;

        std::printf("%-24s peak %8.2f dB  rms %8.2f dB  %s%s\n",
                    name.toRawUTF8(), peakDb, rmsDb, passed ? "PASS" : "FAIL",
                    finite ? "" : " (non-finite samples)");
    }

    std::printf("%d/%d presets passed\n", numPassed, numPresets);
    return numPassed == numPresets ? 0 : 1;
}
