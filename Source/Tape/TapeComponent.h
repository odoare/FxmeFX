/*
  ==============================================================================

    TapeComponent.h

    The Tape's controls. Two layouts:

      standard   the On switch, the title and the machine's settings (type,
                 speed, EQ, oversampling, auto gain) on top; a row of four
                 knobs (drive, saturation, bias, hiss) over the hysteresis
                 loop; input and output VU meters, Mix and Output faders on
                 the right. For the plugin's own editor.
      compact    no title, no loop, no meters, flat knobs: the settings over
                 one row of six knobs (Mix and Output included), for a host
                 that gives the effect a wide and short slot.

    The EQ menu is disabled at 30 ips, where both standards are AES.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/Theme.h"
#include "Tape.h"

/** The tape's magnetisation M against the field H over the last few tens of
    milliseconds (the left channel): a thin line when the tape is barely
    touched, a widening, flattening loop as it is driven, and the loop's width
    moving with Bias. H's axis follows the signal (the larger of the
    current extent and 0.5), M's is fixed at +-1.1, so saturation reads as
    the loop flattening against the top and bottom. */
class HysteresisLoopView : public juce::Component,
                           private juce::Timer
{
public:
    HysteresisLoopView (fxme::WaveformTap& field, fxme::WaveformTap& magnetisation);
    ~HysteresisLoopView() override;

    void paint (juce::Graphics& g) override;

    juce::Colour accent { juce::Colours::orange };
    bool active = true;

private:
    void timerCallback() override;

    static constexpr int numPoints = 1024;

    fxme::WaveformTap& fieldTap;
    fxme::WaveformTap& magTap;
    std::vector<float> h, m;
    float hRange = 0.5f;
};

class TapeComponent : public juce::Component,
                      private juce::Timer
{
public:
    TapeComponent (Tape& tapeToControl,
                   juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& prefix,
                   bool showTitle = true,
                   bool compact = false);
    ~TapeComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** The tint the tape is drawn in (its plugin's editor uses it too). */
    static juce::Colour tint() { return fxmefx::theme::tape; }

private:
    static constexpr float kKnobRowHeight = 96.0f;
    static constexpr float kVuZeroDbfs = -18.0f;   // 0 VU on the meters

    void timerCallback() override;
    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, const juce::String& paramId,
                     const juce::String& tooltip);

    Tape& tape;
    juce::AudioProcessorValueTreeState& apvts;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton, autoButton;
    juce::ComboBox typeBox, speedBox, eqBox, osBox;
    fxme::FxmeSlider driveSlider, satSlider, biasSlider, hissSlider, mixSlider, outSlider;
    HysteresisLoopView loopView;
    fxme::VuMeterComponent inVu, outVu;
    juce::Label inLabel, outLabel;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAtts;

    std::atomic<float>* speedParam = nullptr;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeComponent)
};
