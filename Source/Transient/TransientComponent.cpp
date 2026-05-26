/*
  ==============================================================================

    TransientComponent.cpp

  ==============================================================================
*/

#include "TransientComponent.h"

void TransientComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

TransientComponent::TransientComponent (Transient& t, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : transientFx (t), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onButton.setColour (juce::ToggleButton::tickColourId, juce::Colours::red);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Trans_On", onButton);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Transient", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    addAndMakeVisible (characterBox);
    characterBox.addItem ("Soft",     1);
    characterBox.addItem ("Standard", 2);
    characterBox.addItem ("Hard",     3);
    characterBox.setTooltip ("Character — selects the time-constant set");
    characterAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Trans_Character", characterBox);

    setupBarSlider (preGainSlider, preGainLabel, "Pre Gain",   -24.0,  24.0, 0.0);
    setupSlider    (attackSlider,  attackLabel,  "Attack (%)",  -100.0, 100.0, 0.0);
    setupSlider    (sustainSlider, sustainLabel, "Sustain (%)", -100.0, 100.0, 0.0);
    setupBarSlider (gainSlider,    gainLabel,    "Gain (dB)",  -24.0,  24.0, 0.0);

    preGainSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_PreGain", preGainSlider));
    attackSlider .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Attack",  attackSlider));
    sustainSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Sustain", sustainSlider));
    gainSlider   .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Gain",    gainSlider));

    addAndMakeVisible (gainMeter);
    gainMeter.setMeterColor (juce::Colours::red);
    // Modification spans roughly ±18 dB; show that range with 0 dB as "no change".
    gainMeter.setRange (-18.0f, 18.0f);
    gainMeter.setZeroLevel (0.0f);
    startTimerHz (24);
}

TransientComponent::~TransientComponent()
{
}

void TransientComponent::setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
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
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, color);
}

void TransientComponent::setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::red;

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTextValueSuffix ("dB");
    slider.setTooltip (text);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, color);
}

void TransientComponent::paint (juce::Graphics& g)
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

void TransientComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;
    juce::FlexBox f1, f2, f5, fMain;
    f1.flexDirection   = juce::FlexBox::Direction::row;
    f2.flexDirection   = juce::FlexBox::Direction::row;
    f5.flexDirection   = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    f1.items.add (fi (onButton).withFlex (0.2f));
    f1.items.add (fi (titleLabel).withFlex (1.0f));
    f1.items.add (fi (characterBox).withFlex (0.7f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

    f2.items.add (fi (attackSlider).withFlex (1.f));
    f2.items.add (fi (sustainSlider).withFlex (1.f));

    f5.items.add (fi (preGainSlider).withFlex (0.15f));
    f5.items.add (fi (f2).withFlex (1.f));
    f5.items.add (fi (gainMeter).withFlex (0.15f).withMargin (juce::FlexItem::Margin (0.f, 10.f, 0.f, 0)));
    f5.items.add (fi (gainSlider).withFlex (0.15f));

    fMain.items.add (fi (f1).withFlex (0.15f).withMargin (juce::FlexItem::Margin (5.f, 0.f, 10.f, 0)));
    fMain.items.add (fi (f5).withFlex (1.f));

    fMain.performLayout (area);
}

void TransientComponent::timerCallback()
{
    if (transientFx.isOn())
        gainMeter.setMeterColor (juce::Colours::orange);
    else
        gainMeter.setMeterColor (juce::Colours::grey);

    // VuMeter::getPeak() returns dB of the linear gain factor — i.e. the
    // current gain modification depth, signed.
    gainMeter.setValue (transientFx.getGainMeter().getPeak());
}
