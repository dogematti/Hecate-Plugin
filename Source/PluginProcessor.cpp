#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

HecateAudioProcessor::HecateAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undoManager, "Parameters", createParameterLayout())
{
    params.inputTrim = apvts.getRawParameterValue(param::inputTrim);
    params.dropTune = apvts.getRawParameterValue(param::dropTune);
    params.octaveDirect = apvts.getRawParameterValue(param::octaveDirect);
    params.octaveLevel = apvts.getRawParameterValue(param::octaveLevel);
    params.gateOn = apvts.getRawParameterValue(param::gateOn);
    params.gateThreshold = apvts.getRawParameterValue(param::gateThreshold);
    params.gain = apvts.getRawParameterValue(param::gain);
    params.tight = apvts.getRawParameterValue(param::tight);
    params.boost = apvts.getRawParameterValue(param::boost);
    params.channel = apvts.getRawParameterValue(param::channel);
    params.tone = apvts.getRawParameterValue(param::tone);
    params.cleanBlend = apvts.getRawParameterValue(param::cleanBlend);
    params.bass = apvts.getRawParameterValue(param::bass);
    params.mid = apvts.getRawParameterValue(param::mid);
    params.midFreq = apvts.getRawParameterValue(param::midFreq);
    params.treble = apvts.getRawParameterValue(param::treble);
    params.presence = apvts.getRawParameterValue(param::presence);
    params.sag = apvts.getRawParameterValue(param::sag);
    params.depth = apvts.getRawParameterValue(param::depth);
    params.compOn = apvts.getRawParameterValue(param::compOn);
    params.compThreshold = apvts.getRawParameterValue(param::compThreshold);
    params.compRatio = apvts.getRawParameterValue(param::compRatio);
    params.chorusRate = apvts.getRawParameterValue(param::chorusRate);
    params.chorusDepth = apvts.getRawParameterValue(param::chorusDepth);
    params.chorusDelay = apvts.getRawParameterValue(param::chorusDelay);
    params.chorusFeedback = apvts.getRawParameterValue(param::chorusFeedback);
    params.chorusMix = apvts.getRawParameterValue(param::chorusMix);
    params.delaySync = apvts.getRawParameterValue(param::delaySync);
    params.delayTime = apvts.getRawParameterValue(param::delayTime);
    params.delayFeedback = apvts.getRawParameterValue(param::delayFeedback);
    params.delayDamp = apvts.getRawParameterValue(param::delayDamp);
    params.delayPingPong = apvts.getRawParameterValue(param::delayPingPong);
    params.delayMix = apvts.getRawParameterValue(param::delayMix);
    params.reverbRoom = apvts.getRawParameterValue(param::reverbRoom);
    params.reverbWidth = apvts.getRawParameterValue(param::reverbWidth);
    params.reverbDamp = apvts.getRawParameterValue(param::reverbDamp);
    params.reverbPreDelay = apvts.getRawParameterValue(param::reverbPreDelay);
    params.reverbWet = apvts.getRawParameterValue(param::reverbWet);
    params.reverbDry = apvts.getRawParameterValue(param::reverbDry);
    params.irBlend = apvts.getRawParameterValue(param::irBlend);
    params.cabLowCut = apvts.getRawParameterValue(param::cabLowCut);
    params.cabHighCut = apvts.getRawParameterValue(param::cabHighCut);
    params.doubler = apvts.getRawParameterValue(param::doubler);
    params.doublerSpread = apvts.getRawParameterValue(param::doublerSpread);
    params.doublerDrift = apvts.getRawParameterValue(param::doublerDrift);
    params.outputGain = apvts.getRawParameterValue(param::outputGain);
}

void HecateAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    dropTuner.prepare(sampleRate, samplesPerBlock);
    gate.prepare(sampleRate, samplesPerBlock);
    octaver.prepare(sampleRate, samplesPerBlock);
    compressor.prepare(sampleRate, samplesPerBlock);
    saturator.prepare(sampleRate, samplesPerBlock);
    equalizer.prepare(sampleRate, samplesPerBlock);
    powerAmp.prepare(sampleRate, samplesPerBlock);
    cabinet.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    chorus.prepare(spec);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    doublerFx.prepare(sampleRate, samplesPerBlock);
    delay.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);

    limiter.prepare(spec);
    limiter.setThreshold(-0.3f);
    limiter.setRelease(60.0f);

    lastTrimGain = juce::Decibels::decibelsToGain(params.inputTrim->load());
    lastOutputGain = params.outputGain->load();
    lastCleanBlend = params.cleanBlend->load();

    cleanScratch.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    cleanHpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 90.0f / (float)sampleRate);
    cleanLpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 6000.0f / (float)sampleRate);
    cleanHpState[0] = cleanHpState[1] = 0.0f;
    cleanLpState[0] = cleanLpState[1] = 0.0f;

    lastReportedLatency = saturator.getLatencySamples() + dropTuner.getLatencySamples();
    setLatencySamples(lastReportedLatency);
}

bool HecateAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    // Mono guitar track into a stereo bus is the typical amp-sim setup
    return mainIn == mainOut
           || (mainIn == juce::AudioChannelSet::mono() && mainOut == juce::AudioChannelSet::stereo());
}

// Maps the sync choice to milliseconds using the host tempo (fallback 120 BPM)
float HecateAudioProcessor::resolveDelayTimeMs()
{
    const int syncChoice = (int)params.delaySync->load();
    if (syncChoice == 0)
        return params.delayTime->load();

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto hostBpm = position->getBpm())
                bpm = *hostBpm;

    // Choice order: Free, 1/4, 1/8., 1/8, 1/8T, 1/16 (in fractions of a beat)
    static constexpr double beatFractions[] = {1.0, 1.0, 0.75, 0.5, 1.0 / 3.0, 0.25};
    const double quarterMs = 60000.0 / juce::jmax(20.0, bpm);
    return (float)juce::jlimit(50.0, 1000.0, quarterMs * beatFractions[syncChoice]);
}

void HecateAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Mono input on a stereo bus: duplicate the guitar onto the right channel
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);

    // MIDI program changes switch presets (deferred: parameters may not be
    // touched from the audio thread)
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (message.isProgramChange())
        {
            const int program = message.getProgramChangeNumber();
            juce::MessageManager::callAsync([this, program] { setCurrentProgram(program); });
        }
    }

    const float trimGain = juce::Decibels::decibelsToGain(params.inputTrim->load());
    buffer.applyGainRamp(0, numSamples, lastTrimGain, trimGain);
    lastTrimGain = trimGain;

    // Meter fall is time-based (60 dB over ~0.4 s) so it looks the same at
    // every buffer size
    const float meterDecay = std::pow(0.001f, (float)numSamples / (float)(currentSampleRate * 0.4));

    meterInput.store(juce::jmax(buffer.getMagnitude(0, numSamples),
                                meterInput.load() * meterDecay));

    // Feed the tuner tap (raw post-trim guitar, channel 0)
    {
        const auto* raw = buffer.getReadPointer(0);
        int pos = tunerWritePos.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            tunerRing[(size_t)pos] = raw[i];
            if (++pos >= (int)tunerRing.size())
                pos = 0;
        }
        tunerWritePos.store(pos, std::memory_order_release);
    }

    // Drop-tune the raw guitar; its latency changes with engagement, so the
    // host report is refreshed from the message thread when it moves
    dropTuner.process(buffer, (int)params.dropTune->load());

    const int totalLatency = saturator.getLatencySamples() + dropTuner.getLatencySamples();
    if (totalLatency != lastReportedLatency)
    {
        lastReportedLatency = totalLatency;
        juce::MessageManager::callAsync([this, totalLatency] { setLatencySamples(totalLatency); });
    }

    if (params.gateOn->load() > 0.5f)
        meterGateOpen.store(gate.process(buffer, params.gateThreshold->load()));
    else
        meterGateOpen.store(true);

    octaver.process(buffer, params.octaveLevel->load(), params.octaveDirect->load());

    // Capture the clean parallel path before any drive
    const int cleanChannels = juce::jmin(buffer.getNumChannels(), cleanScratch.getNumChannels());
    for (int ch = 0; ch < cleanChannels; ++ch)
        cleanScratch.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    if (params.compOn->load() > 0.5f)
        compressor.process(buffer, params.compThreshold->load(), params.compRatio->load());

    saturator.process(buffer, params.gain->load(), params.tight->load(),
                      params.tone->load(), params.boost->load() > 0.5f,
                      (int)params.channel->load());

    equalizer.process(buffer, params.bass->load(), params.mid->load(),
                      params.midFreq->load(), params.treble->load());

    powerAmp.process(buffer, params.sag->load(), params.presence->load(),
                     params.depth->load());

    cabinet.process(buffer, params.irBlend->load(), params.cabLowCut->load(),
                    params.cabHighCut->load());

    // Clean blend: the undistorted take mixed in post-cab, band-limited so
    // it sits naturally against the miked amp (the thall clarity trick)
    const float cleanBlendNow = params.cleanBlend->load();
    if (cleanBlendNow > 0.001f || lastCleanBlend > 0.001f)
    {
        for (int ch = 0; ch < cleanChannels; ++ch)
        {
            const auto* clean = cleanScratch.getReadPointer(ch);
            auto* out = buffer.getWritePointer(ch);
            float hp = cleanHpState[ch];
            float lp = cleanLpState[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                hp += cleanHpCoeff * (clean[i] - hp);
                lp += cleanLpCoeff * ((clean[i] - hp) - lp);
                const float blend = lastCleanBlend
                                    + (cleanBlendNow - lastCleanBlend) * (float)i / (float)numSamples;
                out[i] += lp * blend;
            }

            cleanHpState[ch] = hp;
            cleanLpState[ch] = lp;
        }
    }
    lastCleanBlend = cleanBlendNow;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    chorus.setRate(params.chorusRate->load());
    chorus.setDepth(params.chorusDepth->load());
    chorus.setCentreDelay(params.chorusDelay->load());
    chorus.setFeedback(params.chorusFeedback->load());
    chorus.setMix(params.chorusMix->load());
    chorus.process(context);

    doublerFx.process(buffer, params.doubler->load(), params.doublerSpread->load(),
                      params.doublerDrift->load());

    delay.process(buffer, resolveDelayTimeMs(), params.delayFeedback->load(),
                  params.delayDamp->load(), params.delayPingPong->load() > 0.5f,
                  params.delayMix->load());
    reverb.process(buffer, params.reverbRoom->load(), params.reverbWidth->load(),
                   params.reverbDamp->load(), params.reverbPreDelay->load(),
                   params.reverbWet->load(), params.reverbDry->load());

    const float outputGain = params.outputGain->load();
    buffer.applyGainRamp(0, numSamples, lastOutputGain, outputGain);
    lastOutputGain = outputGain;

    // Safety limiter so no preset or IR combination can clip the host
    limiter.process(context);

    const float peak = buffer.getMagnitude(0, numSamples);
    meterOutput.store(juce::jmax(peak, meterOutput.load() * meterDecay));
}

int HecateAudioProcessor::getNumPrograms()
{
    return (int)getFactoryPresets().size();
}

void HecateAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    currentProgram = index;

    // Hosts may call this from any thread; parameter writes belong on the
    // message thread
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        applyFactoryPreset(apvts, index);
    else
        juce::MessageManager::callAsync([this, index] { applyFactoryPreset(apvts, index); });
}

const juce::String HecateAudioProcessor::getProgramName(int index)
{
    const auto& presets = getFactoryPresets();
    if (index >= 0 && index < (int)presets.size())
        return presets[(size_t)index].name;
    return {};
}

juce::AudioProcessorEditor* HecateAudioProcessor::createEditor()
{
    return new HecateAudioProcessorEditor(*this);
}

void HecateAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    const auto xml = apvts.state.toXmlString();
    destData.append(xml.toRawUTF8(), xml.getNumBytesAsUTF8());
}

void HecateAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    const auto xmlString = juce::String::fromUTF8((const char*)data, sizeInBytes);
    if (auto xml = juce::XmlDocument::parse(xmlString))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        // Force-sync every parameter from the restored tree. replaceState's
        // change detection skips parameters whose tree value is unchanged,
        // which can leave stale (e.g. un-snapped bool) raw values behind.
        for (auto* parameter : getParameters())
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            {
                const auto child = apvts.state.getChildWithProperty("id", ranged->paramID);
                if (child.isValid())
                    ranged->setValueNotifyingHost(
                        ranged->convertTo0to1((float)child.getProperty("value")));
            }
        }

        reloadImpulseResponsesFromState();
    }
}

void HecateAudioProcessor::reloadImpulseResponsesFromState()
{
    const char* pathProperties[] = {"irPath", "irPath2"};

    for (int slot = 0; slot < 2; ++slot)
    {
        const auto irPath = apvts.state.getProperty(pathProperties[slot]).toString();
        if (irPath.isEmpty())
        {
            if (cabinet.isUserLoaded(slot))
                clearImpulseResponse(slot);
            continue;
        }

        juce::File irFile(irPath);

        // Fall back to ~/Documents/Hecate/IRs/<name> if the file moved
        if (!irFile.existsAsFile())
            irFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("Hecate").getChildFile("IRs")
                         .getChildFile(irFile.getFileName());

        // Skip redundant reloads: they cost disk I/O and momentarily cut the
        // convolution tail (e.g. on every A/B toggle)
        if (irFile.existsAsFile())
        {
            if (loadedIrPaths[slot] != irFile.getFullPathName())
                loadImpulseResponse(irFile, slot);
        }
        else if (cabinet.isUserLoaded(slot))
        {
            clearImpulseResponse(slot);
        }
    }
}

void HecateAudioProcessor::loadImpulseResponse(const juce::File& file, int slot)
{
    cabinet.loadImpulseResponse(file, slot);
    if (cabinet.isUserLoaded(slot))
    {
        loadedIrPaths[slot] = file.getFullPathName();
        apvts.state.setProperty(slot == 0 ? "irPath" : "irPath2",
                                file.getFullPathName(), nullptr);
    }
}

void HecateAudioProcessor::clearImpulseResponse(int slot)
{
    cabinet.clearSlot(slot);
    loadedIrPaths[slot].clear();
    apvts.state.removeProperty(slot == 0 ? "irPath" : "irPath2", nullptr);
}

void HecateAudioProcessor::readTunerBuffer(float* dest, int numSamples) const
{
    const int size = (int)tunerRing.size();
    int pos = tunerWritePos.load(std::memory_order_acquire) - numSamples;
    while (pos < 0)
        pos += size;

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = tunerRing[(size_t)pos];
        if (++pos >= size)
            pos = 0;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HecateAudioProcessor();
}
