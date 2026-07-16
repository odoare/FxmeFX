/*
  ==============================================================================

    FreezeComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Freeze.h"

//==============================================================================
/** Stylized input-spectrum plot: animates with the live input and holds
    still (with a FROZEN badge) while the freeze is engaged. */
class SpectrumDisplay : public juce::Component,
                        private juce::Timer
{
public:
    SpectrumDisplay (Freeze& freezeToWatch, juce::Colour accentColour);
    ~SpectrumDisplay() override;

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override;
    void updateSpectrum();

    static constexpr int kFftOrder = 11;
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kPoints   = 96;              // log-spaced plot points
    static constexpr float kFloorDb = -80.0f;
    static constexpr float kMinHz   = 25.0f;
    static constexpr float kMaxHz   = 18000.0f;

    Freeze& freeze;
    juce::Colour accent;

    juce::dsp::FFT fft { kFftOrder };
    std::array<float, (size_t) kFftSize>     hannWindow {};
    std::array<float, (size_t) kFftSize>     sampleRing {};
    std::array<float, (size_t) 2 * kFftSize> fftData {};
    std::array<float, (size_t) kPoints>      smoothedDb {};
    int  ringPos    = 0;
    bool wasFrozen  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};

//==============================================================================
class FreezeComponent : public juce::Component
{
public:
    FreezeComponent (Freeze& freezeToControl,
                     juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& prefix);
    ~FreezeComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    Freeze& freeze;
    juce::AudioProcessorValueTreeState& apvts;

    juce::Label        titleLabel;
    juce::ToggleButton onButton;

    SpectrumDisplay  spectrumDisplay;
    fxme::FxmeSlider mixSlider, widthSlider;
    juce::Label      mixLabel, widthLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;

    void setupRotary (fxme::FxmeSlider& slider, juce::Label& label,
                      const juce::String& text, double min, double max, double def,
                      const juce::String& suffix);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreezeComponent)
};
