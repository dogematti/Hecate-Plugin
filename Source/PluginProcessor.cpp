#include "PluginProcessor.h"
#include "PluginEditor.h"

HecateAudioProcessor::HecateAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    params.inputGain = apvts.getRawParameterValue(param::inputGain);
    params.octaveMix = apvts.getRawParameterValue(param::octaveMix);
    params.gateThreshold = apvts.getRawParameterValue(param::gateThreshold);
    params.drive = apvts.getRawParameterValue(param::drive);
    params.tone = apvts.getRawParameterValue(param::tone);
    params.bass = apvts.getRawParameterValue(param::bass);
    params.mid = apvts.getRawParameterValue(param::mid);
    params.treble = apvts.getRawParameterValue(param::treble);
    params.presence = apvts.getRawParameterValue(param::presence);
    params.compThreshold = apvts.getRawParameterValue(param::compThreshold);
    params.compRatio = apvts.getRawParameterValue(param::compRatio);
    params.chorusRate = apvts.getRawParameterValue(param::chorusRate);
    params.chorusDepth = apvts.getRawParameterValue(param::chorusDepth);
    params.chorusMix = apvts.getRawParameterValue(param::chorusMix);
    params.delayTime = apvts.getRawParameterValue(param::delayTime);
    params.delayFeedback = apvts.getRawParameterValue(param::delayFeedback);
    params.delayMix = apvts.getRawParameterValue(param::delayMix);
    params.reverbRoom = apvts.getRawParameterValue(param::reverbRoom);
    params.reverbWidth = apvts.getRawParameterValue(param::reverbWidth);
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

    octaver.prepare(sampleRate, samplesPerBlock);

    gate.prepare(spec);
    gate.setRatio(10.0f);
    gate.setAttack(1.0f);
    gate.setRelease(60.0f);

    saturator.prepare(sampleRate, samplesPerBlock);
    compressor.prepare(sampleRate, samplesPerBlock);
    equalizer.prepare(sampleRate, samplesPerBlock);
    cabinet.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    chorus.prepare(spec);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    delay.prepare(sampleRate, samplesPerBlock);
    reverb.prepare(sampleRate, samplesPerBlock);

    lastInputGain = params.inputGain->load();
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

void HecateAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    const float inputGain = params.inputGain->load();
    buffer.applyGainRamp(0, numSamples, lastInputGain, inputGain);
    lastInputGain = inputGain;

    octaver.process(buffer, params.octaveMix->load());

    gate.setThreshold(params.gateThreshold->load());
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gate.process(context);

    saturator.process(buffer, params.drive->load(), params.tone->load());
    compressor.process(buffer, params.compThreshold->load(), params.compRatio->load());
    equalizer.process(buffer, params.bass->load(), params.mid->load(),
                      params.treble->load(), params.presence->load());
    cabinet.process(buffer);

    chorus.setRate(params.chorusRate->load());
    chorus.setDepth(params.chorusDepth->load());
    chorus.setMix(params.chorusMix->load());
    chorus.process(context);

    delay.process(buffer, params.delayTime->load(), params.delayFeedback->load(),
                  params.delayMix->load());
    reverb.process(buffer, params.reverbRoom->load(), params.reverbWidth->load(),
                   params.reverbWet->load(), params.reverbDry->load());

    const float outputGain = params.outputGain->load();
    buffer.applyGainRamp(0, numSamples, lastOutputGain, outputGain);
    lastOutputGain = outputGain;
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

        const auto irPath = apvts.state.getProperty("irPath").toString();
        if (irPath.isNotEmpty())
            loadImpulseResponse(juce::File(irPath));
        else
            clearImpulseResponse();
    }
}

void HecateAudioProcessor::loadImpulseResponse(const juce::File& file)
{
    cabinet.loadImpulseResponse(file);
    if (cabinet.isLoaded())
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
