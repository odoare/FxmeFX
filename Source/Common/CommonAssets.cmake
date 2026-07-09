# ─────────────────────────────────────────────────────────────────────────────
# CommonAssets.cmake
#
# Registers the FxmeCommonBinaryData target (FX-Mechanics logo, shared across
# every plugin's title bar). Included from each plugin's CMakeLists.txt;
# include_guard(GLOBAL) makes it safe to include from more than one
# subdirectory — the target is only created once, and CMake targets are
# visible to every directory added afterwards.
#
# A dedicated NAMESPACE/HEADER_NAME keeps its generated BinaryData symbols
# from colliding with each plugin's own juce_add_binary_data target (IRs,
# tube images, ...), which all use the default "BinaryData" namespace.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

if(NOT TARGET FxmeCommonBinaryData)
    juce_add_binary_data(FxmeCommonBinaryData
        NAMESPACE   FxmeCommonBinaryData
        HEADER_NAME FxmeCommonBinaryData.h
        SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/Assets/logo.png
    )
    set_target_properties(FxmeCommonBinaryData PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION OFF)
endif()
