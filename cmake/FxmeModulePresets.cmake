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
# The effects' folders are found from this file's own location (the FxmeFX
# checkout it belongs to); a host may point elsewhere by setting FXMEFX_ROOT
# before calling the function.
#
# The root is resolved inside the function, at each call, on purpose: a
# variable set here at include time would only exist in the directory that
# included the file first (include_guard stops the others from running it),
# and every other plugin would then glob an empty path and embed nothing.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

function(fxmefx_add_module_presets target)
    if(DEFINED FXMEFX_ROOT)
        set(_root ${FXMEFX_ROOT})
    else()
        get_filename_component(_root ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.. ABSOLUTE)
    endif()

    set(_files)
    foreach(_effect IN LISTS ARGN)
        file(GLOB _effect_files CONFIGURE_DEPENDS ${_root}/Source/${_effect}/Presets/*.xml)
        if(NOT _effect_files)
            message(STATUS "${target}: no factory presets in ${_root}/Source/${_effect}/Presets")
        endif()
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
