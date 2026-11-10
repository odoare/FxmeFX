# ──────────────────────────────────────────────────────────────────────────
# Dependencies.cmake — FxmeFX's pinned dependencies.
#
# Included after project() by the root CMakeLists.txt and by each plugin's
# CMakeLists.txt when that plugin is configured on its own, so the pins live
# here only. Resolution (sibling clone or pinned commit) is FxmeDeps.cmake's.
#
# Projects that use FxmeFX as a source library (Camshaft, ...) do not include
# this file: they pin JUCE and FxmeTools themselves.
# ──────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/FxmeDeps.cmake)

fxme_dependency(JUCE https://github.com/juce-framework/JUCE 9.0.4 SHALLOW)
fxme_dependency(FxmeTools https://github.com/odoare/FxmeTools
    94e4e4d9147bf9ba51db0c88814eea60104d13a2)
# CLAP format (fxmetools_add_clap). Its last tag predates JUCE 9 support:
# pinned to a commit. Brings the clap and clap-helpers submodules along.
fxme_dependency(ClapJuceExtensions https://github.com/free-audio/clap-juce-extensions
    7adee3a1bd4684d4caa5601100e364abccff4b4d
    SIBLING clap-juce-extensions)
# Neural amp models (FxmeNeuralAmp, fxmefx_attach_neural): our fork of
# mikeoliphant/NeuralAudio until its fixes are merged upstream (branch
# fxme-no-audio-alloc: no allocation on the audio thread). Brings NAM Core,
# RTNeural and math_approx along as submodules.
fxme_dependency(NeuralAudio https://github.com/odoare/NeuralAudio
    2aeff04543bb678546c5128b3e4aa0dc54278e57)

if(NOT COMMAND juce_add_plugin)
    add_subdirectory(${JUCE_DIR} JUCE)
endif()

# Registers the `FxmeTools` JUCE module, FxmeCore, fxmetools_attach() and
# fxmetools_add_clap(), and sets FXMETOOLS_ROOT / FXMETOOLS_WDL_DIR used by
# the plugins' Pd targets.
include(${FXMETOOLS_DIR}/cmake/FxmeTools.cmake)
