#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include "BinaryData.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace
{
    // Canvas matches the artwork's aspect ratio exactly (1619x971), so the
    // triple goddess renders undistorted: art in the upper half, controls
    // in the smoke below.
    constexpr int kCanvasW = 1020, kCanvasH = 612;
    constexpr int kAmpPage = 0, kFxPage = 1, kCabPage = 2;
    constexpr int kRow1Y = 322, kRow2Y = 460;
    constexpr int kKnobSize = 76, kHeroSize = 100;

    struct KnobDef { const char* id; const char* caption; int x; int y; int size; int page; const char* tip; };

    constexpr KnobDef kKnobDefs[] = {
        // AMP page — row 1: input chain into the hero gain
        {param::inputTrim,      "Trim",      30, kRow1Y, kKnobSize, kAmpPage,
         "Input level into the plugin (+/-12 dB). Set so hard picking just reaches the top of the IN meter."},
        {param::octaveDirect,   "Direct",   116, kRow1Y, kKnobSize, kAmpPage,
         "Level of the unprocessed guitar. Turn down with Octave up to play the sub-octave voice alone."},
        {param::octaveLevel,    "Octave",   202, kRow1Y, kKnobSize, kAmpPage,
         "Level of the octave-down voice, like an octave pedal before the amp."},
        {param::gateThreshold,  "Gate",     288, kRow1Y, kKnobSize, kAmpPage,
         "Noise gate threshold. Raise until silence between chugs; the header LED shows gate state."},
        {param::gain,           "Gain",     410, kRow1Y - 10, kHeroSize, kAmpPage,
         "Preamp drive (+12 to +60 dB). Modern metal uses moderate gain plus Boost, not maximum."},
        {param::tight,          "Tight",    542, kRow1Y, kKnobSize, kAmpPage,
         "Pre-drive low cut (40-300 Hz). Higher = tighter chugs; restore weight with Depth."},
        {param::tone,           "Tone",     628, kRow1Y, kKnobSize, kAmpPage,
         "Post-drive brightness sweep, dark to bright."},
        // AMP page — row 2
        {param::bass,           "Bass",      30, kRow2Y, kKnobSize, kAmpPage,
         "Low shelf at 100 Hz."},
        {param::mid,            "Mid",      116, kRow2Y, kKnobSize, kAmpPage,
         "Mid bell cut/boost at the Freq knob's frequency. Metal scoops: -4 to -7 dB around 400-500 Hz."},
        {param::midFreq,        "Freq",     202, kRow2Y, kKnobSize, kAmpPage,
         "Centre of the Mid bell (250 Hz - 2 kHz). Scoop low-mids; keep 700 Hz+ to cut through a mix."},
        {param::treble,         "Treble",   288, kRow2Y, kKnobSize, kAmpPage,
         "High shelf at 8 kHz."},
        {param::presence,       "Presence", 400, kRow2Y, kKnobSize, kAmpPage,
         "Power-amp high shelf at 3.5 kHz — edge and articulation."},
        {param::sag,            "Sag",      486, kRow2Y, kKnobSize, kAmpPage,
         "Power-supply sag: touch-sensitive bloom under hard picking."},
        {param::depth,          "Depth",    572, kRow2Y, kKnobSize, kAmpPage,
         "Power-amp resonance at 100 Hz — chest thump. The partner of the Tight knob."},
        {param::compThreshold,  "Thresh",   678, kRow2Y, kKnobSize, kAmpPage,
         "Compressor threshold (pre-drive sustain compressor; enable with ON)."},
        {param::compRatio,      "Ratio",    764, kRow2Y, kKnobSize, kAmpPage,
         "Compression ratio."},
        {param::outputGain,     "Output",   870, kRow2Y, kKnobSize, kAmpPage,
         "Final level. A safety limiter after this stops host clipping."},

        // FX page
        {param::chorusRate,     "Rate",      30, kRow1Y, kKnobSize, kFxPage,
         "Chorus LFO speed."},
        {param::chorusDepth,    "Depth",    116, kRow1Y, kKnobSize, kFxPage,
         "Chorus modulation depth."},
        {param::chorusMix,      "Mix",      202, kRow1Y, kKnobSize, kFxPage,
         "Chorus wet mix. 0 = off."},
        {param::delayTime,      "Time",     320, kRow1Y, kKnobSize, kFxPage,
         "Delay time in ms. Disabled when Sync is not Free."},
        {param::delayFeedback,  "Feedback", 406, kRow1Y, kKnobSize, kFxPage,
         "Delay repeats; the feedback path darkens each repeat."},
        {param::delayMix,       "Mix",      492, kRow1Y, kKnobSize, kFxPage,
         "Delay wet mix. 0 = off."},
        {param::doubler,        "Amount",   610, kRow1Y, kKnobSize, kFxPage,
         "Quad-track widener: two drifting ghost takes panned wide. Try 30-50% on rhythm."},
        {param::reverbRoom,     "Room",      30, kRow2Y, kKnobSize, kFxPage,
         "Reverb size / decay length."},
        {param::reverbWidth,    "Width",    116, kRow2Y, kKnobSize, kFxPage,
         "Stereo width of the reverb tail."},
        {param::reverbDamp,     "Damping",  202, kRow2Y, kKnobSize, kFxPage,
         "High-frequency absorption of the tail."},
        {param::reverbPreDelay, "Pre-Dly",  288, kRow2Y, kKnobSize, kFxPage,
         "Gap before the tail starts — keeps the riff in front of the reverb."},
        {param::reverbWet,      "Wet",      374, kRow2Y, kKnobSize, kFxPage,
         "Reverb level. 0 = off."},
        {param::reverbDry,      "Dry",      460, kRow2Y, kKnobSize, kFxPage,
         "Dry signal level through the reverb stage."},

        // CAB page
        {param::irBlend,        "Blend",    790, kRow1Y + 10, kKnobSize, kCabPage,
         "Equal-power blend between cabinet A and B. Only active when B is loaded."},
        {param::cabLowCut,      "Low Cut",  790, kRow2Y, kKnobSize, kCabPage,
         "High-pass after the cab — tames boomy IRs. Fully left = off."},
        {param::cabHighCut,     "High Cut", 876, kRow2Y, kKnobSize, kCabPage,
         "Low-pass after the cab — tames fizzy IRs. Fully right = off."},
    };

    constexpr int kDelayTimeKnobIndex = 20;

    // Engraved section rules: small-caps title with a hairline running to x+w
    struct SectionDef { const char* title; int x; int w; int y; int page; };

    constexpr SectionDef kSections[] = {
        {"INPUT",      30, 334, 306, kAmpPage},
        {"AMP",       410, 294, 306, kAmpPage},
        {"VOICING",   740, 250, 306, kAmpPage},
        {"EQ",         30, 334, 444, kAmpPage},
        {"POWER",     400, 248, 444, kAmpPage},
        {"DYNAMICS",  678, 162, 444, kAmpPage},
        {"OUTPUT",    870,  76, 444, kAmpPage},

        {"CHORUS",     30, 248, 306, kFxPage},
        {"DELAY",     320, 248, 306, kFxPage},
        {"DOUBLER",   610,  76, 306, kFxPage},
        {"REVERB",     30, 506, 444, kFxPage},

        {"CABINET A",  30, 400, 306, kCabPage},
        {"CABINET B", 470, 280, 306, kCabPage},
        {"BLEND",     790, 200, 306, kCabPage},
        {"IR TRIM",   790, 200, 444, kCabPage},
        {"RESPONSE",   30, 730, 444, kCabPage},
    };

    const juce::Rectangle<int> kTunerArea{370, 82, 280, 180};
    const juce::Rectangle<int> kEqCurveArea{310, 82, 400, 180};
    const juce::Rectangle<int> kIRCurveArea{30, 458, 730, 132};

    // Shared log-frequency x mapping for the curve displays (40 Hz .. 12 kHz)
    float frequencyToX(float freq, juce::Rectangle<float> area)
    {
        return area.getX() + area.getWidth() * std::log(freq / 40.0f) / std::log(300.0f);
    }

    const juce::Rectangle<int> kMetersArea{580, 4, 164, 36};
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
        knobs.back()->slider.setTooltip(def.tip);
        addAndMakeVisible(knobs.back()->slider);
        addAndMakeVisible(knobs.back()->label);
    }

    // Dirty tracking: listen to every parameter
    for (auto* parameter : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            listenedParameterIds.add(ranged->paramID);
    for (const auto& id : listenedParameterIds)
        processor.apvts.addParameterListener(id, this);

    // A/B starts with both slots holding the current state
    abStates[0] = processor.apvts.copyState();
    abStates[1] = processor.apvts.copyState();

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    addAndMakeVisible(gateButton);
    gateAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::gateOn, gateButton);
    addAndMakeVisible(boostButton);
    boostAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::boost, boostButton);
    addAndMakeVisible(compButton);
    compAttachment = std::make_unique<ButtonAttachment>(processor.apvts, param::compOn, compButton);

    addAndMakeVisible(syncBox);
    for (int i = 0; i < (int)std::size(param::delaySyncChoices); ++i)
        syncBox.addItem(param::delaySyncChoices[i], i + 1);
    syncAttachment = std::make_unique<ComboBoxAttachment>(processor.apvts, param::delaySync, syncBox);
    syncBox.onChange = [this] { updateDelayTimeEnablement(); };
    updateDelayTimeEnablement();

    addAndMakeVisible(clipBox);
    for (int i = 0; i < (int)std::size(param::clipModeChoices); ++i)
        clipBox.addItem(param::clipModeChoices[i], i + 1);
    clipAttachment = std::make_unique<ComboBoxAttachment>(processor.apvts, param::clipMode, clipBox);

    addAndMakeVisible(presetBox);
    presetBox.setTextWhenNothingSelected("Default");
    presetBox.setTooltip("Factory and user presets. * marks unsaved edits.");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= kUserPresetIdOffset)
        {
            loadUserPreset(userPresetFiles[id - kUserPresetIdOffset]);
        }
        else if (id >= kFactoryPresetIdOffset)
        {
            const int program = id - kFactoryPresetIdOffset;
            processor.setCurrentProgram(program);
            markPresetLoaded(getFactoryPresets()[(size_t)program].name, {});
        }
    };
    refreshPresetBox();

    addAndMakeVisible(prevPresetButton);
    prevPresetButton.onClick = [this] { selectFactoryPreset(-1); };
    addAndMakeVisible(nextPresetButton);
    nextPresetButton.onClick = [this] { selectFactoryPreset(1); };

    addAndMakeVisible(savePresetButton);
    savePresetButton.setTooltip("Save the current sound. Overwrites the loaded user preset, or asks for a name.");
    savePresetButton.onClick = [this]
    {
        // Save-in-place when a user preset is loaded; save-as otherwise
        if (currentUserPresetFile.existsAsFile())
        {
            currentUserPresetFile.replaceWithText(processor.apvts.state.toXmlString());
            markPresetLoaded(currentUserPresetFile.getFileNameWithoutExtension(), currentUserPresetFile);
            return;
        }

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
                markPresetLoaded(file.getFileNameWithoutExtension(), file);
            });
    };

    addAndMakeVisible(abButton);
    abButton.onClick = [this] { toggleAB(); };

    for (auto* tab : {&ampTabButton, &fxTabButton, &cabTabButton})
    {
        tab->setComponentID("tab");
        addAndMakeVisible(*tab);
    }
    ampTabButton.onClick = [this] { setPage(kAmpPage); };
    fxTabButton.onClick = [this] { setPage(kFxPage); };
    cabTabButton.onClick = [this] { setPage(kCabPage); };

    auto configureLoad = [this](juce::TextButton& button, int slot)
    {
        addAndMakeVisible(button);
        button.onClick = [this, slot]
        {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Select an impulse response", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, slot](const juce::FileChooser& fc)
                {
                    if (fc.getResult().existsAsFile())
                    {
                        processor.loadImpulseResponse(fc.getResult(), slot);
                        updateIRLabels();
                        refreshIRBrowser();
                    }
                });
        };
    };
    configureLoad(loadIRButton, 0);
    configureLoad(loadIR2Button, 1);

    auto configureClear = [this](juce::TextButton& button, int slot)
    {
        addAndMakeVisible(button);
        button.onClick = [this, slot]
        {
            processor.clearImpulseResponse(slot);
            updateIRLabels();
            refreshIRBrowser();
        };
    };
    configureClear(clearIRButton, 0);
    configureClear(clearIR2Button, 1);

    for (auto* label : {&irNameLabel, &ir2NameLabel})
    {
        addAndMakeVisible(*label);
        label->setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);
    }

    addAndMakeVisible(micBox);
    micBox.setTextWhenNoChoicesAvailable("-");
    micBox.onChange = [this] { micChanged(); };

    addAndMakeVisible(positionBox);
    positionBox.setTextWhenNoChoicesAvailable("-");
    positionBox.onChange = [this] { positionChanged(); };

    addAndMakeVisible(tunerButton);
    tunerButton.setClickingTogglesState(true);
    tunerButton.setTooltip("Chromatic tuner on the raw input. Mute your DAW track while tuning.");
    tunerButton.onClick = [this] { repaint(); };

    // While an EQ or power-section knob is dragged, show the response curve
    for (size_t i = 0; i < std::size(kKnobDefs); ++i)
    {
        const juce::String id(kKnobDefs[i].id);
        if (id == param::bass || id == param::mid || id == param::midFreq
            || id == param::treble || id == param::presence || id == param::depth)
        {
            knobs[i]->slider.onDragStart = [this] { ++eqDragCount; repaint(); };
            knobs[i]->slider.onDragEnd = [this] { --eqDragCount; repaint(); };
        }
    }

    abButton.setTooltip("Compare two settings: edits go to the shown slot, click to switch.");
    boostButton.setTooltip("Screamer-style boost: pre-clip low cut plus a 750 Hz push. The metal recipe.");
    gateButton.setTooltip("Noise gate on/off.");
    compButton.setTooltip("Sustain compressor on/off (sits before the drive).");
    clipBox.setTooltip("Output stage voicing: Tube (soft), Modern (tight), Fuzz (doom).");
    syncBox.setTooltip("Delay sync: Free uses the Time knob, note values follow the host tempo.");

    updateIRLabels();
    refreshIRBrowser();
    setPage(kAmpPage);
    markPresetLoaded("Default", {});

    setSize(kCanvasW, kCanvasH);
}

HecateAudioProcessorEditor::Content::~Content()
{
    for (const auto& id : listenedParameterIds)
        processor.apvts.removeParameterListener(id, this);
}

void HecateAudioProcessorEditor::Content::markPresetLoaded(const juce::String& name,
                                                           const juce::File& userFile)
{
    currentPresetName = name;
    currentUserPresetFile = userFile;
    shownProgram = processor.getCurrentProgram();
    dirtyFlag.store(false);
    showingDirty = false;
    presetBox.setSelectedId(0, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected(name);
    presetBox.repaint();
}

void HecateAudioProcessorEditor::Content::updateHeaderState()
{
    // Follow host-initiated program changes (MIDI, DAW program lane)
    const int program = processor.getCurrentProgram();
    if (program != shownProgram)
        markPresetLoaded(getFactoryPresets()[(size_t)program].name, {});

    const bool dirty = dirtyFlag.load();
    if (dirty != showingDirty)
    {
        showingDirty = dirty;
        presetBox.setTextWhenNothingSelected(currentPresetName + (dirty ? " *" : ""));
        presetBox.repaint();
    }
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
             &gateButton, &boostButton, &compButton, &clipBox})
        component->setVisible(currentPage == kAmpPage);
    syncBox.setVisible(currentPage == kFxPage);
    for (auto* component : std::initializer_list<juce::Component*>{
             &loadIRButton, &clearIRButton, &loadIR2Button, &clearIR2Button,
             &micBox, &positionBox, &irNameLabel, &ir2NameLabel})
        component->setVisible(currentPage == kCabPage);

    ampTabButton.setToggleState(currentPage == kAmpPage, juce::dontSendNotification);
    fxTabButton.setToggleState(currentPage == kFxPage, juce::dontSendNotification);
    cabTabButton.setToggleState(currentPage == kCabPage, juce::dontSendNotification);

    repaint();
}

void HecateAudioProcessorEditor::Content::selectFactoryPreset(int delta)
{
    const int count = (int)getFactoryPresets().size();
    const int next = ((processor.getCurrentProgram() + delta) % count + count) % count;
    processor.setCurrentProgram(next);
    presetBox.setSelectedId(kFactoryPresetIdOffset + next, juce::dontSendNotification);
}

void HecateAudioProcessorEditor::Content::toggleAB()
{
    abStates[abIndex] = processor.apvts.copyState();
    abIndex ^= 1;

    if (abStates[abIndex].isValid())
    {
        processor.apvts.replaceState(abStates[abIndex].createCopy());
        processor.reloadImpulseResponsesFromState();
        updateIRLabels();
        refreshIRBrowser();
    }

    abButton.setButtonText(abIndex == 0 ? "A/B: A" : "A/B: B");
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
        processor.reloadImpulseResponsesFromState();
        updateIRLabels();
        refreshIRBrowser();
        markPresetLoaded(file.getFileNameWithoutExtension(), file);
    }
}

void HecateAudioProcessorEditor::Content::updateIRLabels()
{
    const juce::Colour missing(0xffc85450);
    const char* pathProperties[] = {"irPath", "irPath2"};
    juce::Label* labels[] = {&irNameLabel, &ir2NameLabel};

    for (int slot = 0; slot < 2; ++slot)
    {
        const auto irPath = processor.apvts.state.getProperty(pathProperties[slot]).toString();
        const bool loaded = processor.isUserImpulseResponseLoaded(slot);

        if (irPath.isNotEmpty() && !loaded)
        {
            // The preset/session references an IR this machine doesn't have
            labels[slot]->setText("Missing: " + juce::File(irPath).getFileName()
                                      + " — use Load IR to relocate",
                                  juce::dontSendNotification);
            labels[slot]->setColour(juce::Label::textColourId, missing);
        }
        else
        {
            const auto name = processor.getImpulseResponseName(slot);
            labels[slot]->setText(name.isNotEmpty() ? name : "Empty", juce::dontSendNotification);
            labels[slot]->setColour(juce::Label::textColourId, HecateLookAndFeel::textDim);
        }
    }

    rebuildIRCurves();
}

namespace
{
    // Magnitude response of an impulse as a drawable path over the shared
    // log-frequency axis, normalised so its peak sits at the top of the area
    juce::Path buildResponsePath(const float* samples, int numSamples,
                                 double sampleRate, juce::Rectangle<float> area)
    {
        constexpr int fftOrder = 14;
        constexpr int fftSize = 1 << fftOrder;

        std::vector<float> data((size_t)fftSize * 2, 0.0f);
        std::copy(samples, samples + juce::jmin(numSamples, fftSize), data.begin());

        juce::dsp::FFT fft(fftOrder);
        fft.performFrequencyOnlyForwardTransform(data.data());

        constexpr int points = 220;
        constexpr float rangeDb = 36.0f;
        std::array<float, points> db{};
        float maxDb = -200.0f;

        for (int p = 0; p < points; ++p)
        {
            const float freq = 40.0f * std::pow(300.0f, (float)p / (points - 1));
            const int bin = juce::jlimit(1, fftSize / 2 - 1,
                                         (int)std::round(freq / sampleRate * fftSize));
            db[(size_t)p] = juce::Decibels::gainToDecibels(data[(size_t)bin], -100.0f);
            maxDb = juce::jmax(maxDb, db[(size_t)p]);
        }

        juce::Path path;
        for (int p = 0; p < points; ++p)
        {
            const float freq = 40.0f * std::pow(300.0f, (float)p / (points - 1));
            const float normalised = juce::jlimit(0.0f, 1.0f, (maxDb - db[(size_t)p]) / rangeDb);
            const juce::Point<float> point(frequencyToX(freq, area),
                                           area.getY() + 4.0f + normalised * (area.getHeight() - 8.0f));
            if (p == 0)
                path.startNewSubPath(point);
            else
                path.lineTo(point);
        }
        return path;
    }
}

void HecateAudioProcessorEditor::Content::rebuildIRCurves()
{
    irCurveA.clear();
    irCurveB.clear();

    const auto area = kIRCurveArea.toFloat();
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    auto pathForFile = [&](const juce::String& propertyName) -> juce::Path
    {
        const auto irPath = processor.apvts.state.getProperty(propertyName).toString();
        if (irPath.isEmpty())
            return {};
        std::unique_ptr<juce::AudioFormatReader> reader(
            formats.createReaderFor(juce::File(irPath)));
        if (reader == nullptr)
            return {};

        const int numSamples = (int)juce::jmin<juce::int64>(reader->lengthInSamples, 1 << 14);
        juce::AudioBuffer<float> ir(1, numSamples);
        reader->read(&ir, 0, numSamples, 0, true, false);
        return buildResponsePath(ir.getReadPointer(0), numSamples, reader->sampleRate, area);
    };

    if (processor.isUserImpulseResponseLoaded(0))
    {
        irCurveA = pathForFile("irPath");
    }
    else
    {
        const auto& builtIn = processor.getDefaultCabImpulse();
        if (builtIn.getNumSamples() > 0)
            irCurveA = buildResponsePath(builtIn.getReadPointer(0), builtIn.getNumSamples(),
                                         processor.getSampleRate() > 0.0
                                             ? processor.getSampleRate() : 44100.0,
                                         area);
    }

    if (processor.isUserImpulseResponseLoaded(1))
        irCurveB = pathForFile("irPath2");
}

void HecateAudioProcessorEditor::Content::refreshIRBrowser()
{
    micBox.clear(juce::dontSendNotification);
    positionBox.clear(juce::dontSendNotification);
    micDirs.clear();
    positionFiles.clear();

    const auto irPath = processor.apvts.state.getProperty("irPath").toString();
    const juce::File irFile(irPath);
    const bool browsable = processor.isUserImpulseResponseLoaded(0) && irFile.existsAsFile();
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

    processor.loadImpulseResponse(target, 0);
    updateIRLabels();
    refreshIRBrowser();
}

void HecateAudioProcessorEditor::Content::positionChanged()
{
    const int positionIndex = positionBox.getSelectedId() - 1;
    if (positionIndex < 0 || positionIndex >= positionFiles.size())
        return;

    processor.loadImpulseResponse(positionFiles[positionIndex], 0);
    updateIRLabels();
}

// YIN-style pitch detection on the raw input tap
void HecateAudioProcessorEditor::Content::updateTuner()
{
    processor.readTunerBuffer(tunerSamples.data(), (int)tunerSamples.size());

    const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    constexpr int window = 2048;
    const float* x = tunerSamples.data();

    float rms = 0.0f;
    for (int i = 0; i < window; ++i)
        rms += x[i] * x[i];
    rms = std::sqrt(rms / window);

    if (rms < 0.003f)
    {
        tunerHasPitch = false;
        return;
    }

    const int minLag = juce::jmax(2, (int)(sampleRate / 500.0));   // up to 500 Hz
    const int maxLag = juce::jmin((int)tunerSamples.size() - window - 1,
                                  (int)(sampleRate / 45.0));       // down to 45 Hz

    // Cumulative-mean-normalised difference; first dip under threshold wins
    float cumulative = 0.0f;
    int bestLag = -1;
    float bestValue = 1.0e9f;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        float difference = 0.0f;
        for (int i = 0; i < window; ++i)
        {
            const float d = x[i] - x[i + lag];
            difference += d * d;
        }

        cumulative += difference;
        const float normalised = difference * (float)(lag - minLag + 1) / juce::jmax(1.0e-12f, cumulative);

        if (normalised < bestValue)
        {
            bestValue = normalised;
            bestLag = lag;
        }
        if (normalised < 0.1f)
        {
            bestLag = lag;
            break;
        }
    }

    if (bestLag <= 0)
    {
        tunerHasPitch = false;
        return;
    }

    const float frequency = (float)(sampleRate / bestLag);
    const float midi = 69.0f + 12.0f * std::log2(frequency / 440.0f);
    const int nearest = juce::roundToInt(midi);

    static const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F",
                                      "F#", "G", "G#", "A", "A#", "B"};
    tunerNote = juce::String(noteNames[((nearest % 12) + 12) % 12]) + juce::String(nearest / 12 - 1);
    tunerCents = (midi - (float)nearest) * 100.0f;
    tunerFrequency = frequency;
    tunerHasPitch = true;
}

void HecateAudioProcessorEditor::Content::drawTuner(juce::Graphics& g)
{
    const auto area = kTunerArea.toFloat();
    g.setColour(HecateLookAndFeel::surface.withAlpha(0.92f));
    g.fillRoundedRectangle(area, 10.0f);
    g.setColour(HecateLookAndFeel::accent.withAlpha(0.4f));
    g.drawRoundedRectangle(area.reduced(0.5f), 10.0f, 1.0f);

    if (!tunerHasPitch)
    {
        g.setColour(HecateLookAndFeel::textDim);
        g.setFont(juce::Font(juce::FontOptions(40.0f, juce::Font::bold)));
        g.drawText("-", kTunerArea, juce::Justification::centred, false);
        return;
    }

    const bool inTune = std::abs(tunerCents) < 5.0f;
    g.setColour(inTune ? juce::Colour(0xff6fae6a) : HecateLookAndFeel::textBright);
    g.setFont(juce::Font(juce::FontOptions(56.0f, juce::Font::bold)));
    g.drawText(tunerNote, kTunerArea.withTrimmedBottom(70), juce::Justification::centred, false);

    g.setColour(HecateLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(juce::String(tunerFrequency, 1) + " Hz",
               kTunerArea.withTrimmedTop(96).withHeight(16), juce::Justification::centred, false);

    // Cents bar: -50..+50, needle green in the +/-5 window
    const juce::Rectangle<float> bar(area.getX() + 30.0f, area.getBottom() - 42.0f,
                                     area.getWidth() - 60.0f, 8.0f);
    g.setColour(juce::Colour(0xff17141c));
    g.fillRoundedRectangle(bar, 4.0f);
    g.setColour(HecateLookAndFeel::accentDim.withAlpha(0.6f));
    g.fillRect(bar.getCentreX() - 0.5f, bar.getY() - 4.0f, 1.0f, bar.getHeight() + 8.0f);

    const float position = bar.getCentreX()
                           + juce::jlimit(-1.0f, 1.0f, tunerCents / 50.0f) * (bar.getWidth() * 0.5f);
    g.setColour(inTune ? juce::Colour(0xff6fae6a) : juce::Colour(0xffc85450));
    g.fillRoundedRectangle(position - 2.0f, bar.getY() - 5.0f, 4.0f, bar.getHeight() + 10.0f, 2.0f);

    g.setColour(HecateLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(juce::String(tunerCents > 0 ? "+" : "") + juce::String(tunerCents, 1) + " ct",
               (int)bar.getX(), (int)bar.getBottom() + 6, (int)bar.getWidth(), 12,
               juce::Justification::centred, false);
}

// Combined post-drive tone curve: EQ bands plus the power amp's presence
// and depth, drawn while any of those knobs is being dragged
void HecateAudioProcessorEditor::Content::drawEqCurve(juce::Graphics& g)
{
    const auto area = kEqCurveArea.toFloat();
    g.setColour(HecateLookAndFeel::surface.withAlpha(0.9f));
    g.fillRoundedRectangle(area, 10.0f);
    g.setColour(HecateLookAndFeel::accent.withAlpha(0.4f));
    g.drawRoundedRectangle(area.reduced(0.5f), 10.0f, 1.0f);

    const double sampleRate = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 44100.0;
    auto raw = [this](const char* id) { return processor.apvts.getRawParameterValue(id)->load(); };

    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    const Coefficients::Ptr stages[] = {
        Coefficients::makeLowShelf(sampleRate, 100.0f, 0.707f,
                                   juce::Decibels::decibelsToGain(raw(param::bass))),
        Coefficients::makePeakFilter(sampleRate, juce::jlimit(100.0f, 4000.0f, raw(param::midFreq)),
                                     1.0f, juce::Decibels::decibelsToGain(raw(param::mid))),
        Coefficients::makeHighShelf(sampleRate, 8000.0f, 0.707f,
                                    juce::Decibels::decibelsToGain(raw(param::treble))),
        Coefficients::makeHighShelf(sampleRate, 3500.0f, 0.8f,
                                    juce::Decibels::decibelsToGain(raw(param::presence))),
        Coefficients::makePeakFilter(sampleRate, 100.0f, 0.8f,
                                     juce::Decibels::decibelsToGain(raw(param::depth))),
    };

    const auto plot = area.reduced(14.0f, 16.0f);

    // 0 dB line and octave grid
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRect(plot.getX(), plot.getCentreY(), plot.getWidth(), 1.0f);
    for (float freq : {100.0f, 1000.0f, 10000.0f})
        g.fillRect(frequencyToX(freq, plot), plot.getY(), 1.0f, plot.getHeight());

    juce::Path curve;
    constexpr int points = 160;
    constexpr float rangeDb = 15.0f;

    for (int p = 0; p < points; ++p)
    {
        const float freq = 40.0f * std::pow(300.0f, (float)p / (points - 1));
        double magnitude = 1.0;
        for (const auto& stage : stages)
            magnitude *= stage->getMagnitudeForFrequency(freq, sampleRate);

        const float db = juce::jlimit(-rangeDb, rangeDb,
                                      (float)juce::Decibels::gainToDecibels(magnitude));
        const juce::Point<float> point(frequencyToX(freq, plot),
                                       plot.getCentreY() - db / rangeDb * plot.getHeight() * 0.5f);
        if (p == 0)
            curve.startNewSubPath(point);
        else
            curve.lineTo(point);
    }

    g.setColour(HecateLookAndFeel::accent);
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
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

#ifdef HECATE_VERSION
    g.setColour(HecateLookAndFeel::textDim.withAlpha(0.8f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("v" HECATE_VERSION, 22, 36, 160, 11, juce::Justification::centredLeft, false);
#endif

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
        g.drawText("MIC", 30, 398, 180, 12, juce::Justification::centredLeft, false);
        g.drawText("POSITION", 230, 398, 200, 12, juce::Justification::centredLeft, false);

        // Cabinet response display
        const auto plot = kIRCurveArea.toFloat();
        g.setColour(HecateLookAndFeel::surface.withAlpha(0.75f));
        g.fillRoundedRectangle(plot, 6.0f);

        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        for (float freq : {100.0f, 1000.0f, 10000.0f})
        {
            const float x = frequencyToX(freq, plot);
            g.fillRect(x, plot.getY(), 1.0f, plot.getHeight());
            g.setColour(HecateLookAndFeel::textDim.withAlpha(0.7f));
            g.drawText(freq >= 1000.0f ? juce::String(freq / 1000.0f, 0) + "k" : juce::String((int)freq),
                       (int)x + 3, (int)plot.getBottom() - 13, 30, 11,
                       juce::Justification::centredLeft, false);
            g.setColour(juce::Colours::white.withAlpha(0.07f));
        }

        if (!irCurveB.isEmpty())
        {
            g.setColour(HecateLookAndFeel::textBright.withAlpha(0.65f));
            g.strokePath(irCurveB, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));
        }
        g.setColour(HecateLookAndFeel::accent);
        g.strokePath(irCurveA, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));

        // Trim filter markers
        const float lowCut = processor.apvts.getRawParameterValue(param::cabLowCut)->load();
        const float highCut = processor.apvts.getRawParameterValue(param::cabHighCut)->load();
        g.setColour(juce::Colour(0xffc85450).withAlpha(0.75f));
        if (lowCut > 21.0f && lowCut >= 40.0f)
            g.fillRect(frequencyToX(lowCut, plot), plot.getY(), 1.5f, plot.getHeight());
        if (highCut < 19999.0f && highCut <= 12000.0f)
            g.fillRect(frequencyToX(highCut, plot), plot.getY(), 1.5f, plot.getHeight());

        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        g.setColour(HecateLookAndFeel::accent);
        g.drawText("A", (int)plot.getRight() - 34, (int)plot.getY() + 6, 12, 12,
                   juce::Justification::centred, false);
        if (!irCurveB.isEmpty())
        {
            g.setColour(HecateLookAndFeel::textBright.withAlpha(0.8f));
            g.drawText("B", (int)plot.getRight() - 18, (int)plot.getY() + 6, 12, 12,
                       juce::Justification::centred, false);
        }
    }

    drawMeters(g);

    // Overlays over the artwork area
    if (tunerButton.getToggleState())
        drawTuner(g);
    else if (eqDragCount > 0 && currentPage == kAmpPage)
        drawEqCurve(g);
}

// Slim meters built into the header, visible on every tab
void HecateAudioProcessorEditor::Content::drawMeters(juce::Graphics& g)
{
    g.setColour(HecateLookAndFeel::textDim);
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText("IN", 588, 6, 26, 8, juce::Justification::centredRight, false);
    g.drawText("OUT", 588, 16, 26, 8, juce::Justification::centredRight, false);
    g.drawText("GR", 588, 26, 26, 8, juce::Justification::centredRight, false);

    auto drawLevelBar = [&g](float y, float levelDb, juce::Colour colour)
    {
        juce::Rectangle<float> area(620.0f, y, 80.0f, 6.0f);
        g.setColour(HecateLookAndFeel::surface.withAlpha(0.9f));
        g.fillRoundedRectangle(area, 3.0f);
        const float fraction = juce::jlimit(0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
        g.setColour(colour);
        g.fillRoundedRectangle(area.removeFromLeft(area.getWidth() * fraction), 3.0f);
    };

    const float inDb = juce::Decibels::gainToDecibels(processor.getInputLevel(), -60.0f);
    drawLevelBar(7.0f, inDb, HecateLookAndFeel::textDim);

    const float outDb = juce::Decibels::gainToDecibels(processor.getOutputLevel(), -60.0f);
    drawLevelBar(17.0f, outDb, outDb > -3.0f ? juce::Colour(0xffc85450) : HecateLookAndFeel::accent);

    // Compressor gain reduction, 0..24 dB
    {
        juce::Rectangle<float> area(620.0f, 27.0f, 80.0f, 6.0f);
        g.setColour(HecateLookAndFeel::surface.withAlpha(0.9f));
        g.fillRoundedRectangle(area, 3.0f);
        const float fraction = juce::jlimit(0.0f, 1.0f, processor.getGainReductionDb() / 24.0f);
        g.setColour(juce::Colour(0xffb08a4a));
        g.fillRoundedRectangle(area.removeFromLeft(area.getWidth() * fraction), 3.0f);
    }

    // Gate LED
    const bool open = processor.isGateOpen();
    g.setColour(open ? juce::Colour(0xff6fae6a) : juce::Colour(0xff5a2622));
    g.fillEllipse(712.0f, 11.0f, 15.0f, 15.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawEllipse(712.0f, 11.0f, 15.0f, 15.0f, 1.0f);
}

void HecateAudioProcessorEditor::Content::resized()
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto& def = kKnobDefs[i];
        knobs[i]->slider.setBounds(def.x, def.y, def.size, def.size);
        knobs[i]->label.setBounds(def.x - 8, def.y + def.size + 2, def.size + 16, 14);
    }

    presetBox.setBounds(200, 10, 160, 24);
    prevPresetButton.setBounds(364, 10, 22, 24);
    nextPresetButton.setBounds(388, 10, 22, 24);
    savePresetButton.setBounds(416, 10, 44, 24);
    abButton.setBounds(466, 10, 56, 24);
    tunerButton.setBounds(526, 10, 52, 24);

    ampTabButton.setBounds(796, 8, 68, 28);
    fxTabButton.setBounds(868, 8, 68, 28);
    cabTabButton.setBounds(940, 8, 68, 28);

    gateButton.setBounds(318, 288, 46, 18);
    boostButton.setBounds(740, 318, 56, 20);
    clipBox.setBounds(740, 348, 96, 22);
    compButton.setBounds(796, 426, 44, 16);
    syncBox.setBounds(500, 286, 68, 20);

    loadIRButton.setBounds(30, 332, 120, 30);
    clearIRButton.setBounds(160, 332, 70, 30);
    irNameLabel.setBounds(30, 370, 380, 18);
    micBox.setBounds(30, 412, 180, 24);
    positionBox.setBounds(230, 412, 200, 24);

    loadIR2Button.setBounds(470, 332, 120, 30);
    clearIR2Button.setBounds(600, 332, 70, 30);
    ir2NameLabel.setBounds(470, 370, 280, 18);
}

HecateAudioProcessorEditor::HecateAudioProcessorEditor(HecateAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), content(p)
{
    setLookAndFeel(&lookAndFeel);
    setWantsKeyboardFocus(true);
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
    content.updateHeaderState();

    if (content.tunerButton.getToggleState())
    {
        content.updateTuner();
        content.repaint(kTunerArea);
    }
    else if (content.eqDragCount > 0)
    {
        content.repaint(kEqCurveArea);
    }

    // Trim markers on the response plot track their knobs live
    if (content.currentPage == 2)
        content.repaint(kIRCurveArea);
}

bool HecateAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    auto& undoManager = processor.getUndoManager();

    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        return undoManager.undo();
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier
                                       | juce::ModifierKeys::shiftModifier, 0))
        return undoManager.redo();

    return false;
}
