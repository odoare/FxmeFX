/*
  ==============================================================================

    VuMeterComponent.h

    The VuMeterComponent now lives in FxmeTools (fxme::VuMeterComponent). This
    header is a thin re-export kept so existing includes ("VuMeterComponent.h")
    and the global `VuMeterComponent` type name remain valid — FxmeFX's public
    API is unchanged.

  ==============================================================================
*/

#pragma once

#include <FxmeTools/components/VuMeterComponent.h>

using VuMeterComponent = fxme::VuMeterComponent;
