/*
  ==============================================================================

    LimiterComponent.cpp

  ==============================================================================
*/

#include "LimiterComponent.h"

void LimiterComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

LimiterComponent::LimiterComponent (Limiter& lim, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : limiter (lim), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onButton.setColour (juce::ToggleButton::tickColourId, juce::Colours::orange);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Lim_On", onButton);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Limiter", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupSlider (driveSlider,   "Drive (dB)",   0.0,  24.0,  0.0);
    setupSlider (ceilingSlider, "Ceiling (dB)", -24.0, 0.0, -0.3);
    setupSlider (releaseSlider, "Release (ms)", 1.0, 500.0, 50.0);

    driveSlider.setAttachment   (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Drive",   driveSlider));
    ceilingSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Ceiling", ceilingSlider));
    releaseSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Lim_Release", releaseSlider));

    addAndMakeVisible (grMeter);
    grMeter.setMeterColor (juce::Colours::orange);
    grMeter.setRange (-30.0f, 0.0f);
    grMeter.setZeroLevel (0.0f);
    startTimerHz (24);
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
    setSliderColours (slider, juce::Colours::orange);
}

void LimiterComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto base = juce::Colours::orange.darker (4.f);
    juce::ColourGradient grad (base.darker().darker().darker(), perpendicular * height,
                               base, perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillAll();
}

void LimiterComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;
    juce::FlexBox header, knobs, fMain;
    header.flexDirection = juce::FlexBox::Direction::row;
    knobs.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection  = juce::FlexBox::Direction::column;

    header.items.add (fi (onButton).withFlex (0.3f));
    header.items.add (fi (titleLabel).withFlex (1.0f));

    knobs.items.add (fi (driveSlider).withFlex (1.f));
    knobs.items.add (fi (ceilingSlider).withFlex (1.f));
    knobs.items.add (fi (releaseSlider).withFlex (1.f));
    knobs.items.add (fi (grMeter).withFlex (0.25f).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 8.f)));

    fMain.items.add (fi (header).withFlex (0.15f).withMargin (juce::FlexItem::Margin (5.f, 0.f, 10.f, 0.f)));
    fMain.items.add (fi (knobs).withFlex (1.f));

    fMain.performLayout (area);
}

void LimiterComponent::timerCallback()
{
    grMeter.setMeterColor (limiter.isOn() ? juce::Colours::orange : juce::Colours::grey);
    grMeter.setValue (limiter.getGrMeter().getPeak());
}
