#include "Saturator.h"

namespace
{
    float onePoleCoeff(float cutoffHz, double sampleRate)
    {
        return 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoffHz / (float)sampleRate);
    }

    // Same formula with fc = 1 / (2*pi*t), i.e. 1 - exp(-1 / (t * rate))
    float timeConstantCoeff(float timeMs, double sampleRate)
    {
        return 1.0f - std::exp(-1.0f / (timeMs * 0.001f * (float)sampleRate));
    }

    // One row per amp channel; order matches param::channelChoices
    // (Clean, Rhythm, Lead, Thall, Doom). This table IS the amp's voice —
    // tune sounds by editing rows, not code.
    struct Voicing
    {
        float driveScale;      // scales both drive halves
        float interstageLpHz;  // fizz control between stages
        float padDb;           // interstage attenuation into stage 2
        float stage2Gain;
        float biasBase, biasDepth;   // static + envelope-driven asymmetry
        float clipGain;        // stage 3 drive
        bool fuzzClip;         // hard clip instead of tanh in stage 3
        float brightDb;        // bright-cap emphasis around the clipper
        float screamerHpHz;    // boost pedal low-cut corner
    };

    constexpr Voicing kVoicings[] = {
        // Clean: barely-driven, open top
        {0.35f, 12000.0f,  -8.0f, 1.2f, 0.05f, 0.10f, 0.9f, false, 3.0f, 720.0f},
        // Rhythm: the tight modern high-gain core
        {1.00f,  9500.0f, -12.0f, 2.0f, 0.15f, 0.35f, 1.6f, false, 6.0f, 720.0f},
        // Lead: smoother, more compression and bloom
        {1.10f,  8000.0f, -10.0f, 2.2f, 0.20f, 0.40f, 1.1f, false, 5.0f, 720.0f},
        // Thall: dry, clanky, aggressive top, higher boost corner
        {1.05f, 10500.0f, -12.0f, 2.4f, 0.10f, 0.25f, 1.9f, false, 7.0f, 900.0f},
        // Doom: fuzz clipping, thick and dark
        {1.20f,  7000.0f,  -9.0f, 2.6f, 0.15f, 0.30f, 2.2f, true,  3.0f, 650.0f},
    };
}

void Saturator::applyChannel(int channel)
{
    const int index = juce::jlimit(0, (int)std::size(kVoicings) - 1, channel);
    if (index == currentChannel)
        return;

    currentChannel = index;
    const auto& voicing = kVoicings[(size_t)index];

    vDriveScale = voicing.driveScale;
    vPadGain = juce::Decibels::decibelsToGain(voicing.padDb);
    vStage2Gain = voicing.stage2Gain;
    vBiasBase = voicing.biasBase;
    vBiasDepth = voicing.biasDepth;
    vClipGain = voicing.clipGain;
    vFuzzClip = voicing.fuzzClip;

    interstageLpCoeff = onePoleCoeff(voicing.interstageLpHz, oversampledRate);
    screamerHpCoeff = onePoleCoeff(voicing.screamerHpHz, sampleRate);

    *preEmphasisFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, brightShelfHz, brightShelfQ, juce::Decibels::decibelsToGain(voicing.brightDb));
    *deEmphasisFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, brightShelfHz, brightShelfQ, juce::Decibels::decibelsToGain(-voicing.brightDb));
}

void Saturator::prepare(double newSampleRate, int maxBlockSize)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)maxBlockSize;
    spec.numChannels = 2;

    tightFilter.prepare(spec);
    tightFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);

    preEmphasisFilter.prepare(spec);
    deEmphasisFilter.prepare(spec);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampler->initProcessing((size_t)maxBlockSize);

    lowBandBuffer.setSize(2, maxBlockSize);

    lowBandDelaySamples = oversampler->getLatencyInSamples();
    lowBandDelay.setMaximumDelayInSamples((int)std::ceil(lowBandDelaySamples) + 8);
    lowBandDelay.prepare(spec);

    oversampledRate = sampleRate * 4.0;

    splitCoeff = onePoleCoeff(splitCrossoverHz, sampleRate);
    screamerLpCoeff = onePoleCoeff(screamerLowPassHz, sampleRate);
    dcCoeff = onePoleCoeff(dcBlockerHz, sampleRate);

    interstageHpCoeff = onePoleCoeff(interstageHighPassHz, oversampledRate);
    stageLpCoeff = onePoleCoeff(stageLowPassHz, oversampledRate);
    envAttackCoeff = timeConstantCoeff(envAttackMs, oversampledRate);
    envReleaseCoeff = timeConstantCoeff(envReleaseMs, oversampledRate);

    // Load the voiced coefficients (keeps the selected channel across prepares)
    const int keepChannel = currentChannel < 0 ? 1 : currentChannel;
    currentChannel = -1;
    applyChannel(keepChannel);

    reset();
}

void Saturator::reset()
{
    tightFilter.reset();
    preEmphasisFilter.reset();
    deEmphasisFilter.reset();

    if (oversampler != nullptr)
        oversampler->reset();

    lowBandBuffer.clear();
    lowBandDelay.reset();

    for (int ch = 0; ch < 2; ++ch)
    {
        splitState[ch] = 0.0f;
        screamerHpState[ch] = 0.0f;
        screamerLpState[ch] = 0.0f;
        dcState[ch] = 0.0f;
        toneState[ch] = 0.0f;
        interstageHpState[ch] = 0.0f;
        interstageLpState[ch] = 0.0f;
        stageLpState[ch] = 0.0f;
        envState[ch] = 0.0f;
    }
}

int Saturator::getLatencySamples() const
{
    return oversampler != nullptr ? (int)std::ceil(oversampler->getLatencyInSamples()) : 0;
}

void Saturator::process(juce::AudioBuffer<float>& buffer, float gain, float tightHz,
                        float tone, bool boost, int channel)
{
    if (oversampler == nullptr)
        return;

    applyChannel(channel);

    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    juce::dsp::AudioBlock<float> fullBlock(buffer);
    auto block = fullBlock.getSubsetChannelBlock(0, (size_t)numChannels);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // 1. Tight high-pass
    tightFilter.setCutoffFrequency(juce::jlimit(20.0f, 400.0f, tightHz));
    tightFilter.process(context);

    // 2. Split at 120 Hz: low band gets gentle saturation and skips the drive
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        auto* lowOut = lowBandBuffer.getWritePointer(ch);
        float state = splitState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = samples[i];
            state += splitCoeff * (x - state);
            const float saturatedLow = std::tanh(state * lowBandDrive) / lowBandDrive * lowBandMakeup;

            // Match the high band's oversampler latency
            lowBandDelay.pushSample(ch, saturatedLow);
            lowOut[i] = lowBandDelay.popSample(ch, lowBandDelaySamples);

            samples[i] = x - state;
        }

        splitState[ch] = state;
    }

    // 3. Screamer boost: band-limit the high band and slam the front end
    if (boost)
    {
        const float boostGain = juce::Decibels::decibelsToGain(screamerBoostDb);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* samples = buffer.getWritePointer(ch);
            float hpState = screamerHpState[ch];
            float lpState = screamerLpState[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const float x = samples[i];
                hpState += screamerHpCoeff * (x - hpState);
                lpState += screamerLpCoeff * ((x - hpState) - lpState);
                samples[i] = lpState * boostGain;
            }

            screamerHpState[ch] = hpState;
            screamerLpState[ch] = lpState;
        }
    }

    // 4. Bright-cap pre-emphasis (undone after the drive at step 7)
    preEmphasisFilter.process(context);

    // 5. 4x oversampled three-stage clipping cascade
    const float driveDb = minDriveDb + gain * driveRangeDb;
    const float half = juce::Decibels::decibelsToGain(driveDb * 0.5f) * vDriveScale;
    const float pad = vPadGain;

    auto upsampled = oversampler->processSamplesUp(block);

    for (size_t ch = 0; ch < upsampled.getNumChannels(); ++ch)
    {
        auto* samples = upsampled.getChannelPointer(ch);
        float hpState = interstageHpState[ch];
        float lpState = interstageLpState[ch];
        float sLpState = stageLpState[ch];
        float env = envState[ch];

        for (size_t i = 0; i < upsampled.getNumSamples(); ++i)
        {
            // Stage 1: main gain stage (first half of the drive)
            const float stage1 = std::tanh(samples[i] * half);

            // Interstage: kill DC, tame fizz, then pad into stage 2
            hpState += interstageHpCoeff * (stage1 - hpState);
            float x = stage1 - hpState;
            lpState += interstageLpCoeff * (x - lpState);
            x = lpState * pad;

            // Dynamic asymmetry: bias rides the stage 1 envelope
            const float rectified = std::abs(stage1);
            env += (rectified > env ? envAttackCoeff : envReleaseCoeff) * (rectified - env);
            const float bias = vBiasBase + vBiasDepth * juce::jmin(1.0f, env);

            // Stage 2: second half of the drive, even harmonics from the bias
            x = std::tanh(vStage2Gain * x * half + bias) - std::tanh(bias);

            sLpState += stageLpCoeff * (x - sLpState);
            x = sLpState;

            // Stage 3: output stage character (voiced)
            x = vFuzzClip
                    ? juce::jlimit(-fuzzClipLevel, fuzzClipLevel, x * vClipGain) * fuzzMakeup
                    : std::tanh(vClipGain * x);

            samples[i] = x;
        }

        interstageHpState[ch] = hpState;
        interstageLpState[ch] = lpState;
        stageLpState[ch] = sLpState;
        envState[ch] = env;
    }

    // 6. Downsample
    oversampler->processSamplesDown(block);

    // 7. Bright-cap de-emphasis restores the pre-drive tilt
    deEmphasisFilter.process(context);

    // 8. DC blocker mops up the offset the asymmetric stage introduces
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        float state = dcState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            const float x = samples[i];
            samples[i] = x - state;
            state += dcCoeff * (x - state);
        }

        dcState[ch] = state;
    }

    // 9. Recombine: undriven low band restores the chug
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.addFrom(ch, 0, lowBandBuffer, ch, 0, numSamples);

    // 10. Tone: 1st-order low-pass swept exponentially 2 kHz..16 kHz
    const float toneCutoff = toneBaseHz * std::pow(toneRatio, tone);
    const float toneCoeff = onePoleCoeff(toneCutoff, sampleRate);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* samples = buffer.getWritePointer(ch);
        float state = toneState[ch];

        for (int i = 0; i < numSamples; ++i)
        {
            state += toneCoeff * (samples[i] - state);
            samples[i] = state;
        }

        toneState[ch] = state;
    }
}
