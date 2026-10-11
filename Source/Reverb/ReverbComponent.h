/*
  ==============================================================================

    ReverbComponent.h

    The Reverb's controls. Two layouts:

      standard   the On switch and the title over two rows of four knobs
                 (pre-delay, size, damping, width / low cut, high cut, mix,
                 output), for the plugin's own editor;
      compact    no title, flat knobs (FxmeLookAndFeel::KnobStyle::flat), the
                 On switch over one row of all eight knobs, for a host that
                 gives the effect a wide and short slot (Camshaft's panes).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Reverb.h"

class ReverbComponent : public juce::Component
{
public:
    ReverbComponent (ReverbEffect& reverbToControl,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& prefix,
                     bool showTitle = true,
                     bool compact = false);
    ~ReverbComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** The tint the reverb is drawn in (its plugin's editor uses it too). */
    static juce::Colour tint() { return juce::Colour::fromRGB (150, 125, 235); }

private:
    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);

    ReverbEffect& reverb;
    juce::AudioProcessorValueTreeState& apvts;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton;
    fxme::FxmeSlider preDelaySlider, sizeSlider, dampSlider, widthSlider,
                     lowCutSlider, highCutSlider, mixSlider, outSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbComponent)
};
