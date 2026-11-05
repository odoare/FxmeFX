/*
  ==============================================================================

    LimiterComponent.cpp

  ==============================================================================
*/

#include "LimiterComponent.h"
#include "../Common/Theme.h"
#include "../Common/Controls.h"
#include "../Common/TopBar.h"

void LimiterComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

LimiterComponent::LimiterComponent (Limiter& lim, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle)
    : limiter (lim), apvts (state),
      onButton (state, prefix + "_Lim_On", "On", fxmefx::theme::limiter),
      truePeakButton (state, prefix + "_Lim_TruePeak", "True Peak", fxmefx::theme::limiter)
{
    // Tints this component's drop-down menus and tooltips; both are their own
    // windows and cannot see the widget that opened them.
    fxmeLookAndFeel.setAccentColour (fxmefx::theme::limiter);

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

    addAndMakeVisible (truePeakButton);
    truePeakButton.setLookAndFeel (&fxmeLookAndFeel);
    truePeakButton.button.setTooltip ("True Peak. \n Also limits the peaks between samples, those a DAC rebuilds "
                                      "(read on a 4x oversampled copy, as ITU-R BS.1770 does), so the ceiling holds "
                                      "in dBTP. Adds 6 samples of delay. For a master, a ceiling of -1 dB is usual.");

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Limiter", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    setupSlider (driveSlider,   "Drive (dB)",   0.0,  24.0,  0.0);
    setupSlider (ceilingSlider, "Ceiling (dB)", -24.0, 0.0, -0.3);
    setupSlider (releaseSlider, "Release (ms)", 1.0, 500.0, 50.0);

    driveSlider.setAttachment   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Drive",   driveSlider));
    ceilingSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Ceiling", ceilingSlider));
    releaseSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Release", releaseSlider));

    addAndMakeVisible (grMeter);
    grMeter.setMeterColour (fxmefx::theme::gainReduction);
    grMeter.setRange (-30.0f, 0.0f);
    grMeter.setTooltip ("Gain reduction, in dB. The number above holds the deepest reduction for a moment.");
    startTimerHz (30);
}

LimiterComponent::~LimiterComponent()
{
}

void LimiterComponent::setupSlider (fxme::FxmeSlider& slider, const juce::String& text, double min, double max, double def)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, fxmefx::theme::limiter);
}

void LimiterComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), fxmefx::theme::limiter);
}

void LimiterComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;
    juce::FlexBox header, knobs, fMain;
    header.flexDirection = juce::FlexBox::Direction::row;
    knobs.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection  = juce::FlexBox::Direction::column;

    header.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    header.items.add (fi (titleLabel).withFlex (1.0f));
    header.items.add (fi (truePeakButton).withWidth (96.0f)
                                         .withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 0.f)));

    knobs.items.add (fi (driveSlider).withFlex (1.f));
    knobs.items.add (fi (ceilingSlider).withFlex (1.f));
    knobs.items.add (fi (releaseSlider).withFlex (1.f));
    knobs.items.add (fi (grMeter).withFlex (0.25f).withMinWidth (40.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 8.f)));

    fMain.items.add (fi (header).withHeight (fxmefx::kHeaderRowHeight)
                                .withMinHeight (fxmefx::kHeaderRowHeight)
                                .withMargin (juce::FlexItem::Margin (5.f, 0.f, 10.f, 0.f)));
    fMain.items.add (fi (knobs).withFlex (1.f));

    fMain.performLayout (area);
}

void LimiterComponent::timerCallback()
{
    grMeter.setActive (limiter.isOn());
    grMeter.setValue (limiter.getGrMeter().getPeak());
}
