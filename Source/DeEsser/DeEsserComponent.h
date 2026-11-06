/*
  ==============================================================================

    DeEsserComponent.h

    The De-esser's controls. Two layouts:

      standard   the switches and menus on top; a small spectrum (input and
                 output) with the detector's band drawn on it, whose handle
                 drags the frequency, over a row of five knobs (frequency, Q,
                 threshold, range, speed); the meter (the detector's level
                 against the threshold, which can be dragged, and the gain
                 reduction) on the right. For the plugin's own editor.
      compact    no title, no spectrum, flat knobs: the switches over one row
                 of the knobs and the meter, for a host that gives the effect
                 a wide and short slot.

    The Q knob is only enabled with the band-pass detector.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/Theme.h"
#include "DeEsser.h"

/** The spectrum with the detector's band over it: shaded (more strongly
    while it is reducing), a line at its frequency and a handle on top that
    drags it. The values come in through setBand(); a drag only reports. */
class DeEsserSpectrum : public fxme::SpectrumDisplay
{
public:
    void setBand (float freqHz, float q, bool bandPass, float gainDb, bool active);

    std::function<void()> onFrequencyDragStart;
    std::function<void (float)> onFrequencyDrag;
    std::function<void()> onFrequencyDragEnd;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;

    juce::Colour accent { juce::Colours::orange };

protected:
    void paintBehindTraces (juce::Graphics& g, juce::Rectangle<float> plot) override;
    void paintOverTraces (juce::Graphics& g, juce::Rectangle<float> plot) override;

private:
    bool nearHandle (juce::Point<float> p) const;
    float xToFreq (float x) const;

    float freqHz = 6000.0f, q = 1.5f, gainDb = 0.0f;
    bool bandPass = true, active = true, draggingBand = false;
};

class DeEsserComponent : public juce::Component,
                         private juce::Timer
{
public:
    DeEsserComponent (DeEsser& deEsserToControl,
                      juce::AudioProcessorValueTreeState& apvts,
                      const juce::String& prefix,
                      bool showTitle = true,
                      bool compact = false);
    ~DeEsserComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

    /** The tint the de-esser is drawn in (its plugin's editor uses it too). */
    static juce::Colour tint() { return fxmefx::theme::deEsser; }

private:
    static constexpr float kKnobRowHeight = 96.0f;

    void timerCallback() override;
    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);
    void setupCombo (juce::ComboBox& box, const juce::StringArray& items, const juce::String& paramId,
                     const juce::String& tooltip);
    void beginGesture (const juce::String& id);
    void setParam (const juce::String& id, float value);
    void endGesture (const juce::String& id);

    DeEsser& deEsser;
    juce::AudioProcessorValueTreeState& apvts;
    const juce::String prefix;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton, listenButton, relativeButton;
    juce::ComboBox modeBox, filterBox, stereoBox;
    fxme::FxmeSlider freqSlider, qSlider, threshSlider, rangeSlider, speedSlider;
    fxme::DynamicsMeter meter;
    DeEsserSpectrum spectrum;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<ComboBoxAttachment>> comboAtts;

    std::atomic<float>* freqParam   = nullptr;
    std::atomic<float>* qParam      = nullptr;
    std::atomic<float>* filterParam = nullptr;
    std::atomic<float>* threshParam = nullptr;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeEsserComponent)
};
