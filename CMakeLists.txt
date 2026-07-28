cmake_minimum_required(VERSION 3.22)
project(Hecate VERSION 0.2.0 LANGUAGES C CXX)

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

juce_add_binary_data(HecateAssets SOURCES Assets/background.png)

target_sources(Hecate
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
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

target_include_directories(Hecate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(Hecate PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    HECATE_VERSION="${PROJECT_VERSION}")

target_link_libraries(Hecate
    PRIVATE
        HecateAssets
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)
