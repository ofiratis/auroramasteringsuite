cmake_minimum_required(VERSION 3.22)
project(AuroraMastering VERSION 1.0.0 LANGUAGES C CXX)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(AuroraMastering
    COMPANY_NAME "OpenAudio"
    BUNDLE_ID "com.openaudio.auroramastering"
    PLUGIN_MANUFACTURER_CODE OAud
    PLUGIN_CODE AuMs
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Aurora Mastering"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(AuroraMastering PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
juce_generate_juce_header(AuroraMastering)
target_compile_features(AuroraMastering PRIVATE cxx_std_17)
target_compile_definitions(AuroraMastering PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(AuroraMastering PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_audio_processors juce::juce_gui_extra juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
