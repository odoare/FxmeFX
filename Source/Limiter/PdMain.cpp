/*
  ==============================================================================

    PdMain.cpp — Pure Data external entry point for FxmeLimiter.

    Pd loads "fxmelimiter~.pd_linux" (or .pd_darwin / .dll) and calls
    fxmelimiter_tilde_setup() at load time. FXME_PD_DEFINE_EXTERNAL
    expands to the full set of Pd object methods (_new, _free, _dsp,
    _perform) and the setup function — see PdJuceHost.h for the body.

  ==============================================================================
*/

#include "../PdCommon/PdJuceHost.h"
#include "PluginProcessor.h"

#include <cstring>

FXME_PD_DEFINE_EXTERNAL (fxmelimiter, FxmeLimiterAudioProcessor)
