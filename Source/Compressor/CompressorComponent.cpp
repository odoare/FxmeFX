/*
  ==============================================================================

    CompressorComponent.cpp

  ==============================================================================
*/

#include "CompressorComponent.h"

void CompressorComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

CompressorComponent::CompressorComponent (Compressor& comp, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : compressor (comp), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel(&fxmeLookAndFeel);
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Comp_On", onButton);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Compressor", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupBarSlider (preGainSlider, preGainLabel, "Pre Gain", -24.0, 24.0, 0.0);
    setupSlider (attackSlider, attackLabel, "Attack (ms)", 0.1, 100.0, 10.0);
    setupSlider (releaseSlider, releaseLabel, "Release (ms)", 10.0, 1000.0, 100.0);
    setupSlider (threshSlider, threshLabel, "Thresh (dB)", -60.0, 0.0, 0.0);
    setupSlider (ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, 1.0);
    setupSlider (kneeSlider, kneeLabel, "Knee (dB)", 0.0, 24.0, 0.0);
    setupSlider (peakRmsSlider, peakRmsLabel, "Peak / RMS", 0.0, 1.0, 0.0);
    setupBarSlider (gainSlider, gainLabel, "Gain (dB)", -24.0, 24.0, 0.0);

    addAndMakeVisible (relModeBox);
    relModeBox.addItem ("Linear",  1);
    relModeBox.addItem ("Opto",    2);
    relModeBox.addItem ("Vintage", 3);
    relModeBox.setTooltip ("Release Mode — Linear: fixed release. Opto: release slows with instantaneous GR. Vintage: release slows with sustained GR.");
    relModeAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Comp_RelMode", relModeBox);

    preGainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_PreGain", preGainSlider));
    attackSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Attack", attackSlider));
    releaseSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Release", releaseSlider));
    threshSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Thresh", threshSlider));
    ratioSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Ratio", ratioSlider));
    kneeSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Knee", kneeSlider));
    peakRmsSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_PeakRms", peakRmsSlider));
    gainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_Gain", gainSlider));

    addAndMakeVisible (grMeter);
    grMeter.setMeterColor (juce::Colours::red);
    grMeter.setRange (-30.0f, 0.0f);
    grMeter.setZeroLevel (0.0f);
    startTimerHz (24);
}

CompressorComponent::~CompressorComponent()
{
}

void CompressorComponent::setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::red;

    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void CompressorComponent::setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::red;

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTextValueSuffix ("dB");
    slider.setTooltip (text);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void CompressorComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    //auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    auto bluegreengrey = juce::Colours::red.darker(4.f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void CompressorComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;
    juce::FlexBox f1, f2, f3, f4, f5, fMain;
    f1.flexDirection = juce::FlexBox::Direction::row;
    f2.flexDirection = juce::FlexBox::Direction::row;
    f3.flexDirection = juce::FlexBox::Direction::row;
    f4.flexDirection = juce::FlexBox::Direction::column;
    f5.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    f1.items.add(fi(onButton).withFlex(0.2f));
    f1.items.add(fi(titleLabel).withFlex(1.0f));
    f1.items.add(fi(relModeBox).withFlex(0.7f).withMargin(juce::FlexItem::Margin(0.f, 4.f, 0.f, 4.f)));
    f2.items.add(fi(attackSlider).withFlex(1.f));
    f2.items.add(fi(releaseSlider).withFlex(1.f));
    f2.items.add(fi(kneeSlider).withFlex(1.f));
    f3.items.add(fi(threshSlider).withFlex(1.f));
    f3.items.add(fi(ratioSlider).withFlex(1.f));
    f3.items.add(fi(peakRmsSlider).withFlex(1.f));
    f4.items.add(fi(f2).withFlex(1.f));
    f4.items.add(fi(f3).withFlex(1.f));
    f5.items.add(fi(preGainSlider).withFlex(0.15f));
    f5.items.add(fi(f4).withFlex(1.f));
    f5.items.add(fi(grMeter).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 0)));
    f5.items.add(fi(gainSlider).withFlex(0.15f));

    fMain.items.add(fi(f1).withFlex(0.11f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 10.f, 0)));
    fMain.items.add(fi(f5).withFlex(1.f));

    fMain.performLayout(area);
}

void CompressorComponent::timerCallback()
{
    if (compressor.isOn())
        grMeter.setMeterColor (juce::Colours::red);
    else
        grMeter.setMeterColor (juce::Colours::grey);

    grMeter.setValue (compressor.getGrMeter().getPeak());
}