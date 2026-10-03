# ─────────────────────────────────────────────────────────────────────────────
# FxmeModulePresets.cmake
#
# fxmefx_add_module_presets(<target> <Effect>...)
#
# Embeds the factory presets of the listed FxmeFX effects into <target>:
# every Source/<Effect>/Presets/*.xml (FxmeModulePreset files, see
# Source/Common/EffectPresets.h), in one binary-data target with the
# FxmeModulePresets namespace and header. Defines FXMEFX_HAS_MODULE_PRESETS
# to 1 on <target> when there is anything to embed, 0 otherwise, so
# EffectPresets.h knows whether FxmeModulePresets.h exists.
#
# A single-effect plugin lists its effect; a plugin embedding several
# effects (FxmeSampler, MechanOdd) lists them all in one call: each
# effect's preset library keeps only the files of its own module.
#
# Binary-data symbols come from file names only, so name preset files
# <Effect>_<Preset name>.xml: two effects may then have presets of the
# same name side by side in one host.
#
# A host outside this repository sets FXMEFX_ROOT to its FxmeFX checkout
# before including this file.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

if(NOT DEFINED FXMEFX_ROOT)
    set(FXMEFX_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

function(fxmefx_add_module_presets target)
    set(_files)
    foreach(_effect IN LISTS ARGN)
        file(GLOB _effect_files CONFIGURE_DEPENDS ${FXMEFX_ROOT}/Source/${_effect}/Presets/*.xml)
        list(APPEND _files ${_effect_files})
    endforeach()

    if(_files)
        juce_add_binary_data(${target}_ModulePresets
            NAMESPACE   FxmeModulePresets
            HEADER_NAME FxmeModulePresets.h
            SOURCES     ${_files})
        set_target_properties(${target}_ModulePresets PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION OFF)
        target_link_libraries(${target} PRIVATE ${target}_ModulePresets)
        target_compile_definitions(${target} PRIVATE FXMEFX_HAS_MODULE_PRESETS=1)
    else()
        target_compile_definitions(${target} PRIVATE FXMEFX_HAS_MODULE_PRESETS=0)
    endif()
endfunction()
