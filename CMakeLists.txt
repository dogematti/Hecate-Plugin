cmake_minimum_required(VERSION 3.22)
project(Hecate VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(JUCE_BUILD_EXTRAS OFF)
set(JUCE_BUILD_EXAMPLES OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
)
FetchContent_MakeAvailable(JUCE)

# Header-only pitch shifter for the drop-tune feature (MIT); its CMake also
# fetches its signalsmith-linear dependency
FetchContent_Declare(
    SignalsmithStretch
    GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
    GIT_TAG main
)
FetchContent_MakeAvailable(SignalsmithStretch)

if(APPLE)
    set(PLUGIN_FORMATS AU VST3 Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Hecate
    PRODUCT_NAME "Hecate"
    COMPANY_NAME "Hecate Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Hcte
    PLUGIN_CODE Hct1
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.iconset/icon_512x512@2x.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/AppIcon.iconset/icon_128x128.png"
    FORMATS ${PLUGIN_FORMATS}
    VST3_CATEGORIES "Fx" "Distortion"
)

juce_add_binary_data(HecateAssets SOURCES
    Assets/background.png
    Assets/default-cab.wav)

target_sources(Hecate
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/DSP/DropTuner.cpp
        Source/DSP/Octaver.cpp
        Source/DSP/Saturator.cpp
        Source/DSP/PowerAmp.cpp
        Source/DSP/GateProcessor.cpp
        Source/DSP/Doubler.cpp
        Source/DSP/StereoDelay.cpp
        Source/DSP/Equalizer.cpp
        Source/DSP/Compressor.cpp
        Source/DSP/StereoReverb.cpp
        Source/DSP/CabinetSimulator.cpp
)

target_include_directories(Hecate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${signalsmithstretch_SOURCE_DIR})

target_compile_definitions(Hecate PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    HECATE_VERSION="${PROJECT_VERSION}")

target_link_libraries(Hecate
    PRIVATE
        HecateAssets
        signalsmith-stretch
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

# Offline render harness: bounces a deterministic DI (or a supplied WAV)
# through every factory preset, checks for NaN/silence/clipping, writes WAVs.
# Runs in CI as a DSP regression test.
juce_add_console_app(HecateRender PRODUCT_NAME "HecateRender")

target_sources(HecateRender PRIVATE
    Tools/RenderPresets.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/Presets.cpp
    Source/DSP/DropTuner.cpp
    Source/DSP/Octaver.cpp
    Source/DSP/Saturator.cpp
    Source/DSP/PowerAmp.cpp
    Source/DSP/GateProcessor.cpp
    Source/DSP/Doubler.cpp
    Source/DSP/StereoDelay.cpp
    Source/DSP/Equalizer.cpp
    Source/DSP/Compressor.cpp
    Source/DSP/CabinetSimulator.cpp
    Source/DSP/StereoReverb.cpp
)

target_include_directories(HecateRender PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${signalsmithstretch_SOURCE_DIR})

target_compile_definitions(HecateRender PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    HECATE_VERSION="${PROJECT_VERSION}")

target_link_libraries(HecateRender
    PRIVATE
        HecateAssets
        signalsmith-stretch
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_core
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
)
