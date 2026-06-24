cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    project(FxmeEqualizer VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/cmake/FxmeTools.cmake)
endif()

juce_add_plugin(FxmeEqualizer
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 EQUL
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeEqualizer"
    DESCRIPTION                 "FX-Mechanics 5-band Equalizer"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx EQ
    BUNDLE_ID                   "com.FXMechanics.FxmeEqualizer"
)

target_sources(FxmeEqualizer PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Equalizer.cpp
    EqualizerComponent.cpp
)

target_include_directories(FxmeEqualizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeEqualizer)

target_compile_definitions(FxmeEqualizer
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeEqualizer
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
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeEqualizer_pd
    LIBNAME   fxmeequalizer~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        Equalizer.cpp
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        # Equalizer.cpp/.h use header-only <FxmeTools/dsp/Biquad.h>.
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools
    DEFINES
        JucePlugin_Name="FxmeEqualizer"
)
