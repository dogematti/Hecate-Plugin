#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

// Factory presets: parameter overrides applied on top of defaults.
// Anything not listed in a preset stays at its default value.
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<FactoryPreset>& getFactoryPresets();

// Resets every parameter to default, then applies the preset's overrides
void applyFactoryPreset(juce::AudioProcessorValueTreeState& apvts, int index);

// Directory where user presets are saved/loaded (created on demand)
juce::File getUserPresetDirectory();
