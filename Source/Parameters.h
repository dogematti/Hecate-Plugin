#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter ID lives here so the processor and editor never disagree.
namespace param
{
    inline constexpr const char* inputTrim     = "inputTrim";
    inline constexpr const char* octaveDirect  = "octaveDirect";
    inline constexpr const char* octaveLevel   = "octaveLevel";
    inline constexpr const char* gateOn        = "gateOn";
    inline constexpr const char* gateThreshold = "gateThreshold";
    inline constexpr const char* dropTune      = "dropTune";
    inline constexpr const char* gain          = "gain";
    inline constexpr const char* tight         = "tight";
    inline constexpr const char* boost         = "boost";
    inline constexpr const char* channel       = "channel";
    inline constexpr const char* tone          = "tone";
    inline constexpr const char* cleanBlend    = "cleanBlend";
    inline constexpr const char* bass          = "bass";
    inline constexpr const char* mid           = "mid";
    inline constexpr const char* midFreq       = "midFreq";
    inline constexpr const char* treble        = "treble";
    inline constexpr const char* presence      = "presence";
    inline constexpr const char* sag           = "sag";
    inline constexpr const char* depth         = "depth";
    inline constexpr const char* compOn        = "compOn";
    inline constexpr const char* compThreshold = "compThreshold";
    inline constexpr const char* compRatio     = "compRatio";
    inline constexpr const char* chorusRate    = "chorusRate";
    inline constexpr const char* chorusDepth   = "chorusDepth";
    inline constexpr const char* chorusDelay   = "chorusDelay";
    inline constexpr const char* chorusFeedback = "chorusFeedback";
    inline constexpr const char* chorusMix     = "chorusMix";
    inline constexpr const char* delaySync     = "delaySync";
    inline constexpr const char* delayTime     = "delayTime";
    inline constexpr const char* delayFeedback = "delayFeedback";
    inline constexpr const char* delayDamp     = "delayDamp";
    inline constexpr const char* delayPingPong = "delayPingPong";
    inline constexpr const char* delayMix      = "delayMix";
    inline constexpr const char* reverbRoom    = "reverbRoom";
    inline constexpr const char* reverbWidth   = "reverbWidth";
    inline constexpr const char* reverbDamp    = "reverbDamp";
    inline constexpr const char* reverbPreDelay = "reverbPreDelay";
    inline constexpr const char* reverbWet     = "reverbWet";
    inline constexpr const char* reverbDry     = "reverbDry";
    inline constexpr const char* irBlend       = "irBlend";
    inline constexpr const char* cabLowCut     = "cabLowCut";
    inline constexpr const char* cabHighCut    = "cabHighCut";
    inline constexpr const char* doubler       = "doubler";
    inline constexpr const char* doublerSpread = "doublerSpread";
    inline constexpr const char* doublerDrift  = "doublerDrift";
    inline constexpr const char* outputGain    = "outputGain";

    // Delay sync choice order; "Free" uses the milliseconds knob
    inline constexpr const char* delaySyncChoices[] = {"Free", "1/4", "1/8.", "1/8", "1/8T", "1/16"};

    // Amp channel voicings — each index selects a full voicing table row
    // in the Saturator, not just a clip curve. Order matters.
    inline constexpr const char* channelChoices[] = {"Clean", "Rhythm", "Lead", "Thall", "Doom"};

    // Drop-tune amounts; "Off" bypasses the shifter entirely (zero latency)
    inline constexpr const char* dropTuneChoices[] = {"Off", "-1 st", "-2 st", "-3 st", "-4 st"};
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
