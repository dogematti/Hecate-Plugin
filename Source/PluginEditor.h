#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"
#include "HecateLookAndFeel.h"

class HecateAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HecateAudioProcessorEditor(HecateAudioProcessor&);
    ~HecateAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // One rotary control: slider + caption + parameter attachment
    struct Knob
    {
        Knob(juce::AudioProcessorValueTreeState& apvts, const char* paramId, const char* caption);

        juce::Slider slider;
        juce::Label label;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };

    HecateAudioProcessor& processor;
    HecateLookAndFeel lookAndFeel;
    juce::Image background;

    std::vector<std::unique_ptr<Knob>> knobs;

    juce::TextButton loadIRButton{"Load IR..."};
    juce::TextButton clearIRButton{"Clear"};
    juce::Label irNameLabel;
    std::unique_ptr<juce::FileChooser> irFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessorEditor)
};
