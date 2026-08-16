cmake_minimum_required(VERSION 3.22)

# Series version — single source of truth is Source/Common/Version.h.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FxmeVersion.cmake)

# Both of these must be set before project(): the compiler probes run there,
# so a CMAKE_OSX_ARCHITECTURES set afterwards gives a single-slice build that
# still calls itself universal. 10.13 rather than 11.0 — 11.0 silently
# excludes Catalina.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Target architectures")
endif()

project(FxmeFX VERSION ${FXMEFX_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is expected as a sibling of this repo:
#   ../JUCE/   (juce-framework/JUCE checkout)
add_subdirectory(../JUCE JUCE)

# FxmeTools (shared GUI controls, look-and-feel, DSP, and the WDL convolution
# engine) ships as a submodule under lib/FxmeTools. Including its .cmake here
# registers the `FxmeTools` JUCE module and the fxmetools_attach() helper.
include(${CMAKE_CURRENT_SOURCE_DIR}/lib/FxmeTools/cmake/FxmeTools.cmake)

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
    Cab
    Oct
    Limiter
    Freeze
    Chorus
    Flanger
    Phaser
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
