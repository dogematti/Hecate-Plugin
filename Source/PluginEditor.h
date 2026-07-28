#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"
#include "HecateLookAndFeel.h"

class HecateAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit HecateAudioProcessorEditor(HecateAudioProcessor&);
    ~HecateAudioProcessorEditor() override;

    void resized() override;

private:
    // All controls live on a fixed 1020x672 canvas; the editor scales it
    // as a whole when the window is resized.
    class Content : public juce::Component
    {
    public:
        explicit Content(HecateAudioProcessor&);

        void paint(juce::Graphics&) override;
        void resized() override;

        void refreshPresetBox();

    private:
        // One rotary control: slider + caption + parameter attachment
        struct Knob
        {
            Knob(juce::AudioProcessorValueTreeState& apvts, const char* paramId,
                 const char* caption, juce::Component* popupParent);

            juce::Slider slider;
            juce::Label label;
            juce::AudioProcessorValueTreeState::SliderAttachment attachment;
        };

        void loadUserPreset(const juce::File& file);
        void updateDelayTimeEnablement();
        void drawMeters(juce::Graphics& g);

        HecateAudioProcessor& processor;
        juce::Image background;

        std::vector<std::unique_ptr<Knob>> knobs;

        juce::TextButton gateButton{"ON"}, boostButton{"BOOST"}, compButton{"ON"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
            gateAttachment, boostAttachment, compAttachment;

        juce::ComboBox syncBox;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncAttachment;

        juce::ComboBox presetBox;
        juce::TextButton savePresetButton{"Save"};
        juce::Array<juce::File> userPresetFiles;

        juce::TextButton loadIRButton{"Load IR..."}, clearIRButton{"Clear"};
        juce::Label irNameLabel;
        std::unique_ptr<juce::FileChooser> fileChooser;

        friend class HecateAudioProcessorEditor;
    };

    void timerCallback() override;

    HecateAudioProcessor& processor;
    HecateLookAndFeel lookAndFeel;
    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessorEditor)
};
