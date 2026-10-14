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

fxme_dependency(JUCE https://github.com/juce-framework/JUCE 8.0.12 SHALLOW)
fxme_dependency(FxmeTools https://github.com/odoare/FxmeTools
    b4bb53e78c34261951ca2400932a4dd19f88d327)

if(NOT COMMAND juce_add_plugin)
    add_subdirectory(${JUCE_DIR} JUCE)
endif()

# Registers the `FxmeTools` JUCE module, FxmeCore and fxmetools_attach(), and
# sets FXMETOOLS_ROOT / FXMETOOLS_WDL_DIR used by the plugins' Pd targets.
include(${FXMETOOLS_DIR}/cmake/FxmeTools.cmake)
