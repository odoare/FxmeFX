/*
  ==============================================================================

    JuceHeader.h — shim for PD-external builds.

    juce_generate_juce_header() only runs on juce_add_* targets, so our
    plain add_library(MODULE) PD externals need a hand-written umbrella
    header. This pulls in the JUCE modules linked by PdExternal.cmake and
    nothing else — no GUI, no audio_devices, no plugin_client.

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
