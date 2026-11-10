cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/FxmeVersion.cmake)

    # Both of these must be set before project(): the compiler probes run
    # there, so a later CMAKE_OSX_ARCHITECTURES gives a single-slice build.
    # 10.13 here as for the others; the neural backend raises this plugin to
    # 10.15 (std::filesystem; see cmake/FxmeNeural.cmake).
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    project(FxmeNeuralAmp VERSION ${FXMEFX_VERSION})

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/Dependencies.cmake)
endif()

juce_add_plugin(FxmeNeuralAmp
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 NAMP
    FORMATS                     VST3 AU
    PRODUCT_NAME                "FxmeNeuralAmp"
    DESCRIPTION                 "FX-Mechanics Neural Amp: plays Neural Amp Modeler (NAM) models"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Distortion
    BUNDLE_ID                   "com.FXMechanics.FxmeNeuralAmp"
)

target_sources(FxmeNeuralAmp PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    NeuralAmp.cpp
    NeuralAmpComponent.cpp
)

target_include_directories(FxmeNeuralAmp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeNeuralAmp)

target_compile_definitions(FxmeNeuralAmp
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../Common/CommonAssets.cmake)

# Factory presets of the effect (Presets/*.xml): settings only, no model
# (models belong to their authors; a preset without one keeps the model
# playing). See Common/EffectPresets.h.
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FxmeModulePresets.cmake)
fxmefx_add_module_presets(FxmeNeuralAmp NeuralAmp)

# The neural engine: FxmeTools' FxmeNeural and NeuralAudio (pinned in
# cmake/Dependencies.cmake), built for x86-64-v3.
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FxmeNeural.cmake)
fxmefx_attach_neural(FxmeNeuralAmp)

target_link_libraries(FxmeNeuralAmp
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

# CLAP, beside VST3 and AU (clap-juce-extensions; see fxmetools_add_clap
# in FxmeTools.cmake). The ID is permanent once released.
fxmetools_add_clap(FxmeNeuralAmp
    ID       "com.fx-mechanics.FxmeNeuralAmp"
    FEATURES audio-effect distortion stereo)

# No Pure Data external: loading a model needs a message the Pd host does
# not have.
