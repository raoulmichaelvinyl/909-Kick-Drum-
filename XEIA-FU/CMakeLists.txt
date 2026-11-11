cmake_minimum_required(VERSION 3.22)
project(XEIA_FU VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use a local JUCE checkout if you have one (-DJUCE_DIR=/path/to/JUCE),
# otherwise download JUCE automatically.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(JUCE)
endif()

set(XFU_FORMATS VST3 Standalone)
set(XFU_COPY_AFTER_BUILD FALSE)
if(APPLE)
    list(APPEND XFU_FORMATS AU)
    set(XFU_COPY_AFTER_BUILD TRUE) # installs into ~/Library/Audio/Plug-Ins
endif()

juce_add_plugin(XeiaFu
    COMPANY_NAME "Xeia"
    PLUGIN_MANUFACTURER_CODE Xeia
    PLUGIN_CODE Xfu9
    FORMATS ${XFU_FORMATS}
    PRODUCT_NAME "XEIA-FU"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${XFU_COPY_AFTER_BUILD}
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)

target_sources(XeiaFu PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(XeiaFu PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(XeiaFu
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
