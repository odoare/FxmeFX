/*
  ==============================================================================

    TransientComponent.cpp

  ==============================================================================
*/

#include "TransientComponent.h"
#include "../Common/Theme.h"
#include "../Common/Controls.h"
#include "../Common/TopBar.h"

void TransientComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

TransientComponent::TransientComponent (Transient& t, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle)
    : transientFx (t), apvts (state),
      onButton (state, prefix + "_Trans_On", "On", fxmefx::theme::transient)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (fxmefx::theme::transient);

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Transient", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (characterBox);
    characterBox.setLookAndFeel (&fxmeLookAndFeel);
    characterBox.setColour (juce::ComboBox::outlineColourId, fxmefx::theme::transient.darker());
    characterBox.setColour (juce::ComboBox::arrowColourId,   fxmefx::theme::transient.brighter (0.3f));
    characterBox.addItem ("Soft",     1);
    characterBox.addItem ("Standard", 2);
    characterBox.addItem ("Hard",     3);
    characterBox.setTooltip ("Character — selects the time-constant set");
    characterAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Trans_Character", characterBox);

    fxmefx::setupGainFader (*this, preGainSlider, "Pre Gain", fxmefx::theme::transient, fxmeLookAndFeel);
    setupSlider    (attackSlider,  attackLabel,  "Attack (%)",  -100.0, 100.0, 0.0);
    setupSlider    (sustainSlider, sustainLabel, "Sustain (%)", -100.0, 100.0, 0.0);
    fxmefx::setupGainFader (*this, gainSlider, "Gain", fxmefx::theme::transient, fxmeLookAndFeel);

    preGainSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_PreGain", preGainSlider));
    attackSlider .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Attack",  attackSlider));
    sustainSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Sustain", sustainSlider));
    gainSlider   .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Trans_Gain",    gainSlider));

    addAndMakeVisible (gainMeter);
    gainMeter.setMeterColour (fxmefx::theme::gainReduction);
    // Modification spans roughly ±18 dB: segments light up from 0 dB for a
    // boost, down for a cut.
    gainMeter.setRange (-18.0f, 18.0f);
    gainMeter.setScaleTicks ({ 18.0f, 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f });
    gainMeter.setTooltip ("Gain change, in dB: up for a boost, down for a cut. The number above holds the largest change for a moment.");
    startTimerHz (30);
}

TransientComponent::~TransientComponent()
{
}

void TransientComponent::setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = fxmefx::theme::transient;

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


void TransientComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), fxmefx::theme::transient);
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

    f1.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    f1.items.add (fi (titleLabel).withFlex (1.0f));
    f1.items.add (fi (characterBox).withFlex (0.7f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

    f2.items.add (fi (attackSlider).withFlex (1.f));
    f2.items.add (fi (sustainSlider).withFlex (1.f));

    f5.items.add (fi (preGainSlider).withFlex (0.15f));
    f5.items.add (fi (f2).withFlex (1.f));
    f5.items.add (fi (gainMeter).withFlex (0.15f).withMargin (juce::FlexItem::Margin (0.f, 10.f, 0.f, 0)));
    f5.items.add (fi (gainSlider).withFlex (0.15f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 0.f)));

    fMain.items.add (fi (f1).withHeight (fxmefx::kHeaderRowHeight)
                            .withMinHeight (fxmefx::kHeaderRowHeight)
                            .withMargin (juce::FlexItem::Margin (5.f, 0.f, 10.f, 0)));
    fMain.items.add (fi (f5).withFlex (1.f));

    fMain.performLayout (area);
}

void TransientComponent::timerCallback()
{
    gainMeter.setActive (transientFx.isOn());

    // VuMeter::getPeak() returns dB of the linear gain factor — i.e. the
    // current gain modification depth, signed.
    gainMeter.setValue (transientFx.getGainMeter().getPeak());
}
