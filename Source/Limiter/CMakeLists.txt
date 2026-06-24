cmake_minimum_required(VERSION 3.22)

# When building this plugin standalone, set up the project and pull in JUCE +
# FxmeTools. When included via the root FxmeFX CMakeLists.txt, the parent
# has already done that — skip to keep targets unique.
if(NOT COMMAND juce_add_plugin)
    project(FxmeLimiter VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/cmake/FxmeTools.cmake)
endif()

juce_add_plugin(FxmeLimiter
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 LIMT
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeLimiter"
    DESCRIPTION                 "FX-Mechanics look-ahead limiter / maximizer"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Dynamics
    BUNDLE_ID                   "com.FXMechanics.FxmeLimiter"
)

target_sources(FxmeLimiter PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Limiter.cpp
    LimiterComponent.cpp
)

target_include_directories(FxmeLimiter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    # VuMeter/VuMeterComponent now live in FxmeTools; ../VuMeter holds thin
    # re-export shims kept for source/API compatibility.
    ${CMAKE_CURRENT_SOURCE_DIR}/../VuMeter
)

# Generate a JuceHeader.h for the target (replaces the Projucer-generated JuceLibraryCode/)
juce_generate_juce_header(FxmeLimiter)

target_compile_definitions(FxmeLimiter
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeLimiter
    PRIVATE
        FxmeTools
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
# Shares the DSP source (PluginProcessor + Limiter + VuMeter) with the VST3
# target; the only divergence is PdMain.cpp (Pd entry points) and the
# FXME_PD_BUILD compile flag, which neuters the editor in PluginProcessor.
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeLimiter_pd
    LIBNAME   fxmelimiter~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        Limiter.cpp
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        # ../VuMeter holds the re-export shim; lib/FxmeTools makes the
        # header-only <FxmeTools/dsp/VuMeter.h> resolve in the Pd build.
        ${CMAKE_CURRENT_SOURCE_DIR}/../VuMeter
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools
    DEFINES
        JucePlugin_Name="FxmeLimiter"
)
