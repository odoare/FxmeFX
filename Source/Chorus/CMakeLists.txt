cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/FxmeVersion.cmake)

    # Both of these must be set before project(): the compiler probes run
    # there, so a later CMAKE_OSX_ARCHITECTURES gives a single-slice build.
    # 10.13 rather than 11.0 — 11.0 silently excludes Catalina.
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    project(FxmeChorus VERSION ${FXMEFX_VERSION})

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    add_subdirectory(../../../JUCE JUCE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/cmake/FxmeTools.cmake)
endif()

juce_add_plugin(FxmeChorus
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 CHOR
    FORMATS                     VST3 AU
    PRODUCT_NAME                "FxmeChorus"
    DESCRIPTION                 "FX-Mechanics Stereo Chorus"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Modulation
    BUNDLE_ID                   "com.FXMechanics.FxmeChorus"
)

target_sources(FxmeChorus PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Chorus.cpp
    ChorusComponent.cpp
)

target_include_directories(FxmeChorus PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeChorus)

target_compile_definitions(FxmeChorus
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CommonAssets.cmake)

target_link_libraries(FxmeChorus
    PRIVATE
        FxmeCommonBinaryData
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
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeChorus_pd
    LIBNAME   fxmechorus~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        Chorus.cpp
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools
    DEFINES
        JucePlugin_Name="FxmeChorus"
)
