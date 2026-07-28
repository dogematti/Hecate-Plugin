#include "Parameters.h"

namespace
{
    using Attributes = juce::AudioParameterFloatAttributes;

    // Every attribute set defines BOTH directions so typed entry ("-42 dB",
    // "450 Hz", "35 %") parses in hosts and in the UI
    Attributes decibelText()
    {
        return Attributes()
            .withStringFromValueFunction(
                [](float value, int) { return juce::String(value, 1) + " dB"; })
            .withValueFromStringFunction(
                [](const juce::String& text) { return text.getFloatValue(); });
    }

    Attributes gainAsDecibelText()
    {
        return Attributes()
            .withStringFromValueFunction(
                [](float value, int) { return juce::String(juce::Decibels::gainToDecibels(value), 1) + " dB"; })
            .withValueFromStringFunction(
                [](const juce::String& text) { return juce::Decibels::decibelsToGain(text.getFloatValue()); });
    }

    Attributes percentText()
    {
        return Attributes()
            .withStringFromValueFunction(
                [](float value, int) { return juce::String(juce::roundToInt(value * 100.0f)) + " %"; })
            .withValueFromStringFunction(
                [](const juce::String& text) { return text.getFloatValue() * 0.01f; });
    }

    Attributes hertzText()
    {
        return Attributes()
            .withStringFromValueFunction(
                [](float value, int) { return juce::String(juce::roundToInt(value)) + " Hz"; })
            .withValueFromStringFunction(
                [](const juce::String& text) { return text.getFloatValue(); });
    }

    std::unique_ptr<juce::AudioParameterFloat> makeParam(const char* id, const char* name,
                                                         juce::NormalisableRange<float> range,
                                                         float defaultValue, Attributes attributes = {})
    {
        return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                                                           range, defaultValue, attributes);
    }

    std::unique_ptr<juce::AudioParameterBool> makeBool(const char* id, const char* name, bool defaultValue)
    {
        return std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, defaultValue);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(makeParam(param::inputTrim, "Input Trim",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::octaveDirect, "Direct Level",
                               {0.0f, 1.0f, 0.01f}, 1.0f, percentText()));
    params.push_back(makeParam(param::octaveLevel, "Octave Level",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));
    params.push_back(makeBool(param::gateOn, "Gate On", true));
    params.push_back(makeParam(param::gateThreshold, "Gate Threshold",
                               {-80.0f, -20.0f, 0.1f}, -60.0f, decibelText()));

    params.push_back(makeParam(param::gain, "Gain",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    params.push_back(makeParam(param::tight, "Tight",
                               {40.0f, 300.0f, 1.0f}, 100.0f, hertzText()));
    params.push_back(makeBool(param::boost, "Boost", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{param::channel, 1}, "Amp Channel",
        juce::StringArray(param::channelChoices,
                          (int)std::size(param::channelChoices)), 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{param::dropTune, 1}, "Drop Tune",
        juce::StringArray(param::dropTuneChoices,
                          (int)std::size(param::dropTuneChoices)), 0));
    params.push_back(makeParam(param::tone, "Tone",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    params.push_back(makeParam(param::cleanBlend, "Clean Blend",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));

    params.push_back(makeParam(param::bass, "Bass",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::mid, "Mid",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    {
        juce::NormalisableRange<float> midFreqRange(250.0f, 2000.0f, 1.0f);
        midFreqRange.setSkewForCentre(700.0f);
        params.push_back(makeParam(param::midFreq, "Mid Frequency",
                                   midFreqRange, 500.0f, hertzText()));
    }
    params.push_back(makeParam(param::treble, "Treble",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::presence, "Presence",
                               {-12.0f, 12.0f, 0.1f}, 0.0f, decibelText()));
    params.push_back(makeParam(param::sag, "Sag",
                               {0.0f, 1.0f, 0.01f}, 0.2f, percentText()));
    params.push_back(makeParam(param::depth, "Depth",
                               {0.0f, 6.0f, 0.1f}, 2.0f, decibelText()));

    params.push_back(makeBool(param::compOn, "Comp On", false));
    params.push_back(makeParam(param::compThreshold, "Comp Threshold",
                               {-60.0f, 0.0f, 0.1f}, -24.0f, decibelText()));
    params.push_back(makeParam(param::compRatio, "Comp Ratio",
                               {1.0f, 16.0f, 0.1f}, 4.0f,
                               Attributes()
                                   .withStringFromValueFunction(
                                       [](float value, int) { return juce::String(value, 1) + ":1"; })
                                   .withValueFromStringFunction(
                                       [](const juce::String& text) { return text.getFloatValue(); })));

    params.push_back(makeParam(param::chorusRate, "Chorus Rate",
                               {0.1f, 5.0f, 0.01f}, 1.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(value, 2) + " Hz"; })));
    params.push_back(makeParam(param::chorusDepth, "Chorus Depth",
                               {0.0f, 1.0f, 0.01f}, 0.25f, percentText()));
    params.push_back(makeParam(param::chorusDelay, "Chorus Delay",
                               {1.0f, 30.0f, 0.1f}, 7.0f,
                               Attributes()
                                   .withStringFromValueFunction(
                                       [](float value, int) { return juce::String(value, 1) + " ms"; })
                                   .withValueFromStringFunction(
                                       [](const juce::String& text) { return text.getFloatValue(); })));
    params.push_back(makeParam(param::chorusFeedback, "Chorus Feedback",
                               {-0.95f, 0.95f, 0.01f}, 0.0f, percentText()));
    params.push_back(makeParam(param::chorusMix, "Chorus Mix",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{param::delaySync, 1}, "Delay Sync",
        juce::StringArray(param::delaySyncChoices,
                          (int)std::size(param::delaySyncChoices)), 0));
    params.push_back(makeParam(param::delayTime, "Delay Time",
                               {50.0f, 1000.0f, 1.0f}, 350.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(juce::roundToInt(value)) + " ms"; })));
    params.push_back(makeParam(param::delayFeedback, "Delay Feedback",
                               {0.0f, 0.9f, 0.01f}, 0.35f, percentText()));
    {
        juce::NormalisableRange<float> dampRange(1000.0f, 12000.0f, 10.0f);
        dampRange.setSkewForCentre(4500.0f);
        params.push_back(makeParam(param::delayDamp, "Delay Damping",
                                   dampRange, 4500.0f, hertzText()));
    }
    params.push_back(makeBool(param::delayPingPong, "Delay Ping-Pong", false));
    params.push_back(makeParam(param::delayMix, "Delay Mix",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));

    params.push_back(makeParam(param::reverbRoom, "Reverb Room Size",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    params.push_back(makeParam(param::reverbWidth, "Reverb Width",
                               {0.0f, 1.0f, 0.01f}, 1.0f, percentText()));
    params.push_back(makeParam(param::reverbDamp, "Reverb Damping",
                               {0.0f, 1.0f, 0.01f}, 0.4f, percentText()));
    params.push_back(makeParam(param::reverbPreDelay, "Reverb Pre-Delay",
                               {0.0f, 200.0f, 1.0f}, 20.0f,
                               Attributes().withStringFromValueFunction(
                                   [](float value, int) { return juce::String(juce::roundToInt(value)) + " ms"; })));
    params.push_back(makeParam(param::reverbWet, "Reverb Wet",
                               {0.0f, 1.0f, 0.01f}, 0.15f, percentText()));
    params.push_back(makeParam(param::reverbDry, "Reverb Dry",
                               {0.0f, 1.0f, 0.01f}, 0.85f, percentText()));

    params.push_back(makeParam(param::irBlend, "Cab Blend",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));
    {
        juce::NormalisableRange<float> lowCutRange(20.0f, 300.0f, 1.0f);
        lowCutRange.setSkewForCentre(80.0f);
        params.push_back(makeParam(param::cabLowCut, "Cab Low Cut",
                                   lowCutRange, 20.0f, hertzText()));
    }
    {
        juce::NormalisableRange<float> highCutRange(3000.0f, 20000.0f, 10.0f);
        highCutRange.setSkewForCentre(8000.0f);
        params.push_back(makeParam(param::cabHighCut, "Cab High Cut",
                                   highCutRange, 20000.0f, hertzText()));
    }
    params.push_back(makeParam(param::doubler, "Doubler",
                               {0.0f, 1.0f, 0.01f}, 0.0f, percentText()));
    params.push_back(makeParam(param::doublerSpread, "Doubler Spread",
                               {0.5f, 2.0f, 0.01f}, 1.0f, percentText()));
    params.push_back(makeParam(param::doublerDrift, "Doubler Drift",
                               {0.0f, 1.0f, 0.01f}, 0.5f, percentText()));

    params.push_back(makeParam(param::outputGain, "Output Gain",
                               {0.0f, 4.0f, 0.01f}, 1.0f, gainAsDecibelText()));

    return {params.begin(), params.end()};
}
