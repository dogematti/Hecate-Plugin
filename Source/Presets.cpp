#include "Presets.h"
#include "Parameters.h"

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        {"Default", {}},

        {"Modern Rhythm", {
            {param::gain, 0.6f}, {param::boost, 1.0f}, {param::tight, 130.0f},
            {param::tone, 0.55f}, {param::bass, 2.0f}, {param::mid, -2.0f},
            {param::treble, 2.5f}, {param::presence, 3.0f},
            {param::gateThreshold, -50.0f}, {param::reverbWet, 0.08f}, {param::reverbDry, 0.95f},
        }},

        {"Djent", {
            {param::gain, 0.5f}, {param::boost, 1.0f}, {param::tight, 190.0f},
            {param::tone, 0.6f}, {param::mid, -1.0f}, {param::treble, 3.0f},
            {param::presence, 4.0f}, {param::gateThreshold, -45.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -20.0f}, {param::compRatio, 3.0f},
            {param::reverbWet, 0.05f},
        }},

        {"Doom", {
            {param::gain, 0.85f}, {param::tight, 55.0f}, {param::tone, 0.35f},
            {param::bass, 4.0f}, {param::mid, 1.0f}, {param::octaveLevel, 0.35f},
            {param::gateThreshold, -55.0f}, {param::reverbRoom, 0.7f},
            {param::reverbWet, 0.2f}, {param::reverbDamp, 0.6f},
        }},

        {"Solo Lead", {
            {param::gain, 0.7f}, {param::boost, 1.0f}, {param::tight, 100.0f},
            {param::tone, 0.6f}, {param::mid, 2.0f}, {param::presence, 2.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -30.0f}, {param::compRatio, 3.0f},
            {param::delaySync, 2.0f}, {param::delayFeedback, 0.45f}, {param::delayMix, 0.35f},
            {param::reverbRoom, 0.6f}, {param::reverbWet, 0.18f}, {param::reverbPreDelay, 40.0f},
        }},

        {"Clean Shimmer", {
            {param::gain, 0.05f}, {param::gateOn, 0.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -35.0f}, {param::compRatio, 4.0f},
            {param::treble, 2.0f}, {param::chorusMix, 0.4f}, {param::chorusDepth, 0.35f},
            {param::reverbWet, 0.3f}, {param::reverbDry, 0.8f}, {param::reverbPreDelay, 30.0f},
        }},

        {"Ambient Swells", {
            {param::gain, 0.3f}, {param::octaveLevel, 0.5f}, {param::gateOn, 0.0f},
            {param::delaySync, 1.0f}, {param::delayFeedback, 0.6f}, {param::delayMix, 0.5f},
            {param::reverbRoom, 0.9f}, {param::reverbWidth, 1.0f},
            {param::reverbWet, 0.5f}, {param::reverbDry, 0.6f}, {param::reverbPreDelay, 60.0f},
        }},
    };

    return presets;
}

void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int)presets.size())
        return;

    for (auto* parameter : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            ranged->setValueNotifyingHost(ranged->getDefaultValue());

    for (const auto& [id, value] : presets[(size_t)index].values)
        if (auto* parameter = apvts.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

juce::File getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("Hecate").getChildFile("Presets");
    dir.createDirectory();
    return dir;
}
