/*
  ==============================================================================

    Controls.h

    Set-up shared by the FxmeFX components, so that a control of one kind
    looks the same in every effect.

      setupFader      a console fader (FxmeLookAndFeel's "console" linear
                      style): a scale beside a slot, a ribbed cap, the value
                      under it;
      setupGainFader  the same for a ±dB gain (input, output): the fill grows
                      from 0 dB, one decimal, " dB".

    Both are vertical by default; a horizontal one (`vertical` false) has its
    scale under the slot and its value on the right.

    The look-and-feel is the component's own (FxmeFX components set theirs
    per widget). A fader on a panel's right edge wants kEdgeMargin of room on
    its right: the cap reaches the fader's right side.

    Author: Olivier Doaré, github.com/odoare
    Dual-licensed, mirroring the JUCE framework it depends on: under the GNU
    AGPL Version 3.0, or under commercial terms available from the author.
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace fxmefx
{
    /** Room to leave between a fader or a meter and the panel's right edge. */
    inline constexpr float kEdgeMargin = 10.0f;

    inline void setupFader (juce::Component& owner, fxme::FxmeSlider& slider, const juce::String& name,
                            juce::Colour accent, juce::LookAndFeel& laf, bool vertical = true)
    {
        owner.addAndMakeVisible (slider);
        slider.setSliderStyle (vertical ? juce::Slider::LinearVertical : juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.getProperties().set ("linearStyle", "console");
        slider.setTooltip (name);
        slider.setName (name);
        slider.setShowLabel (true);
        slider.setLookAndFeel (&laf);
        slider.setColour (juce::Slider::trackColourId,              accent.darker());
        slider.setColour (juce::Slider::thumbColourId,              accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, accent.darker (2.0f));
        slider.setColour (juce::Slider::rotarySliderFillColourId,   juce::Colour (0xff2b2b2b));   // the cap's body
    }

    inline void setupGainFader (juce::Component& owner, fxme::FxmeSlider& slider, const juce::String& name,
                                juce::Colour accent, juce::LookAndFeel& laf, double minDb = -24.0, double maxDb = 24.0,
                                bool vertical = true)
    {
        setupFader (owner, slider, name, accent, laf, vertical);
        slider.setRange (minDb, maxDb);
        slider.setValue (0.0);
        slider.setCentralValue (0.0);   // the fill grows from 0 dB
        slider.setNumDecimalPlacesToDisplay (1);
        slider.setTextValueSuffix (" dB");
    }
}
