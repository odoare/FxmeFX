/*
  ==============================================================================

    PdMain.cpp — Pure Data external entry point for FxmeEqualizer.
    See Source/PdCommon/PdJuceHost.h for the body of FXME_PD_DEFINE_EXTERNAL.

  ==============================================================================
*/

#include "../PdCommon/PdJuceHost.h"
#include "PluginProcessor.h"

#include <cstring>

FXME_PD_DEFINE_EXTERNAL (fxmeequalizer, FxmeEqualizerAudioProcessor)
