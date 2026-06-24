cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    project(FxmeConvolReverb VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/cmake/FxmeTools.cmake)
endif()

juce_add_plugin(FxmeConvolReverb
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 CREV
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeConvolReverb"
    DESCRIPTION                 "FX-Mechanics Convolution Reverb"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Reverb
    BUNDLE_ID                   "com.FXMechanics.FxmeConvolReverb"
)

# Embed the built-in impulse responses as binary data so they ship with the
# plugin and can be loaded without any external files.
juce_add_binary_data(FxmeConvolReverbBinaryData SOURCES
    ir/Council\ Chamber.wav
    ir/Forest\ short.wav
    ir/Forest\ long.wav
    ir/Rectangular\ room\ small.wav
    ir/Rectangular\ room\ medium.wav
    ir/Rectangular\ room\ large.wav
)

# Embedded WAV byte arrays don't benefit from optimisation/LTO and can blow up
# RAM during compile — disable them for the data target only.
set_target_properties(FxmeConvolReverbBinaryData PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF)
target_compile_options(FxmeConvolReverbBinaryData PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0 -fno-lto>
    $<$<CXX_COMPILER_ID:MSVC>:/Od>)

target_sources(FxmeConvolReverb PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    ConvolReverb.cpp
    ConvolReverbComponent.cpp
    # WDL convolution + FFT + resampling are compiled in by fxmetools_attach()
    # below (sourced from FxmeTools' WDL submodule).
)

target_include_directories(FxmeConvolReverb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeConvolReverb)

target_compile_definitions(FxmeConvolReverb
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeConvolReverb
    PRIVATE
        FxmeConvolReverbBinaryData
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

# Links the FxmeTools module and compiles the WDL convolution engine
# (convoengine.cpp/fft.c/resample.cpp) used via <convoengine.h>/<resample.h>.
fxmetools_attach(FxmeConvolReverb)

# ── Pure Data external ────────────────────────────────────────────────────────
# Reuses FxmeConvolReverbBinaryData. The Pd build hides the "External IR
# file" slot — see the FXME_PD_BUILD branch in ConvolReverb::addParameters.
include(${CMAKE_CURRENT_LIST_DIR}/../PdCommon/PdExternal.cmake)
fxme_add_pd_external(FxmeConvolReverb_pd
    LIBNAME   fxmeconvolreverb~
    SOURCES
        PdMain.cpp
        PluginProcessor.cpp
        ConvolReverb.cpp
        ../../lib/FxmeTools/WDL/WDL/convoengine.cpp
        ../../lib/FxmeTools/WDL/WDL/fft.c
        ../../lib/FxmeTools/WDL/WDL/resample.cpp
    INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../lib/FxmeTools/WDL/WDL
    DEFINES
        JucePlugin_Name="FxmeConvolReverb"
    LINK_LIBRARIES
        FxmeConvolReverbBinaryData
)
