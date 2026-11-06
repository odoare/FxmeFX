/*
  ==============================================================================

    GateComponent.h

    The Gate's controls. Two layouts:

      standard   the On switch, the title, the state LED, Listen and the mode
                 over two rows of five knobs (threshold, hysteresis, range,
                 ratio, look-ahead / attack, hold, release, sidechain
                 high-pass and low-pass), the meter (detector level against
                 the threshold, which can be dragged, and the gain) and the
                 Output fader on the right, for the plugin's own editor;
      compact    no title, flat knobs, the switches over one row of all the
                 knobs, the meter and Output as a knob, for a host that gives
                 the effect a wide and short slot.

    The ratio knob is only enabled in Expander mode (a gate's ratio is
    infinite).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/Theme.h"
#include "Gate.h"

class GateComponent : public juce::Component,
                      private juce::Timer
{
public:
    GateComponent (Gate& gateToControl,
                   juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& prefix,
                   bool showTitle = true,
                   bool compact = false);
    ~GateComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** The tint the gate is drawn in (its plugin's editor uses it too). */
    static juce::Colour tint() { return fxmefx::theme::gate; }

private:
    /** Open (green), hold (amber) or closed (dim red), as a LED and a word. */
    class StateLed : public juce::Component,
                     public juce::SettableTooltipClient
    {
    public:
        void setState (int newState, bool newActive);
        void paint (juce::Graphics& g) override;

    private:
        int state = 0;
        bool active = true;
    };

    void timerCallback() override;
    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);

    Gate& gate;
    juce::AudioProcessorValueTreeState& apvts;
    const juce::String threshId;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton, listenButton;
    juce::ComboBox modeBox;
    StateLed stateLed;
    fxme::DynamicsMeter meter;

    fxme::FxmeSlider threshSlider, hystSlider, rangeSlider, ratioSlider, lookaheadSlider,
                     attackSlider, holdSlider, releaseSlider, highPassSlider, lowPassSlider,
                     outSlider;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAtt;

    std::atomic<float>* modeParam   = nullptr;
    std::atomic<float>* threshParam = nullptr;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GateComponent)
};
