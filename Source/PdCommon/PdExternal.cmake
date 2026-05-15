# ─────────────────────────────────────────────────────────────────────────────
# fxme_add_pd_external(target
#     LIBNAME  <pd-library-base-name>     # e.g. "fxmecompressor~"  (no suffix)
#     SOURCES  <list>                     # cpp files (PdMain.cpp + shared DSP)
#     INCLUDES <list>                     # extra include directories
#     DEFINES  <list>                     # extra compile definitions
# )
#
# Builds a Pd external as a MODULE library. Links the JUCE modules needed for
# headless AudioProcessor hosting, but deliberately omits juce_audio_plugin_client
# (would inject a plugin entry point) and the GUI-only utility modules.
# ─────────────────────────────────────────────────────────────────────────────

include_guard(GLOBAL)

function(fxme_add_pd_external target)
    cmake_parse_arguments(ARG "" "LIBNAME" "SOURCES;INCLUDES;DEFINES;LINK_LIBRARIES" ${ARGN})

    if(NOT ARG_LIBNAME)
        message(FATAL_ERROR "fxme_add_pd_external: LIBNAME is required")
    endif()

    add_library(${target} MODULE ${ARG_SOURCES})

    # Pd loader expects e.g. "fxmecompressor~.pd_linux", with no "lib" prefix.
    set_target_properties(${target} PROPERTIES
        PREFIX      ""
        OUTPUT_NAME "${ARG_LIBNAME}"
    )

    if(APPLE)
        set_target_properties(${target} PROPERTIES SUFFIX ".pd_darwin")
    elseif(UNIX)
        set_target_properties(${target} PROPERTIES SUFFIX ".pd_linux")
    elseif(WIN32)
        set_target_properties(${target} PROPERTIES SUFFIX ".dll")
    endif()

    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}                      # for "m_pd.h" shipped in PdCommon/
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/JuceHeaderShim       # for <JuceHeader.h>
        ${ARG_INCLUDES}
    )

    # Mirrors the JucePlugin_* macros that juce_add_plugin would normally
    # synthesise. Most PluginProcessor.cpp files only need a handful.
    target_compile_definitions(${target} PRIVATE
        FXME_PD_BUILD=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_IsSynth=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_ManufacturerCode=0x464d4520     # 'FXME'
        JucePlugin_PluginCode=0x434f4d50           # 'COMP' — overridden via DEFINES
        ${ARG_DEFINES}
    )

    # APVTS (juce::AudioProcessorValueTreeState) lives in the full
    # juce_audio_processors module, which transitively depends on
    # juce_gui_extra/juce_gui_basics/juce_graphics — so the resulting Pd
    # external links against the JUCE GUI stack (and pulls X11/freetype on
    # Linux at load time). That is the documented cost of headless-hosting a
    # JUCE plugin: nothing is instantiated GUI-wise (hasEditor() == false in
    # PD builds), the modules are just present for link.
    # We omit juce_audio_plugin_client (we are not a plugin) and
    # FxmeJuceTools (GUI-only helpers, not referenced by the DSP path).
    target_link_libraries(${target} PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    )

    target_link_libraries(${target} PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    if(ARG_LINK_LIBRARIES)
        target_link_libraries(${target} PRIVATE ${ARG_LINK_LIBRARIES})
    endif()

    # Pd externals are loaded by Pd via dlopen; the m_pd.h ABI symbols
    # (pd_new, outlet_new, dsp_add, ...) are resolved at load time against
    # the host process, so they must remain undefined in our shared module.
    if(UNIX AND NOT APPLE)
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        # GNU ld allows undefined symbols in shared modules by default; we
        # only need to make sure nothing higher up flips it to --no-undefined.
    elseif(APPLE)
        target_link_options(${target} PRIVATE "-undefined" "dynamic_lookup")
    endif()

endfunction()
