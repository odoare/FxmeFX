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
    0b46947472d10bb044c058d9ddb8b6d38f66050a)
# CLAP format (fxmetools_add_clap). Its last tag predates JUCE 9 support:
# pinned to a commit. Brings the clap and clap-helpers submodules along.
fxme_dependency(ClapJuceExtensions https://github.com/free-audio/clap-juce-extensions
    7adee3a1bd4684d4caa5601100e364abccff4b4d
    SIBLING clap-juce-extensions)

if(NOT COMMAND juce_add_plugin)
    add_subdirectory(${JUCE_DIR} JUCE)
endif()

# Registers the `FxmeTools` JUCE module, FxmeCore, fxmetools_attach() and
# fxmetools_add_clap(), and sets FXMETOOLS_ROOT / FXMETOOLS_WDL_DIR used by
# the plugins' Pd targets.
include(${FXMETOOLS_DIR}/cmake/FxmeTools.cmake)
