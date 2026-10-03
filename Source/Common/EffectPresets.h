/*
  ==============================================================================

    EffectPresets.h

    An FxmeFX effect's presets: FxmeTools module presets (see FxmeTools'
    doc/local-presets-plan.md), so the same presets serve the effect's own
    plugin and every plugin that embeds the effect (FxmeSampler, MechanOdd).

      - Module name: the effect's class name ("Compressor", "StereoDelay").
      - Scope: every parameter whose ID starts with "<prefix>_<Tag>_" (the
        effect's own parameters, whatever prefix the host gives it).
      - User presets: FX-Mechanics/Modules/<Effect>/Presets, shared by all of
        those plugins. For a single-effect plugin these are its only
        presets: no folder of its own (a deliberate exception to the
        FX-Mechanics/<Plugin>/Presets rule, confirmed 2026-10-03).
      - Factory presets: Source/<Effect>/Presets/*.xml, embedded through
        fxmefx_add_module_presets() (cmake/FxmeModulePresets.cmake), which
        sets FXMEFX_HAS_MODULE_PRESETS.

    In a processor, after its APVTS:

        fxmefx::EffectPresets presets { apvts, "Compressor", "Main_Comp_" };

    Not available in the Pure Data externals (FXME_PD_BUILD): they have no
    FxmeTools module, and no GUI to show presets in.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#ifndef FXMEFX_HAS_MODULE_PRESETS
 #define FXMEFX_HAS_MODULE_PRESETS 0
#endif

#if FXMEFX_HAS_MODULE_PRESETS
 #include <FxmeModulePresets.h>
#endif

namespace fxmefx
{

struct EffectPresets
{
    /** Format version of every FxmeFX effect's presets, for now. */
    static constexpr int formatVersion = 1;

    EffectPresets (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& moduleName,
                   const juce::String& idPrefix)
        : library (moduleName, formatVersion,
                   fxme::PresetManager::getModulePresetDirectory (moduleName)
                  #if FXMEFX_HAS_MODULE_PRESETS
                   , FxmeModulePresets::namedResourceList,
                   FxmeModulePresets::namedResourceListSize,
                   FxmeModulePresets::getNamedResource
                  #endif
                   ),
          target (library, apvts, idPrefix)
    {
    }

    fxme::ModulePresetLibrary library;
    fxme::ModulePresetTarget target;

    JUCE_DECLARE_NON_COPYABLE (EffectPresets)
};

} // namespace fxmefx
