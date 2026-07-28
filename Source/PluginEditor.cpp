#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include "BinaryData.h"

namespace
{
    constexpr int kCanvasW = 1020, kCanvasH = 672;
    constexpr int kKnobW = 80, kKnobH = 140;
    constexpr int kRow1Y = 100, kRow2Y = 304, kRow3Y = 508;

    struct KnobDef { const char* id; const char* caption; int x; int y; };

    constexpr KnobDef kKnobDefs[] = {
        {param::octaveDirect,   "Direct",    18, kRow1Y},
        {param::octaveLevel,    "Octave",   104, kRow1Y},
        {param::gateThreshold,  "Gate",     190, kRow1Y},
        {param::gain,           "Gain",     296, kRow1Y},
        {param::tight,          "Tight",    382, kRow1Y},
        {param::tone,           "Tone",     468, kRow1Y},
        {param::bass,           "Bass",     574, kRow1Y},
        {param::mid,            "Mid",      660, kRow1Y},
        {param::treble,         "Treble",   746, kRow1Y},
        {param::presence,       "Presence", 832, kRow1Y},
        {param::outputGain,     "Output",   926, kRow1Y},

        {param::chorusRate,     "Rate",      18, kRow2Y},
        {param::chorusDepth,    "Depth",    104, kRow2Y},
        {param::chorusMix,      "Mix",      190, kRow2Y},
        {param::delayTime,      "Time",     296, kRow2Y},
        {param::delayFeedback,  "Feedback", 382, kRow2Y},
        {param::delayMix,       "Mix",      468, kRow2Y},
        {param::compThreshold,  "Threshold", 574, kRow2Y},
        {param::compRatio,      "Ratio",    660, kRow2Y},

        {param::reverbRoom,     "Room",      18, kRow3Y},
        {param::reverbWidth,    "Width",    104, kRow3Y},
        {param::reverbDamp,     "Damping",  190, kRow3Y},
        {param::reverbPreDelay, "Pre-Delay", 276, kRow3Y},
        {param::reverbWet,      "Wet",      362, kRow3Y},
        {param::reverbDry,      "Dry",      448, kRow3Y},
    };

    constexpr int kDelayTimeKnobIndex = 14;

    struct PanelDef { int x; int y; int w; int h; const char* title; };

    constexpr PanelDef kPanels[] = {
        { 10,  56, 268, 192, "INPUT"},
        {288,  56, 268, 192, "AMP"},
        {566,  56, 346, 192, "EQ"},
        {922,  56,  88, 192, "OUTPUT"},
        { 10, 260, 268, 192, "CHORUS"},
        {288, 260, 268, 192, "DELAY"},
        {566, 260, 180, 192, "COMPRESSOR"},
        {756, 260, 254, 192, "METERS"},
        { 10, 464, 532, 192, "REVERB"},
        {552, 464, 458, 192, "CABINET IR"},
    };

    const juce::Rectangle<int> kMetersArea{756, 260, 254, 192};
    constexpr int kFactoryPresetIdOffset = 1;
    constexpr int kUserPresetIdOffset = 1000;
}

HecateAudioProcessorEditor::Content::Knob::Knob(juce::AudioProcessorValueTreeState& apvts,
                                                const char* paramId, const char* caption,
                                                juce::Component* popupParent)
    : slider(juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox),
      attachment(apvts, paramId, slider)
{
    slider.setPopupDisplayEnabled(true, true, popupParent);

    if (auto* parameter = apvts.getParameter(paramId))
        slider.setDoubleClickReturnValue(
            true, (double)parameter->convertFrom0to1(parameter->getDefaultValue()));

    label.setText(caption, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(13.0f)));
    label.attachToComponent(&slider, false);
}

HecateAudioProcessorEditor::Content::Content(HecateAudioProcessor& p)
    : processor(p)
{
    background = juce::ImageCache::getFromMemory(BinaryData::background_png,
                                                 BinaryData::background_pngSize);

    for (const auto& def : kKnobDefs)
    {
        knobs.push_back(std::make_unique<Knob>(processor.apvts, def.id, def.caption, this));
        addAndMakeVisible(knobs.back()->slider);
        addAndMakeVisible(knobs.back()->label);
    }

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    addAndMakeVisible(gateButton);
    gateAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::gateOn, gateButton);
    addAndMakeVisible(boostButton);
    boostAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::boost, boostButton);
    addAndMakeVisible(compButton);
    compAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::compOn, compButton);

    addAndMakeVisible(syncBox);
    for (int i = 0; i < (int)std::size(param::delaySyncChoices); ++i)
        syncBox.addItem(param::delaySyncChoices[i], i + 1);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, param::delaySync, syncBox);
    syncBox.onChange = [this] { updateDelayTimeEnablement(); };
    updateDelayTimeEnablement();

    addAndMakeVisible(presetBox);
    presetBox.setTextWhenNothingSelected("Presets...");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= kUserPresetIdOffset)
            loadUserPreset(userPresetFiles[id - kUserPresetIdOffset]);
        else if (id >= kFactoryPresetIdOffset)
            processor.setCurrentProgram(id - kFactoryPresetIdOffset);
    };
    refreshPresetBox();

    addAndMakeVisible(savePresetButton);
    savePresetButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save preset", getUserPresetDirectory().getChildFile("MyPreset.xml"), "*.xml");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file == juce::File())
                    return;
                file.replaceWithText(processor.apvts.state.toXmlString());
                refreshPresetBox();
            });
    };

    addAndMakeVisible(loadIRButton);
    loadIRButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Select an impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
        fileChooser->launchAsync(
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
        irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
    };

    addAndMakeVisible(irNameLabel);
    irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
    irNameLabel.setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);

    setSize(kCanvasW, kCanvasH);
}

void HecateAudioProcessorEditor::Content::refreshPresetBox()
{
    presetBox.clear(juce::dontSendNotification);

    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int)factory.size(); ++i)
        presetBox.addItem(factory[(size_t)i].name, kFactoryPresetIdOffset + i);

    userPresetFiles = getUserPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.xml");
    if (!userPresetFiles.isEmpty())
    {
        presetBox.addSeparator();
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem(userPresetFiles[i].getFileNameWithoutExtension(),
                              kUserPresetIdOffset + i);
    }
}

void HecateAudioProcessorEditor::Content::loadUserPreset(const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse(file))
    {
        processor.apvts.replaceState(juce::ValueTree::fromXml(*xml));

        const auto irPath = processor.apvts.state.getProperty("irPath").toString();
        if (irPath.isNotEmpty() && juce::File(irPath).existsAsFile())
            processor.loadImpulseResponse(juce::File(irPath));
        else
            processor.clearImpulseResponse();

        irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
    }
}

void HecateAudioProcessorEditor::Content::updateDelayTimeEnablement()
{
    const bool freeMode = syncBox.getSelectedItemIndex() <= 0;
    auto& timeKnob = *knobs[(size_t)kDelayTimeKnobIndex];
    timeKnob.slider.setEnabled(freeMode);
    timeKnob.slider.setAlpha(freeMode ? 1.0f : 0.4f);
}

void HecateAudioProcessorEditor::Content::paint(juce::Graphics& g)
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

    drawMeters(g);
}

void HecateAudioProcessorEditor::Content::drawMeters(juce::Graphics& g)
{
    const auto barArea = [](int x) { return juce::Rectangle<float>((float)x, 300.0f, 26.0f, 128.0f); };

    // Output peak, -60..0 dB
    {
        auto area = barArea(800);
        g.setColour(juce::Colour(0xff17141c));
        g.fillRoundedRectangle(area, 3.0f);

        const float levelDb = juce::Decibels::gainToDecibels(processor.getOutputLevel(), -60.0f);
        const float fraction = juce::jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
        auto fill = area.removeFromBottom(area.getHeight() * fraction);
        g.setColour(levelDb > -3.0f ? juce::Colour(0xffc85450) : HecateLookAndFeel::accent);
        g.fillRoundedRectangle(fill, 3.0f);
    }

    // Compressor gain reduction, 0..24 dB from the top
    {
        auto area = barArea(870);
        g.setColour(juce::Colour(0xff17141c));
        g.fillRoundedRectangle(area, 3.0f);

        const float fraction = juce::jlimit(0.0f, 1.0f, processor.getGainReductionDb() / 24.0f);
        auto fill = area.removeFromTop(area.getHeight() * fraction);
        g.setColour(juce::Colour(0xffb08a4a));
        g.fillRoundedRectangle(fill, 3.0f);
    }

    // Gate LED
    {
        const bool open = processor.isGateOpen();
        g.setColour(open ? juce::Colour(0xff6fae6a) : juce::Colour(0xff5a2622));
        g.fillEllipse(931.0f, 310.0f, 18.0f, 18.0f);
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawEllipse(931.0f, 310.0f, 18.0f, 18.0f, 1.0f);
    }

    g.setColour(HecateLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("OUT", 790, 434, 46, 14, juce::Justification::centred, false);
    g.drawText("GR", 860, 434, 46, 14, juce::Justification::centred, false);
    g.drawText("GATE", 917, 434, 46, 14, juce::Justification::centred, false);
}

void HecateAudioProcessorEditor::Content::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->slider.setBounds(kKnobDefs[i].x, kKnobDefs[i].y, kKnobW, kKnobH);

    presetBox.setBounds(20, 14, 250, 26);
    savePresetButton.setBounds(278, 14, 60, 26);

    gateButton.setBounds(214, 60, 54, 20);
    boostButton.setBounds(494, 60, 54, 20);
    syncBox.setBounds(460, 262, 88, 22);
    compButton.setBounds(688, 262, 50, 20);

    loadIRButton.setBounds(572, 508, 110, 30);
    clearIRButton.setBounds(692, 508, 70, 30);
    irNameLabel.setBounds(572, 552, 420, 22);
}

HecateAudioProcessorEditor::HecateAudioProcessorEditor(HecateAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content(p)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(content);

    setResizable(true, true);
    setResizeLimits(kCanvasW / 2, kCanvasH / 2, kCanvasW * 2, kCanvasH * 2);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio((double)kCanvasW / (double)kCanvasH);

    const int savedWidth = processor.apvts.state.getProperty("uiWidth", kCanvasW);
    setSize(savedWidth, savedWidth * kCanvasH / kCanvasW);

    startTimerHz(30);
}

HecateAudioProcessorEditor::~HecateAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void HecateAudioProcessorEditor::resized()
{
    const float scale = (float)getWidth() / (float)kCanvasW;
    content.setTransform(juce::AffineTransform::scale(scale));
    content.setBounds(0, 0, kCanvasW, kCanvasH);

    processor.apvts.state.setProperty("uiWidth", getWidth(), nullptr);
}

void HecateAudioProcessorEditor::timerCallback()
{
    content.repaint(kMetersArea);
}
