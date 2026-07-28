#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

HecateAudioProcessor::HecateAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    params.inputTrim = apvts.getRawParameterValue(param::inputTrim);
    params.octaveDirect = apvts.getRawParameterValue(param::octaveDirect);
    params.octaveLevel = apvts.getRawParameterValue(param::octaveLevel);
    params.gateOn = apvts.getRawParameterValue(param::gateOn);
    params.gateThreshold = apvts.getRawParameterValue(param::gateThreshold);
    params.gain = apvts.getRawParameterValue(param::gain);
    params.tight = apvts.getRawParameterValue(param::tight);
    params.boost = apvts.getRawParameterValue(param::boost);
    params.clipMode = apvts.getRawParameterValue(param::clipMode);
    params.tone = apvts.getRawParameterValue(param::tone);
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
    params.chorusMix = apvts.getRawParameterValue(param::chorusMix);
    params.delaySync = apvts.getRawParameterValue(param::delaySync);
    params.delayTime = apvts.getRawParameterValue(param::delayTime);
    params.delayFeedback = apvts.getRawParameterValue(param::delayFeedback);
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
    params.outputGain = apvts.getRawParameterValue(param::outputGain);
}

void HecateAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

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

    setLatencySamples(saturator.getLatencySamples());
}

bool HecateAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return mainOut == layouts.getMainInputChannelSet();
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

void HecateAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    const float trimGain = juce::Decibels::decibelsToGain(params.inputTrim->load());
    buffer.applyGainRamp(0, numSamples, lastTrimGain, trimGain);
    lastTrimGain = trimGain;

    meterInput.store(juce::jmax(buffer.getMagnitude(0, numSamples),
                                meterInput.load() * 0.85f));

    if (params.gateOn->load() > 0.5f)
        meterGateOpen.store(gate.process(buffer, params.gateThreshold->load()));
    else
        meterGateOpen.store(true);

    octaver.process(buffer, params.octaveLevel->load(), params.octaveDirect->load());

    if (params.compOn->load() > 0.5f)
        compressor.process(buffer, params.compThreshold->load(), params.compRatio->load());

    saturator.process(buffer, params.gain->load(), params.tight->load(),
                      params.tone->load(), params.boost->load() > 0.5f,
                      (int)params.clipMode->load());

    equalizer.process(buffer, params.bass->load(), params.mid->load(),
                      params.midFreq->load(), params.treble->load());

    powerAmp.process(buffer, params.sag->load(), params.presence->load(),
                     params.depth->load());

    cabinet.process(buffer, params.irBlend->load(), params.cabLowCut->load(),
                    params.cabHighCut->load());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    chorus.setRate(params.chorusRate->load());
    chorus.setDepth(params.chorusDepth->load());
    chorus.setMix(params.chorusMix->load());
    chorus.process(context);

    doublerFx.process(buffer, params.doubler->load());

    delay.process(buffer, resolveDelayTimeMs(), params.delayFeedback->load(),
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
    meterOutput.store(juce::jmax(peak, meterOutput.load() * 0.85f));
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
    applyFactoryPreset(apvts, index);
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
        apvts.state = juce::ValueTree::fromXml(*xml);
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
            clearImpulseResponse(slot);
            continue;
        }

        juce::File irFile(irPath);

        // Fall back to ~/Documents/Hecate/IRs/<name> if the file moved
        if (!irFile.existsAsFile())
            irFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                         .getChildFile("Hecate").getChildFile("IRs")
                         .getChildFile(irFile.getFileName());

        if (irFile.existsAsFile())
            loadImpulseResponse(irFile, slot);
        else
            clearImpulseResponse(slot);
    }
}

void HecateAudioProcessor::loadImpulseResponse(const juce::File& file, int slot)
{
    cabinet.loadImpulseResponse(file, slot);
    if (cabinet.isUserLoaded(slot))
        apvts.state.setProperty(slot == 0 ? "irPath" : "irPath2",
                                file.getFullPathName(), nullptr);
}

void HecateAudioProcessor::clearImpulseResponse(int slot)
{
    cabinet.clearSlot(slot);
    apvts.state.removeProperty(slot == 0 ? "irPath" : "irPath2", nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HecateAudioProcessor();
}
