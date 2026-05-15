cmake_minimum_required(VERSION 3.22)

# When building this plugin standalone, set up the project and pull in JUCE +
# FxmeJuceTools. When included via the root FxmeFX CMakeLists.txt, the parent
# has already done that — skip to keep targets unique.
if(NOT COMMAND juce_add_plugin)
    project(FxmeCompressor VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    juce_add_module(../../../JUCE/usermodules/FxmeJuceTools)
endif()

juce_add_plugin(FxmeCompressor
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 COMP
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeCompressor"
    DESCRIPTION                 "FX-Mechanics Compressor"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Dynamics
    BUNDLE_ID                   "com.FXMechanics.FxmeCompressor"
)

target_sources(FxmeCompressor PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Compressor.cpp
    CompressorComponent.cpp
    ../VuMeter/VuMeter.cpp
    ../VuMeter/VuMeterComponent.cpp
)

target_include_directories(FxmeCompressor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../VuMeter
)

# Generate a JuceHeader.h for the target (replaces the Projucer-generated JuceLibraryCode/)
juce_generate_juce_header(FxmeCompressor)

target_compile_definitions(FxmeCompressor
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeCompressor
    PRIVATE
        FxmeJuceTools
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Pure Data external ────────────────────────────────────────────────────────
# Shares the DSP source (PluginProcessor + Compressor + VuMeter) with the VST3
# target; the only divergence is PdMain.cpp (Pd entry points) and the
# FXME_PD_BUILD compile flag, which neuters the editor in PluginProcessor.
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeCompressor_pd
    LIBNAME   fxmecompressor~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        Compressor.cpp
        ../VuMeter/VuMeter.cpp
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../VuMeter
    DEFINES
        JucePlugin_Name="FxmeCompressor"
)
