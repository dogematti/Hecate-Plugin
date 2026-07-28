#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include "BinaryData.h"

namespace
{
    // Canvas matches the artwork's aspect ratio exactly (1619x971), so the
    // triple goddess renders undistorted: art in the upper half, controls
    // in the smoke below.
    constexpr int kCanvasW = 1020, kCanvasH = 612;
    constexpr int kAmpPage = 0, kFxPage = 1, kCabPage = 2;
    constexpr int kRow1Y = 322, kRow2Y = 460;
    constexpr int kKnobSize = 76, kHeroSize = 100;

    struct KnobDef { const char* id; const char* caption; int x; int y; int size; int page; };

    constexpr KnobDef kKnobDefs[] = {
        // AMP page — row 1: input chain into the hero gain
        {param::octaveDirect,   "Direct",    30, kRow1Y, kKnobSize, kAmpPage},
        {param::octaveLevel,    "Octave",   116, kRow1Y, kKnobSize, kAmpPage},
        {param::gateThreshold,  "Gate",     202, kRow1Y, kKnobSize, kAmpPage},
        {param::gain,           "Gain",     320, kRow1Y - 10, kHeroSize, kAmpPage},
        {param::tight,          "Tight",    452, kRow1Y, kKnobSize, kAmpPage},
        {param::tone,           "Tone",     538, kRow1Y, kKnobSize, kAmpPage},
        // AMP page — row 2
        {param::bass,           "Bass",      30, kRow2Y, kKnobSize, kAmpPage},
        {param::mid,            "Mid",      116, kRow2Y, kKnobSize, kAmpPage},
        {param::treble,         "Treble",   202, kRow2Y, kKnobSize, kAmpPage},
        {param::presence,       "Presence", 288, kRow2Y, kKnobSize, kAmpPage},
        {param::compThreshold,  "Thresh",   400, kRow2Y, kKnobSize, kAmpPage},
        {param::compRatio,      "Ratio",    486, kRow2Y, kKnobSize, kAmpPage},
        {param::outputGain,     "Output",   640, kRow2Y, kKnobSize, kAmpPage},

        // FX page
        {param::chorusRate,     "Rate",      30, kRow1Y, kKnobSize, kFxPage},
        {param::chorusDepth,    "Depth",    116, kRow1Y, kKnobSize, kFxPage},
        {param::chorusMix,      "Mix",      202, kRow1Y, kKnobSize, kFxPage},
        {param::delayTime,      "Time",     320, kRow1Y, kKnobSize, kFxPage},
        {param::delayFeedback,  "Feedback", 406, kRow1Y, kKnobSize, kFxPage},
        {param::delayMix,       "Mix",      492, kRow1Y, kKnobSize, kFxPage},
        {param::reverbRoom,     "Room",      30, kRow2Y, kKnobSize, kFxPage},
        {param::reverbWidth,    "Width",    116, kRow2Y, kKnobSize, kFxPage},
        {param::reverbDamp,     "Damping",  202, kRow2Y, kKnobSize, kFxPage},
        {param::reverbPreDelay, "Pre-Dly",  288, kRow2Y, kKnobSize, kFxPage},
        {param::reverbWet,      "Wet",      374, kRow2Y, kKnobSize, kFxPage},
        {param::reverbDry,      "Dry",      460, kRow2Y, kKnobSize, kFxPage},
    };

    constexpr int kDelayTimeKnobIndex = 16;

    // Engraved section rules: small-caps title with a hairline running to x+w
    struct SectionDef { const char* title; int x; int w; int y; int page; };

    constexpr SectionDef kSections[] = {
        {"INPUT",     30, 248, 306, kAmpPage},
        {"AMP",      320, 294, 306, kAmpPage},
        {"EQ",        30, 334, 444, kAmpPage},
        {"DYNAMICS", 400, 162, 444, kAmpPage},
        {"OUTPUT",   640,  76, 444, kAmpPage},

        {"CHORUS",    30, 248, 306, kFxPage},
        {"DELAY",    320, 248, 306, kFxPage},
        {"REVERB",    30, 506, 444, kFxPage},

        {"CABINET",   30, 590, 306, kCabPage},
    };

    const juce::Rectangle<int> kMetersArea{585, 8, 160, 30};
    constexpr int kFactoryPresetIdOffset = 1;
    constexpr int kUserPresetIdOffset = 1000;

    // Strips the shared filename prefix within a folder ("OD-E112-DEMON-DYN-57-")
    // so the position combo shows just the varying part ("P10-50")
    juce::String commonNamePrefix(const juce::Array<juce::File>& files)
    {
        if (files.size() < 2)
            return {};

        juce::String prefix = files[0].getFileNameWithoutExtension();
        for (const auto& file : files)
        {
            const auto name = file.getFileNameWithoutExtension();
            int len = 0;
            const int limit = juce::jmin(prefix.length(), name.length());
            while (len < limit && prefix[len] == name[len])
                ++len;
            prefix = prefix.substring(0, len);
        }

        // Cut back to the last separator so we never split mid-token
        const int lastDash = juce::jmax(prefix.lastIndexOfChar('-'), prefix.lastIndexOfChar('_'));
        return lastDash >= 0 ? prefix.substring(0, lastDash + 1) : juce::String();
    }

    juce::Array<juce::File> wavFilesIn(const juce::File& dir)
    {
        auto files = dir.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.flac");
        files.sort();
        return files;
    }
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

    label.setText(juce::String(caption).toUpperCase(), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(11.0f)).withExtraKerningFactor(0.12f));
    label.setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);
    label.setInterceptsMouseClicks(false, false);
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

    for (auto* tab : {&ampTabButton, &fxTabButton, &cabTabButton})
    {
        tab->setComponentID("tab");
        addAndMakeVisible(*tab);
    }
    ampTabButton.onClick = [this] { setPage(kAmpPage); };
    fxTabButton.onClick = [this] { setPage(kFxPage); };
    cabTabButton.onClick = [this] { setPage(kCabPage); };

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
                    refreshIRBrowser();
                }
            });
    };

    addAndMakeVisible(clearIRButton);
    clearIRButton.onClick = [this]
    {
        processor.clearImpulseResponse();
        irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
        refreshIRBrowser();
    };

    addAndMakeVisible(irNameLabel);
    irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
    irNameLabel.setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);

    addAndMakeVisible(micBox);
    micBox.setTextWhenNoChoicesAvailable("-");
    micBox.onChange = [this] { micChanged(); };

    addAndMakeVisible(positionBox);
    positionBox.setTextWhenNoChoicesAvailable("-");
    positionBox.onChange = [this] { positionChanged(); };

    refreshIRBrowser();
    setPage(kAmpPage);

    setSize(kCanvasW, kCanvasH);
}

void HecateAudioProcessorEditor::Content::setPage(int newPage)
{
    currentPage = newPage;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const bool visible = kKnobDefs[i].page == currentPage;
        knobs[i]->slider.setVisible(visible);
        knobs[i]->label.setVisible(visible);
    }

    for (auto* component : std::initializer_list<juce::Component*>{
             &gateButton, &boostButton, &compButton})
        component->setVisible(currentPage == kAmpPage);
    syncBox.setVisible(currentPage == kFxPage);
    for (auto* component : std::initializer_list<juce::Component*>{
             &loadIRButton, &clearIRButton, &micBox, &positionBox, &irNameLabel})
        component->setVisible(currentPage == kCabPage);

    ampTabButton.setToggleState(currentPage == kAmpPage, juce::dontSendNotification);
    fxTabButton.setToggleState(currentPage == kFxPage, juce::dontSendNotification);
    cabTabButton.setToggleState(currentPage == kCabPage, juce::dontSendNotification);

    repaint();
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
        refreshIRBrowser();
    }
}

void HecateAudioProcessorEditor::Content::refreshIRBrowser()
{
    micBox.clear(juce::dontSendNotification);
    positionBox.clear(juce::dontSendNotification);
    micDirs.clear();
    positionFiles.clear();

    const auto irPath = processor.apvts.state.getProperty("irPath").toString();
    const juce::File irFile(irPath);
    const bool browsable = processor.isUserImpulseResponseLoaded() && irFile.existsAsFile();
    micBox.setEnabled(browsable);
    positionBox.setEnabled(browsable);
    if (!browsable)
        return;

    const auto micDir = irFile.getParentDirectory();
    const auto packDir = micDir.getParentDirectory();

    // Sibling folders that also hold IRs are treated as alternate mics
    for (const auto& dir : packDir.findChildFiles(juce::File::findDirectories, false))
        if (!wavFilesIn(dir).isEmpty())
            micDirs.add(dir);
    micDirs.sort();

    for (int i = 0; i < micDirs.size(); ++i)
    {
        micBox.addItem(micDirs[i].getFileName(), i + 1);
        if (micDirs[i] == micDir)
            micBox.setSelectedId(i + 1, juce::dontSendNotification);
    }
    micBox.setEnabled(micDirs.size() > 1);

    positionFiles = wavFilesIn(micDir);
    const auto prefix = commonNamePrefix(positionFiles);

    for (int i = 0; i < positionFiles.size(); ++i)
    {
        auto display = positionFiles[i].getFileNameWithoutExtension();
        if (display.startsWith(prefix))
            display = display.substring(prefix.length());
        positionBox.addItem(display, i + 1);
        if (positionFiles[i] == irFile)
            positionBox.setSelectedId(i + 1, juce::dontSendNotification);
    }
    positionBox.setEnabled(positionFiles.size() > 1);
}

void HecateAudioProcessorEditor::Content::micChanged()
{
    const int micIndex = micBox.getSelectedId() - 1;
    if (micIndex < 0 || micIndex >= micDirs.size())
        return;

    // Keep the same position in the new mic folder when the pack's naming
    // allows it, otherwise fall back to the first file
    const auto newFiles = wavFilesIn(micDirs[micIndex]);
    if (newFiles.isEmpty())
        return;

    const auto wantedPosition = positionBox.getText();
    const auto prefix = commonNamePrefix(newFiles);

    juce::File target = newFiles[0];
    for (const auto& file : newFiles)
    {
        auto display = file.getFileNameWithoutExtension();
        if (display.startsWith(prefix))
            display = display.substring(prefix.length());
        if (display == wantedPosition)
        {
            target = file;
            break;
        }
    }

    processor.loadImpulseResponse(target);
    irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
    refreshIRBrowser();
}

void HecateAudioProcessorEditor::Content::positionChanged()
{
    const int positionIndex = positionBox.getSelectedId() - 1;
    if (positionIndex < 0 || positionIndex >= positionFiles.size())
        return;

    processor.loadImpulseResponse(positionFiles[positionIndex]);
    irNameLabel.setText(processor.getImpulseResponseName(), juce::dontSendNotification);
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
    // Artwork, aspect-correct
    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0xff0d0b12));

    // Header scrim, then a soft gradient down into the smoke so controls read
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.62f), 0.0f, 0.0f,
                                           juce::Colours::transparentBlack, 0.0f, 72.0f, false));
    g.fillRect(0, 0, kCanvasW, 72);

    g.setGradientFill(juce::ColourGradient(juce::Colours::transparentBlack, 0.0f, 262.0f,
                                           juce::Colours::black.withAlpha(0.68f), 0.0f, (float)kCanvasH, false));
    g.fillRect(0, 262, kCanvasW, kCanvasH - 262);

    // Title, left of the artwork's centre
    g.setColour(HecateLookAndFeel::accent);
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)).withExtraKerningFactor(0.42f));
    g.drawText("HECATE", 20, 8, 170, 28, juce::Justification::centredLeft, false);

    // Engraved section rules
    for (const auto& section : kSections)
    {
        if (section.page != currentPage)
            continue;

        const auto font = juce::Font(juce::FontOptions(11.0f, juce::Font::bold)).withExtraKerningFactor(0.25f);
        g.setFont(font);
        g.setColour(HecateLookAndFeel::accent.withAlpha(0.85f));
        g.drawText(section.title, section.x, section.y - 15, section.w, 13,
                   juce::Justification::centredLeft, false);

        const float textEnd = (float)section.x
                              + juce::GlyphArrangement::getStringWidth(font, section.title) + 10.0f;
        g.setColour(HecateLookAndFeel::accent.withAlpha(0.25f));
        g.fillRect(textEnd, (float)section.y - 9.0f, (float)(section.x + section.w) - textEnd, 1.0f);
    }

    if (currentPage == kCabPage)
    {
        g.setColour(HecateLookAndFeel::textDim);
        g.setFont(juce::Font(juce::FontOptions(11.0f)).withExtraKerningFactor(0.12f));
        g.drawText("MIC", 30, 392, 200, 12, juce::Justification::centredLeft, false);
        g.drawText("POSITION", 340, 392, 200, 12, juce::Justification::centredLeft, false);
    }

    drawMeters(g);
}

// Slim meters built into the header, visible on every tab
void HecateAudioProcessorEditor::Content::drawMeters(juce::Graphics& g)
{
    g.setColour(HecateLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("OUT", 588, 11, 26, 9, juce::Justification::centredRight, false);
    g.drawText("GR", 588, 23, 26, 9, juce::Justification::centredRight, false);

    // Output peak, -60..0 dB
    {
        juce::Rectangle<float> area(620.0f, 12.0f, 80.0f, 7.0f);
        g.setColour(HecateLookAndFeel::surface.withAlpha(0.9f));
        g.fillRoundedRectangle(area, 3.0f);

        const float levelDb = juce::Decibels::gainToDecibels(processor.getOutputLevel(), -60.0f);
        const float fraction = juce::jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
        g.setColour(levelDb > -3.0f ? juce::Colour(0xffc85450) : HecateLookAndFeel::accent);
        g.fillRoundedRectangle(area.removeFromLeft(area.getWidth() * fraction), 3.0f);
    }

    // Compressor gain reduction, 0..24 dB
    {
        juce::Rectangle<float> area(620.0f, 24.0f, 80.0f, 7.0f);
        g.setColour(HecateLookAndFeel::surface.withAlpha(0.9f));
        g.fillRoundedRectangle(area, 3.0f);

        const float fraction = juce::jlimit(0.0f, 1.0f, processor.getGainReductionDb() / 24.0f);
        g.setColour(juce::Colour(0xffb08a4a));
        g.fillRoundedRectangle(area.removeFromLeft(area.getWidth() * fraction), 3.0f);
    }

    // Gate LED
    const bool open = processor.isGateOpen();
    g.setColour(open ? juce::Colour(0xff6fae6a) : juce::Colour(0xff5a2622));
    g.fillEllipse(712.0f, 14.0f, 15.0f, 15.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawEllipse(712.0f, 14.0f, 15.0f, 15.0f, 1.0f);
}

void HecateAudioProcessorEditor::Content::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto& def = kKnobDefs[i];
        knobs[i]->slider.setBounds(def.x, def.y, def.size, def.size);
        knobs[i]->label.setBounds(def.x - 8, def.y + def.size + 2, def.size + 16, 14);
    }

    presetBox.setBounds(200, 10, 200, 24);
    savePresetButton.setBounds(408, 10, 52, 24);
    ampTabButton.setBounds(796, 8, 68, 28);
    fxTabButton.setBounds(868, 8, 68, 28);
    cabTabButton.setBounds(940, 8, 68, 28);

    gateButton.setBounds(228, 290, 46, 18);
    boostButton.setBounds(560, 290, 52, 18);
    compButton.setBounds(514, 428, 46, 18);
    syncBox.setBounds(500, 288, 68, 22);

    loadIRButton.setBounds(30, 332, 130, 30);
    clearIRButton.setBounds(176, 332, 74, 30);
    micBox.setBounds(30, 408, 280, 26);
    positionBox.setBounds(340, 408, 280, 26);
    irNameLabel.setBounds(30, 452, 560, 20);
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
