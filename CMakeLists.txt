cmake_minimum_required(VERSION 3.22)

project(FxmeFX VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

# JUCE and the FxmeJuceTools user-module are expected as siblings of this repo:
#   ../JUCE/                              (juce-framework/JUCE checkout)
#   ../JUCE/usermodules/FxmeJuceTools/    (odoare/FxmeJuceTools, module/ moved in)
add_subdirectory(../JUCE JUCE)
juce_add_module(../JUCE/usermodules/FxmeJuceTools)

# ── Plugin selection ──────────────────────────────────────────────────────────
# Build every plugin by default. To build only one:
#   cmake -B build -DPLUGIN=Compressor
set(PLUGIN "" CACHE STRING "Build only this plugin (leave empty to build all)")

set(_all_plugins
    Compressor
    Equalizer
    Tube
    Transient
    StereoDelay
    ConvolReverb
)

if(PLUGIN STREQUAL "")
    foreach(_p IN LISTS _all_plugins)
        add_subdirectory(Source/${_p})
    endforeach()
else()
    if(NOT "${PLUGIN}" IN_LIST _all_plugins)
        message(FATAL_ERROR "Unknown PLUGIN '${PLUGIN}'. Valid values: ${_all_plugins}")
    endif()
    add_subdirectory(Source/${PLUGIN})
endif()
