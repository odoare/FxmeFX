/*
  ==============================================================================

    PdMain.cpp — Pure Data external entry point for FxmeCab.
    See Source/PdCommon/PdJuceHost.h for the body of FXME_PD_DEFINE_EXTERNAL.

    All cabinet impulse responses ship as juce_add_binary_data resources;
    there is no file-loading path in the Pd build. The IR L / IR R inlets
    pick a built-in by 1-based index, identical to the VST parameter.

  ==============================================================================
*/

#include "../PdCommon/PdJuceHost.h"
#include "PluginProcessor.h"

#include <cstring>

FXME_PD_DEFINE_EXTERNAL (fxmecab, FxmeCabAudioProcessor)
