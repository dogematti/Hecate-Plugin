#include "Parameters.h"

namespace
{
    using Attributes = juce::AudioParameterFloatAttributes;

    Attributes decibelText()
    {
        return Attributes().withStringFromValueFunction(
            [](float value, int) { return juce::String(value, 1) + " dB"; });
    }

    Attributes gainAsDecibelText()
    {
        return Attributes().withStringFromValueFunction(
            [](float value, int) { return juce::String(juce::Decibels::gainToDecibels(value), 1) + " dB"; });
    }

    Attributes percentText()
    {
        return Attributes().withStringFromValueFunction(
            [](float value, int) { return juce::String(juce::roundToInt(value * 100.0f)) + " %"; });
    }

    std::unique_ptr<juce::AudioParameterFloat> makeParam(const char* id, const char* name,
                                                         juce::NormalisableRange<float> range,
                                                         float defaultValue, Attributes attributes = {})
    {
        return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                                                           range, defaultValue, attributes);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(makeParam(param::inputGain, "Input Gain",
                               {0.0f, 4.0f, 0.01f}, 1.0f, gainAsDecibelText()));
    params.push_back(makeParam(param::octaveMix, "Octave Mix",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));
    params.push_back(makeParam(param::gateThreshold, "Gate Threshold",
                               {-80.0f, -20.0f, 0.1f}, -60.0f, decibelText()));
    params.push_back(makeParam(param::drive, "Drive",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    params.push_back(makeParam(param::tone, "Tone",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));

    params.push_back(makeParam(param::bass, "Bass",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::mid, "Mid",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::treble, "Treble",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::presence, "Presence",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));

    params.push_back(makeParam(param::compThreshold, "Comp Threshold",
                               {-60.0f, 0.0f, 0.1f}, -24.0f, decibelText()));
    params.push_back(makeParam(param::compRatio, "Comp Ratio",
                               {1.0f, 16.0f, 0.1f}, 4.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(value, 1) + ":1"; })));

    params.push_back(makeParam(param::chorusRate, "Chorus Rate",
                               {0.1f, 5.0f, 0.01f}, 1.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(value, 2) + " Hz"; })));
    params.push_back(makeParam(param::chorusDepth, "Chorus Depth",
                               {0.0f, 1.0f, 0.01f}, 0.25f, percentText()));
    params.push_back(makeParam(param::chorusMix, "Chorus Mix",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));

    params.push_back(makeParam(param::delayTime, "Delay Time",
                               {50.0f, 1000.0f, 1.0f}, 350.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(juce::roundToInt(value)) + " ms"; })));
    params.push_back(makeParam(param::delayFeedback, "Delay Feedback",
                               {0.0f, 0.9f, 0.01f}, 0.35f, percentText()));
    params.push_back(makeParam(param::delayMix, "Delay Mix",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));

    params.push_back(makeParam(param::reverbRoom, "Reverb Room Size",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    params.push_back(makeParam(param::reverbWidth, "Reverb Width",
                               {0.0f, 1.0f, 0.01f}, 1.0f, percentText()));
    params.push_back(makeParam(param::reverbWet, "Reverb Wet",
                               {0.0f, 1.0f, 0.01f}, 0.15f, percentText()));
    params.push_back(makeParam(param::reverbDry, "Reverb Dry",
                               {0.0f, 1.0f, 0.01f}, 0.85f, percentText()));

    params.push_back(makeParam(param::outputGain, "Output Gain",
                               {0.0f, 4.0f, 0.01f}, 1.0f, gainAsDecibelText()));

    return {params.begin(), params.end()};
}
