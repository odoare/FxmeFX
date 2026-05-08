cmake_minimum_required(VERSION 3.22)

if(NOT COMMAND juce_add_plugin)
    project(FxmeCabinet VERSION 0.1.0)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
    endif()

    add_subdirectory(../../../JUCE JUCE)
    juce_add_module(../../../JUCE/usermodules/FxmeJuceTools)
endif()

juce_add_plugin(FxmeCabinet
    COMPANY_NAME                "FX-Mechanics"
    COMPANY_WEBSITE             "www.fx-mechanics.com"
    PLUGIN_MANUFACTURER_CODE    FXME
    PLUGIN_CODE                 CABN
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "FxmeCabinet"
    DESCRIPTION                 "FX-Mechanics Cabinet IR loader"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Distortion
    BUNDLE_ID                   "com.FXMechanics.FxmeCabinet"
)

# Embed every cabinet impulse response as binary data so the plugin ships
# self-contained. The processor scans BinaryData at startup and lists every
# .wav resource it finds, so adding/removing a file here automatically
# updates the in-plugin selector.
juce_add_binary_data(FxmeCabinetBinaryData SOURCES
    "IR/2off-pres5.wav"
    "IR/Allure_67_Brit_Greenback.wav"
    "IR/Allure_90s_Cali_V30.wav"
    "IR/AnnihilatorFeast.wav"
    "IR/Brohymn Mesa 4x12 SM57 V30 1.wav"
    "IR/Brohymn-Mesa-4x12-SM57-V30-5.wav"
    "IR/Cenzo Celestion V30 Mix.wav"
    "IR/Excalibur 1 - Bright1, a.wav"
    "IR/Excalibur 2 - Dark3, b.wav"
    "IR/GuitarHack JJ CENTRE45 0.wav"
    "IR/GuitarHack JJ FRED45 0.wav"
    "IR/KornKorn.wav"
    "IR/Marshall1960A-G12Ms-M160-CapEdge-3in.wav"
    "IR/Marshall1960A-G12Ms-SM57-Cap-0in.wav"
    "IR/NewmannU87 0_dc.wav"
    "IR/OH 412 MES-ST V60 421-04.wav"
    "IR/OH 412 MES-ST V60 57-00.wav"
    "IR/ShureSM57_0_dc.wav"
    "IR/s-preshigh.wav"
)

# Embedded WAV byte arrays don't benefit from optimisation/LTO and can blow
# up RAM during compile — disable both for the binary-data target only.
set_target_properties(FxmeCabinetBinaryData PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF)
target_compile_options(FxmeCabinetBinaryData PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O0 -fno-lto>
    $<$<CXX_COMPILER_ID:MSVC>:/Od>)

target_sources(FxmeCabinet PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp
    Cabinet.cpp
    CabinetComponent.cpp
    # WDL convolution + FFT + resampling
    ../../WDL/WDL/convoengine.cpp
    ../../WDL/WDL/fft.c
    ../../WDL/WDL/resample.cpp
)

target_include_directories(FxmeCabinet PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

juce_generate_juce_header(FxmeCabinet)

target_compile_definitions(FxmeCabinet
    PUBLIC
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FxmeCabinet
    PRIVATE
        FxmeCabinetBinaryData
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
