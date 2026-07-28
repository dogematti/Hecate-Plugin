#include "PluginEditor.h"
#include "Parameters.h"
#include "BinaryData.h"

namespace
{
    constexpr int kKnobW = 80, kKnobH = 140;
    constexpr int kRow1Y = 100, kRow2Y = 304, kRow3Y = 508;

    struct KnobDef { const char* id; const char* caption; int x; int y; };

    constexpr KnobDef kKnobDefs[] = {
        {param::inputGain,     "Input",     20, kRow1Y},
        {param::octaveMix,     "Octave",   112, kRow1Y},
        {param::gateThreshold, "Gate",     204, kRow1Y},
        {param::drive,         "Drive",    316, kRow1Y},
        {param::tone,          "Tone",     408, kRow1Y},
        {param::bass,          "Bass",     520, kRow1Y},
        {param::mid,           "Mid",      612, kRow1Y},
        {param::treble,        "Treble",   704, kRow1Y},
        {param::presence,      "Presence", 796, kRow1Y},
        {param::outputGain,    "Output",   918, kRow1Y},

        {param::chorusRate,    "Rate",      20, kRow2Y},
        {param::chorusDepth,   "Depth",    112, kRow2Y},
        {param::chorusMix,     "Mix",      204, kRow2Y},
        {param::delayTime,     "Time",     316, kRow2Y},
        {param::delayFeedback, "Feedback", 408, kRow2Y},
        {param::delayMix,      "Mix",      500, kRow2Y},
        {param::reverbRoom,    "Room",     612, kRow2Y},
        {param::reverbWidth,   "Width",    704, kRow2Y},
        {param::reverbWet,     "Wet",      796, kRow2Y},
        {param::reverbDry,     "Dry",      888, kRow2Y},

        {param::compThreshold, "Threshold", 20, kRow3Y},
        {param::compRatio,     "Ratio",    112, kRow3Y},
    };

    struct PanelDef { int x; int y; int w; int h; const char* title; };

    constexpr PanelDef kPanels[] = {
        { 10,  56, 284, 192, "INPUT"},
        {306,  56, 192, 192, "DRIVE"},
        {510,  56, 376, 192, "EQ"},
        {908,  56, 100, 192, "OUTPUT"},
        { 10, 260, 284, 192, "CHORUS"},
        {306, 260, 284, 192, "DELAY"},
        {602, 260, 376, 192, "REVERB"},
        { 10, 464, 194, 192, "COMPRESSOR"},
        {214, 464, 794, 192, "CABINET IR"},
    };
}

HecateAudioProcessorEditor::Knob::Knob(juce::AudioProcessorValueTreeState& apvts,
                                       const char* paramId, const char* caption)
    : slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow),
      attachment(apvts, paramId, slider)
{
    label.setText(caption, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(13.0f)));
    label.attachToComponent(&slider, false);
}

HecateAudioProcessorEditor::HecateAudioProcessorEditor(HecateAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel);

    background = juce::ImageCache::getFromMemory(BinaryData::background_png,
                                                 BinaryData::background_pngSize);

    for (const auto& def : kKnobDefs)
    {
        knobs.push_back(std::make_unique<Knob>(processor.apvts, def.id, def.caption));
        addAndMakeVisible(knobs.back()->slider);
        addAndMakeVisible(knobs.back()->label);
    }

    addAndMakeVisible(loadIRButton);
    loadIRButton.onClick = [this]
    {
        irFileChooser = std::make_unique<juce::FileChooser>(
            "Select an impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        irFileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                if (fc.getResult().existsAsFile())
                {
                    processor.loadImpulseResponse(fc.getResult());
                    irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
                }
            });
    };

    addAndMakeVisible(clearIRButton);
    clearIRButton.onClick = [this]
    {
        processor.clearImpulseResponse();
        irNameLabel.setText("No IR loaded", juce::dontSendNotification);
    };

    addAndMakeVisible(irNameLabel);
    irNameLabel.setText(processor.getImpulseResponseName().isNotEmpty()
                            ? processor.getImpulseResponseName() : "No IR loaded",
                        juce::dontSendNotification);
    irNameLabel.setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);

    // Must come after the child components exist — setSize() synchronously calls resized()
    setSize(1020, 672);
}

HecateAudioProcessorEditor::~HecateAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void HecateAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0xff0d0b12));

    // Title, over the moons
    g.setColour(HecateLookAndFeel::accent);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)).withExtraKerningFactor(0.4f));
    g.drawText("HECATE", getLocalBounds().removeFromTop(52), juce::Justification::centred, false);

    // Translucent panels keep the knobs readable while the artwork shows through
    for (const auto& panel : kPanels)
    {
        auto r = juce::Rectangle<float>((float)panel.x, (float)panel.y, (float)panel.w, (float)panel.h);
        g.setColour(juce::Colour(0xff0d0b12).withAlpha(0.70f));
        g.fillRoundedRectangle(r, 8.0f);
        g.setColour(HecateLookAndFeel::accent.withAlpha(0.18f));
        g.drawRoundedRectangle(r.reduced(0.5f), 8.0f, 1.0f);

        g.setColour(HecateLookAndFeel::textDim);
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)).withExtraKerningFactor(0.15f));
        g.drawText(panel.title, panel.x, panel.y + 6, panel.w, 16, juce::Justification::centred, false);
    }
}

void HecateAudioProcessorEditor::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->slider.setBounds(kKnobDefs[i].x, kKnobDefs[i].y, kKnobW, kKnobH);

    loadIRButton.setBounds(234, 508, 110, 30);
    clearIRButton.setBounds(354, 508, 70, 30);
    irNameLabel.setBounds(234, 552, 500, 22);
}
