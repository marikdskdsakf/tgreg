cmake_minimum_required(VERSION 3.22)
project(Zankyo VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Zankyo
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE Ynam
    PLUGIN_CODE Zank
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    PRODUCT_NAME "Zankyo Reverb"
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Zankyo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Zankyo PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Zankyo
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lib_flags)
