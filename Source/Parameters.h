#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter ID lives here so the processor and editor never disagree.
namespace param
{
    inline constexpr const char* inputGain     = "inputGain";
    inline constexpr const char* octaveMix     = "octaveMix";
    inline constexpr const char* gateThreshold = "gateThreshold";
    inline constexpr const char* drive         = "drive";
    inline constexpr const char* tone          = "tone";
    inline constexpr const char* bass          = "bass";
    inline constexpr const char* mid           = "mid";
    inline constexpr const char* treble        = "treble";
    inline constexpr const char* presence      = "presence";
    inline constexpr const char* compThreshold = "compThreshold";
    inline constexpr const char* compRatio     = "compRatio";
    inline constexpr const char* chorusRate    = "chorusRate";
    inline constexpr const char* chorusDepth   = "chorusDepth";
    inline constexpr const char* chorusMix     = "chorusMix";
    inline constexpr const char* delayTime     = "delayTime";
    inline constexpr const char* delayFeedback = "delayFeedback";
    inline constexpr const char* delayMix      = "delayMix";
    inline constexpr const char* reverbRoom    = "reverbRoom";
    inline constexpr const char* reverbWidth   = "reverbWidth";
    inline constexpr const char* reverbWet     = "reverbWet";
    inline constexpr const char* reverbDry     = "reverbDry";
    inline constexpr const char* outputGain    = "outputGain";
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
