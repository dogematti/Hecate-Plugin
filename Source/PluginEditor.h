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
    // All controls live on a fixed 1020x612 canvas; the editor scales it
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
        void updateIRLabels();
        void drawMeters(juce::Graphics& g);
        void setPage(int newPage);
        void selectFactoryPreset(int delta);
        void toggleAB();

        // Mic-placement browser (slot A): when the loaded IR sits in a pack
        // laid out as <pack>/<mic>/<position>.wav, the combos step through it
        void refreshIRBrowser();
        void micChanged();
        void positionChanged();

        HecateAudioProcessor& processor;
        juce::Image background;

        std::vector<std::unique_ptr<Knob>> knobs;

        juce::TextButton gateButton{"ON"}, boostButton{"BOOST"}, compButton{"ON"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
            gateAttachment, boostAttachment, compAttachment;

        juce::ComboBox syncBox, clipBox;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
            syncAttachment, clipAttachment;

        juce::ComboBox presetBox;
        juce::TextButton prevPresetButton{"<"}, nextPresetButton{">"};
        juce::TextButton savePresetButton{"Save"};
        juce::TextButton abButton{"A/B: A"};
        juce::Array<juce::File> userPresetFiles;
        juce::ValueTree abStates[2];
        int abIndex = 0;

        juce::TextButton ampTabButton{"AMP"}, fxTabButton{"FX"}, cabTabButton{"CAB"};
        int currentPage = 0;

        juce::TextButton loadIRButton{"Load IR..."}, clearIRButton{"Clear"};
        juce::TextButton loadIR2Button{"Load IR..."}, clearIR2Button{"Clear"};
        juce::Label irNameLabel, ir2NameLabel;
        juce::ComboBox micBox, positionBox;
        juce::Array<juce::File> micDirs, positionFiles;
        std::unique_ptr<juce::FileChooser> fileChooser;

        friend class HecateAudioProcessorEditor;
    };

    void timerCallback() override;

    HecateAudioProcessor& processor;
    HecateLookAndFeel lookAndFeel;
    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessorEditor)
};
