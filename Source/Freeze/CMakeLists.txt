cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/FxmeVersion.cmake)
    project(FxmeFreeze VERSION ${FXMEFX_VERSION})

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/cmake/FxmeTools.cmake)
endif()

juce_add_plugin(FxmeFreeze
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 FRZE
    FORMATS                     VST3 AU
    PRODUCT_NAME                "FxmeFreeze"
    DESCRIPTION                 "FX-Mechanics Spectral Freeze"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx
    BUNDLE_ID                   "com.FXMechanics.FxmeFreeze"
)

target_sources(FxmeFreeze PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Freeze.cpp
    FreezeComponent.cpp
    # WDL's fft.c (needed by <FxmeTools/dsp/SpectralFreeze.h>) is compiled in
    # by fxmetools_attach() below.
)

target_include_directories(FxmeFreeze PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeFreeze)

target_compile_definitions(FxmeFreeze
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CommonAssets.cmake)

target_link_libraries(FxmeFreeze
    PRIVATE
        FxmeCommonBinaryData
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

# Links the FxmeTools module and compiles the WDL FFT/convolution sources
# (SpectralFreeze.h needs WDL/fft.c in the target).
fxmetools_attach(FxmeFreeze)

# ── Pure Data external ────────────────────────────────────────────────────────
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeFreeze_pd
    LIBNAME   fxmefreeze~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        Freeze.cpp
        ../../lib/FxmeTools/WDL/WDL/fft.c
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/WDL/WDL
    DEFINES
        JucePlugin_Name="FxmeFreeze"
)
