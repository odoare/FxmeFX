cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    project(FxmeTube VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    juce_add_module(../../../JUCE/usermodules/FxmeJuceTools)
endif()

# Tube faceplate images embedded as binary data (the component switches between
# the colour and bypass artwork via BinaryData::getNamedResource).
juce_add_binary_data(FxmeTubeBinaryData SOURCES
    img/tube.png
    img/tube_bw.png
)

set_target_properties(FxmeTubeBinaryData PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF)
target_compile_options(FxmeTubeBinaryData PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0 -fno-lto>
    $<$<CXX_COMPILER_ID:MSVC>:/Od>)

juce_add_plugin(FxmeTube
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 TUBE
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeTube"
    DESCRIPTION                 "FX-Mechanics Tube saturation"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Distortion
    BUNDLE_ID                   "com.FXMechanics.FxmeTube"
)

target_sources(FxmeTube PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Tube.cpp
    TubeComponent.cpp
)

target_include_directories(FxmeTube PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeTube)

target_compile_definitions(FxmeTube
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeTube
    PRIVATE
        FxmeTubeBinaryData
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
