/*
  ==============================================================================

    Theme.h

    Every FxmeFX effect's accent colour, in one place. A component, its
    plugin editor (top bar, outer background) and anything embedding the
    component read it from here.

    The colours follow the effect families: each family runs along a
    gradient between two colours, and its members are spread evenly along
    it, in the order of the family's enum. A new effect is one more enum
    entry (before `count`) and one line below: the family's other colours
    move along the gradient to make room, which keeps a family reading as
    one.

      Dynamics            brown   to red
      Modulation          teal    to steel blue
      Distortion and tone orange  to apricot
      Equalisation        cyan
      Delay               green   to olive
      Reverb              yellow  to beige
      Spectral and pitch  violet  to pink

    The ends are kept a little short of full saturation: on the near-black
    panels a pure primary glares.

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
    /** Member `index` of `count`, spread evenly from `from` to `to` (a
        family of one takes `from`). */
    inline juce::Colour along (juce::Colour from, juce::Colour to, int index, int count)
    {
        return count <= 1 ? from : from.interpolatedWith (to, (float) index / (float) (count - 1));
    }

    //==============================================================================
    // The families: their two ends and their members, in gradient order.

    namespace dynamics     { enum Member { limiter, transient, compressor, gate, count };
                             inline const juce::Colour from { 0xffb0703c }, to { 0xffdc4a44 }; }   // brown, red

    namespace modulation   { enum Member { chorus, phaser, flanger, count };
                             inline const juce::Colour from { 0xff3cc8be }, to { 0xff5a96f0 }; }   // teal, steel blue

    namespace tone         { enum Member { tube, cab, count };
                             inline const juce::Colour from { 0xfff0a030 }, to { 0xfffbc490 }; }   // orange, apricot (salmon was too near the dynamics' red)

    namespace equalisation { enum Member { equalizer, count };
                             inline const juce::Colour from { 0xff30dcf0 }, to { 0xff30dcf0 }; }   // cyan

    namespace delay        { enum Member { stereoDelay, count };
                             inline const juce::Colour from { 0xff4cb450 }, to { 0xffa0a040 }; }   // green, olive

    namespace reverbs      { enum Member { reverb, convolReverb, count };
                             inline const juce::Colour from { 0xffe8cc40 }, to { 0xffd9c9a3 }; }   // yellow, beige

    namespace spectral     { enum Member { freeze, oct, count };
                             inline const juce::Colour from { 0xff8c64dc }, to { 0xffe678b4 }; }   // violet, pink

    //==============================================================================
    // Each effect's accent.

    inline const juce::Colour limiter      = along (dynamics::from, dynamics::to, dynamics::limiter,    dynamics::count);
    inline const juce::Colour transient    = along (dynamics::from, dynamics::to, dynamics::transient,  dynamics::count);
    inline const juce::Colour compressor   = along (dynamics::from, dynamics::to, dynamics::compressor, dynamics::count);
    inline const juce::Colour gate         = along (dynamics::from, dynamics::to, dynamics::gate,       dynamics::count);

    inline const juce::Colour chorus       = along (modulation::from, modulation::to, modulation::chorus,  modulation::count);
    inline const juce::Colour phaser       = along (modulation::from, modulation::to, modulation::phaser,  modulation::count);
    inline const juce::Colour flanger      = along (modulation::from, modulation::to, modulation::flanger, modulation::count);

    inline const juce::Colour tube         = along (tone::from, tone::to, tone::tube, tone::count);
    inline const juce::Colour cab          = along (tone::from, tone::to, tone::cab,  tone::count);

    inline const juce::Colour equalizer    = along (equalisation::from, equalisation::to, equalisation::equalizer, equalisation::count);

    inline const juce::Colour stereoDelay  = along (delay::from, delay::to, delay::stereoDelay, delay::count);

    inline const juce::Colour reverb       = along (reverbs::from, reverbs::to, reverbs::reverb,       reverbs::count);
    inline const juce::Colour convolReverb = along (reverbs::from, reverbs::to, reverbs::convolReverb, reverbs::count);

    inline const juce::Colour freeze       = along (spectral::from, spectral::to, spectral::freeze, spectral::count);
    inline const juce::Colour oct          = along (spectral::from, spectral::to, spectral::oct,    spectral::count);

    /** Gain-reduction meters: warmer than the panels' accents, so the meter
        reads apart from the controls around it. */
    inline const juce::Colour gainReduction { 0xffe8704a };
}
