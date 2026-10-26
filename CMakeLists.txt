cmake_minimum_required(VERSION 3.22)
project(BlueToothRot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BlueToothRot
    COMPANY_NAME "Antigravity Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Anti
    PLUGIN_CODE BtRt
    FORMATS VST3 Standalone
    PRODUCT_NAME "BlueToothRot"
    DESCRIPTION "Real-World Bluetooth Connection Degradation & Headphone Acoustic Rot"
)

target_sources(BlueToothRot PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP/Parameters.h
    Source/DSP/RFSimulationEngine.h
    Source/DSP/JitterBuffer.h
    Source/DSP/PacketLossConcealment.h
    Source/DSP/SBCFilterBank.h
    Source/DSP/HandsFreeProfile.h
    Source/DSP/StereoDesyncEngine.h
    Source/DSP/HeadphoneAcoustics.h
)

target_compile_definitions(BlueToothRot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BlueToothRot PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
)
