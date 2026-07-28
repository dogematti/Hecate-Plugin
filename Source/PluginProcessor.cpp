#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

HecateAudioProcessor::HecateAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    params.octaveDirect = apvts.getRawParameterValue(param::octaveDirect);
    params.octaveLevel = apvts.getRawParameterValue(param::octaveLevel);
    params.gateOn = apvts.getRawParameterValue(param::gateOn);
    params.gateThreshold = apvts.getRawParameterValue(param::gateThreshold);
    params.gain = apvts.getRawParameterValue(param::gain);
    params.tight = apvts.getRawParameterValue(param::tight);
    params.boost = apvts.getRawParameterValue(param::boost);
    params.tone = apvts.getRawParameterValue(param::tone);
    params.bass = apvts.getRawParameterValue(param::bass);
    params.mid = apvts.getRawParameterValue(param::mid);
    params.treble = apvts.getRawParameterValue(param::treble);
    params.presence = apvts.getRawParameterValue(param::presence);
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
    params.outputGain = apvts.getRawParameterValue(param::outputGain);
}

void HecateAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    gate.prepare(spec);
    gate.setRatio(10.0f);
    gate.setAttack(1.0f);
    gate.setRelease(60.0f);

    octaver.prepare(sampleRate, samplesPerBlock);
    saturator.prepare(sampleRate, samplesPerBlock);
    compressor.prepare(sampleRate, samplesPerBlock);
    equalizer.prepare(sampleRate, samplesPerBlock);
    cabinet.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    chorus.prepare(spec);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delay.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);

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

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Gate keys off the raw input, before the octaver adds content
    if (params.gateOn->load() > 0.5f)
    {
        const float preGateLevel = buffer.getMagnitude(0, numSamples);
        gate.setThreshold(params.gateThreshold->load());
        gate.process(context);
        const float postGateLevel = buffer.getMagnitude(0, numSamples);
        meterGateOpen.store(preGateLevel < 1.0e-4f || postGateLevel > preGateLevel * 0.25f);
    }
    else
    {
        meterGateOpen.store(true);
    }

    octaver.process(buffer, params.octaveLevel->load(), params.octaveDirect->load());

    saturator.process(buffer, params.gain->load(), params.tight->load(),
                      params.tone->load(), params.boost->load() > 0.5f);

    if (params.compOn->load() > 0.5f)
        compressor.process(buffer, params.compThreshold->load(), params.compRatio->load());

    equalizer.process(buffer, params.bass->load(), params.mid->load(),
                      params.treble->load(), params.presence->load());
    cabinet.process(buffer);

    chorus.setRate(params.chorusRate->load());
    chorus.setDepth(params.chorusDepth->load());
    chorus.setMix(params.chorusMix->load());
    chorus.process(context);

    delay.process(buffer, resolveDelayTimeMs(), params.delayFeedback->load(),
                  params.delayMix->load());
    reverb.process(buffer, params.reverbRoom->load(), params.reverbWidth->load(),
                   params.reverbDamp->load(), params.reverbPreDelay->load(),
                   params.reverbWet->load(), params.reverbDry->load());

    const float outputGain = params.outputGain->load();
    buffer.applyGainRamp(0, numSamples, lastOutputGain, outputGain);
    lastOutputGain = outputGain;

    // Peak level with a slow fall for the output meter
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

        auto irPath = apvts.state.getProperty("irPath").toString();
        if (irPath.isNotEmpty())
        {
            juce::File irFile(irPath);

            // Fall back to ~/Documents/Hecate/IRs/<name> if the file moved
            if (!irFile.existsAsFile())
                irFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                             .getChildFile("Hecate").getChildFile("IRs")
                             .getChildFile(irFile.getFileName());

            if (irFile.existsAsFile())
                loadImpulseResponse(irFile);
            else
                clearImpulseResponse();
        }
        else
        {
            clearImpulseResponse();
        }
    }
}

void HecateAudioProcessor::loadImpulseResponse(const juce::File& file)
{
    cabinet.loadImpulseResponse(file);
    if (cabinet.isUserLoaded())
        apvts.state.setProperty("irPath", file.getFullPathName(), nullptr);
}

void HecateAudioProcessor::clearImpulseResponse()
{
    cabinet.clear();
    apvts.state.removeProperty("irPath", nullptr);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HecateAudioProcessor();
}
