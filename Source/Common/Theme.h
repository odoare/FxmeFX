/*
  ==============================================================================

    Theme.h

    Every FxmeFX effect's accent colour, in one place. A component, its
    plugin editor (top bar, outer background) and anything embedding the
    component read it from here, so changing an effect's colour is one line.
    The colours are kept a little short of full saturation: on the
    near-black panels a pure primary glares (the Compressor's red did).

    Author: Olivier Doaré, github.com/odoare
    Dual-licensed, mirroring the JUCE framework it depends on: under the GNU
    AGPL Version 3.0, or under commercial terms available from the author.
    SPDX-License-Identifier: AGPL-3.0-or-later OR LicenseRef-FXME-Commercial

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

namespace fxmefx::theme
{
    // Dynamics
    inline const juce::Colour compressor   { 0xffdc4a44 };   // red, desaturated (was pure red)
    inline const juce::Colour transient    { 0xffdc4a44 };   // as the Compressor
    inline const juce::Colour limiter      { 0xffffa500 };   // orange

    /** Gain-reduction meters: warmer than the panels' accents, so the meter
        reads apart from the controls around it. */
    inline const juce::Colour gainReduction { 0xffe8704a };

    // Tone
    inline const juce::Colour equalizer    { 0xff00ffff };   // cyan
    inline const juce::Colour tube         { 0xffffa500 };   // orange
    inline const juce::Colour cab          { 0xffffa500 };   // orange

    // Modulation
    inline const juce::Colour chorus       { 0xff3cc8be };   // teal
    inline const juce::Colour flanger      { 0xff5a96f0 };   // steel blue
    inline const juce::Colour phaser       { 0xffe664b4 };   // magenta

    // Time and space
    inline const juce::Colour stereoDelay  { 0xff008000 };   // green
    inline const juce::Colour reverb       { 0xff967deb };   // lavender
    inline const juce::Colour convolReverb { 0xff9acd32 };   // yellow-green

    // Spectral and pitch
    inline const juce::Colour freeze       { 0xff8c64dc };   // violet
    inline const juce::Colour oct          { 0xff8c64dc };   // violet, as Freeze
}
