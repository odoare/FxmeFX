/*
  ==============================================================================

    CrusherComponent.h

    The Crusher's controls. Two layouts:

      standard   the On switch, the title, Clean, Smooth and the quantizer
                 mode over a row of five knobs (drive, bits, rate, jitter,
                 noise) and a waveform view of the last 20 ms or so (input
                 and output of the left channel, where the steps show), Mix
                 and Output as console faders on the right, for the plugin's
                 own editor;
      compact    no title, flat knobs, the switches over one row of all seven
                 knobs, no waveform, for a host that gives the effect a wide
                 and short slot.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/Theme.h"
#include "Crusher.h"

class CrusherComponent : public juce::Component
{
public:
    CrusherComponent (Crusher& crusherToControl,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& prefix,
                      bool showTitle = true,
                      bool compact = false);
    ~CrusherComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** The tint the crusher is drawn in (its plugin's editor uses it too). */
    static juce::Colour tint() { return fxmefx::theme::crusher; }

private:
    static constexpr float kKnobRowHeight = 96.0f;

    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);

    Crusher& crusher;
    juce::AudioProcessorValueTreeState& apvts;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton, cleanButton, smoothButton;
    juce::ComboBox modeBox;
    fxme::FxmeSlider driveSlider, bitsSlider, rateSlider, jitterSlider, noiseSlider, mixSlider, outSlider;
    fxme::WaveformDisplay waveform;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrusherComponent)
};
