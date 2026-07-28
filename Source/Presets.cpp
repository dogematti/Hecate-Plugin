#include "Presets.h"
#include "Parameters.h"

// Voicing notes: modern metal tone is moderate preamp gain + boost + tight,
// not maximum gain; scoops live at 350-500 Hz (never 1 kHz); Depth restores
// the low end the tight filter removes. clipMode values: 0 Tube, 1 Modern, 2 Fuzz.
const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = {
        {"Default", {}},

        {"Modern Rhythm", {
            {param::gain, 0.6f}, {param::boost, 1.0f}, {param::tight, 120.0f},
            {param::clipMode, 1.0f}, {param::tone, 0.55f},
            {param::bass, 3.0f}, {param::mid, -3.0f}, {param::midFreq, 450.0f},
            {param::treble, 2.0f}, {param::presence, 3.5f},
            {param::sag, 0.2f}, {param::depth, 3.0f},
            {param::gateThreshold, -42.0f},
            {param::reverbWet, 0.04f}, {param::reverbDry, 1.0f},
        }},

        {"7-String Rhythm", {
            {param::gain, 0.6f}, {param::boost, 1.0f}, {param::tight, 110.0f},
            {param::clipMode, 1.0f}, {param::tone, 0.5f},
            {param::bass, 2.5f}, {param::mid, -4.0f}, {param::midFreq, 450.0f},
            {param::treble, 2.0f}, {param::presence, 3.0f},
            {param::sag, 0.25f}, {param::depth, 4.0f},
            {param::gateThreshold, -40.0f},
            {param::reverbWet, 0.03f}, {param::reverbDry, 1.0f},
        }},

        {"7-String Scoop", {
            {param::gain, 0.65f}, {param::boost, 1.0f}, {param::tight, 140.0f},
            {param::clipMode, 1.0f}, {param::tone, 0.55f},
            {param::bass, 3.5f}, {param::mid, -7.0f}, {param::midFreq, 500.0f},
            {param::treble, 2.5f}, {param::presence, 3.0f},
            {param::sag, 0.2f}, {param::depth, 4.5f},
            {param::gateThreshold, -40.0f},
            {param::reverbWet, 0.03f}, {param::reverbDry, 1.0f},
        }},

        {"Djent", {
            {param::gain, 0.5f}, {param::boost, 1.0f}, {param::tight, 160.0f},
            {param::clipMode, 1.0f}, {param::tone, 0.6f},
            {param::bass, 2.0f}, {param::mid, -5.0f}, {param::midFreq, 380.0f},
            {param::treble, 3.0f}, {param::presence, 4.0f},
            {param::sag, 0.15f}, {param::depth, 2.5f},
            {param::gateThreshold, -38.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -22.0f}, {param::compRatio, 4.0f},
            {param::reverbWet, 0.0f},
        }},

        {"Doom", {
            {param::gain, 1.0f}, {param::boost, 0.0f}, {param::tight, 40.0f},
            {param::clipMode, 2.0f}, {param::tone, 0.28f},
            {param::bass, 5.0f}, {param::mid, 2.5f}, {param::midFreq, 400.0f},
            {param::treble, -1.5f}, {param::presence, -2.0f},
            {param::sag, 0.5f}, {param::depth, 5.0f},
            {param::octaveLevel, 0.35f}, {param::gateOn, 0.0f},
            {param::reverbRoom, 0.7f}, {param::reverbDamp, 0.65f},
            {param::reverbWet, 0.18f}, {param::reverbDry, 0.95f},
        }},

        {"Solo Lead", {
            {param::gain, 0.8f}, {param::boost, 1.0f}, {param::tight, 100.0f},
            {param::clipMode, 0.0f}, {param::tone, 0.6f},
            {param::mid, 2.0f}, {param::midFreq, 750.0f}, {param::presence, 2.0f},
            {param::sag, 0.3f}, {param::depth, 2.0f},
            {param::gateThreshold, -45.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -30.0f}, {param::compRatio, 3.0f},
            {param::delaySync, 2.0f}, {param::delayFeedback, 0.45f}, {param::delayMix, 0.35f},
            {param::reverbRoom, 0.6f}, {param::reverbWet, 0.18f}, {param::reverbPreDelay, 40.0f},
        }},

        {"Clean Shimmer", {
            {param::gain, 0.05f}, {param::boost, 0.0f}, {param::clipMode, 0.0f},
            {param::gateOn, 0.0f},
            {param::compOn, 1.0f}, {param::compThreshold, -35.0f}, {param::compRatio, 4.0f},
            {param::treble, 2.0f}, {param::depth, 1.5f},
            {param::chorusMix, 0.4f}, {param::chorusDepth, 0.35f},
            {param::reverbWet, 0.3f}, {param::reverbDry, 0.8f}, {param::reverbPreDelay, 30.0f},
        }},

        {"Ambient Swells", {
            {param::gain, 0.5f}, {param::octaveLevel, 0.5f}, {param::gateOn, 0.0f},
            {param::clipMode, 0.0f},
            {param::delaySync, 1.0f}, {param::delayFeedback, 0.6f}, {param::delayMix, 0.5f},
            {param::doubler, 0.4f},
            {param::reverbRoom, 0.9f}, {param::reverbWidth, 1.0f},
            {param::reverbWet, 0.5f}, {param::reverbDry, 0.6f}, {param::reverbPreDelay, 60.0f},
        }},
    };

    return presets;
}

// Single pass, gestured: each parameter is set exactly once (override merged
// with default) so the audio thread never renders a half-applied preset, and
// hosts recording automation see proper begin/end gestures.
void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int)presets.size())
        return;

    const auto& values = presets[(size_t)index].values;

    for (auto* parameter : apvts.processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
        if (ranged == nullptr)
            continue;

        float normalised = ranged->getDefaultValue();
        for (const auto& [id, value] : values)
        {
            if (ranged->paramID == id)
            {
                normalised = ranged->convertTo0to1(value);
                break;
            }
        }

        ranged->beginChangeGesture();
        ranged->setValueNotifyingHost(normalised);
        ranged->endChangeGesture();
    }
}

juce::File getUserPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("Hecate").getChildFile("Presets");
    dir.createDirectory();
    return dir;
}
