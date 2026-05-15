/*
  ==============================================================================

    PdMain.cpp — Pure Data external entry point for FxmeConvolReverb.
    See Source/PdCommon/PdJuceHost.h for the body of FXME_PD_DEFINE_EXTERNAL.

    Built-in IRs ship as juce_add_binary_data resources. The Pd build hides
    the "External IR file" slot — see ConvolReverb::addParameters — so the
    IR inlet only addresses built-ins 1..N.

  ==============================================================================
*/

#include "../PdCommon/PdJuceHost.h"
#include "PluginProcessor.h"

#include <cstring>

FXME_PD_DEFINE_EXTERNAL (fxmeconvolreverb, FxmeConvolReverbAudioProcessor)
