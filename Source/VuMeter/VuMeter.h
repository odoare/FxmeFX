/*
  ==============================================================================

    VuMeter.h

    The VuMeter DSP class now lives in FxmeTools (fxme::VuMeter). This header is
    a thin re-export kept so existing includes ("VuMeter.h") and the global
    `VuMeter` type name remain valid — FxmeFX's public API is unchanged.

  ==============================================================================
*/

#pragma once

#include <FxmeTools/dsp/VuMeter.h>

using VuMeter = fxme::VuMeter;
