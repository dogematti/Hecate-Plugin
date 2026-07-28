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
    class Content : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        explicit Content(HecateAudioProcessor&);
        ~Content() override;

        void paint(juce::Graphics&) override;
        void resized() override;

        void refreshPresetBox();

        // Called from the editor's timer: dirty asterisk + host program sync
        void updateHeaderState();

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

        void parameterChanged(const juce::String&, float) override { dirtyFlag.store(true); }

        void updateTuner();
        void drawTuner(juce::Graphics&);
        void drawEqCurve(juce::Graphics&);
        void rebuildIRCurves();

        void loadUserPreset(const juce::File& file);
        void markPresetLoaded(const juce::String& name, const juce::File& userFile);
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
        juce::TextButton pingButton{"PING"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
            gateAttachment, boostAttachment, compAttachment, pingAttachment;

        // Tap tempo: average of the last few tap intervals sets the delay time
        juce::TextButton tapButton{"TAP"};
        double lastTapMs = 0.0;
        juce::Array<double> tapIntervals;
        void tapTempo();

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

        // Preset bookkeeping: name shown in the combo, the file behind a
        // loaded user preset (empty for factory), dirty tracking
        juce::String currentPresetName{"Default"};
        juce::File currentUserPresetFile;
        juce::StringArray listenedParameterIds;
        std::atomic<bool> dirtyFlag{false};
        bool showingDirty = false;
        int shownProgram = 0;

        juce::TextButton ampTabButton{"AMP"}, fxTabButton{"FX"}, cabTabButton{"CAB"};
        int currentPage = 0;

        juce::TextButton loadIRButton{"Load IR..."}, clearIRButton{"Clear"};
        juce::TextButton loadIR2Button{"Load IR..."}, clearIR2Button{"Clear"};
        juce::Label irNameLabel, ir2NameLabel;
        juce::ComboBox micBox, positionBox;
        juce::Array<juce::File> micDirs, positionFiles;
        std::unique_ptr<juce::FileChooser> fileChooser;

        // Tuner: pulls the processor's input tap, YIN-style detection
        juce::TextButton tunerButton{"TUNER"};
        std::vector<float> tunerSamples = std::vector<float>(4096, 0.0f);
        juce::String tunerNote;
        float tunerCents = 0.0f;
        float tunerFrequency = 0.0f;
        bool tunerHasPitch = false;

        // EQ curve overlay shows while an EQ/power knob is dragged
        int eqDragCount = 0;

        // Cabinet response curves, rebuilt on IR changes
        juce::Path irCurveA, irCurveB;

        friend class HecateAudioProcessorEditor;
    };

    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

    HecateAudioProcessor& processor;
    HecateLookAndFeel lookAndFeel;
    Content content;
    juce::TooltipWindow tooltipWindow{this, 700};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HecateAudioProcessorEditor)
};
