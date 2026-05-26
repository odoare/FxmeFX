/*
  ==============================================================================

    OctComponent.cpp

  ==============================================================================
*/

#include "OctComponent.h"

namespace
{
    const auto octTint = juce::Colour::fromRGB (140, 100, 220); // violet
}

void OctComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void OctComponent::setupRotary (fxme::FxmeSlider& slider, juce::Label& label,
                                const juce::String& text,
                                double min, double max, double def,
                                const juce::String& suffix)
{
    juce::ignoreUnused (label);
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTextValueSuffix (suffix);
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, octTint);
}

OctComponent::OctComponent (Oct& o,
                            juce::AudioProcessorValueTreeState& state,
                            const juce::String& prefix)
    : oct (o), apvts (state)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Octaver", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setColour (juce::ToggleButton::tickColourId, octTint);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Oct_On", onButton);

    setupRotary (drySlider,    dryLabel,    "Dry",    -60.0, 12.0,  0.0,   " dB");
    setupRotary (oct1Slider,   oct1Label,   "-1 Oct", -60.0, 12.0, -6.0,   " dB");
    setupRotary (oct2Slider,   oct2Label,   "-2 Oct", -60.0, 12.0, -60.0,  " dB");
    setupRotary (detectSlider, detectLabel, "Detect",  60.0, 1500.0, 400.0,  " Hz");
    setupRotary (toneSlider,   toneLabel,   "Tone",   300.0, 10000.0, 2000.0, " Hz");

    drySlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Oct_Dry",    drySlider));
    oct1Slider  .setAttachment (new SliderAttachment (apvts, prefix + "_Oct_Oct1",   oct1Slider));
    oct2Slider  .setAttachment (new SliderAttachment (apvts, prefix + "_Oct_Oct2",   oct2Slider));
    detectSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Oct_Detect", detectSlider));
    toneSlider  .setAttachment (new SliderAttachment (apvts, prefix + "_Oct_Tone",   toneSlider));
}

OctComponent::~OctComponent() = default;

void OctComponent::paint (juce::Graphics& g)
{
    auto diagonale     = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length        = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height        = float (getWidth() * getHeight()) / length;
    auto base          = octTint.darker (3.0f);
    juce::ColourGradient grad (base.darker().darker(),  perpendicular *  height,
                               base,                     perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillAll();
}

void OctComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, mixRow, detectRow, fMain;
    top.flexDirection       = juce::FlexBox::Direction::row;
    mixRow.flexDirection    = juce::FlexBox::Direction::row;
    detectRow.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection     = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withFlex (0.15f));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    mixRow.items.add (fi (drySlider) .withFlex (1.0f));
    mixRow.items.add (fi (oct1Slider).withFlex (1.0f));
    mixRow.items.add (fi (oct2Slider).withFlex (1.0f));

    detectRow.items.add (fi (detectSlider).withFlex (1.0f));
    detectRow.items.add (fi (toneSlider)  .withFlex (1.0f));

    fMain.items.add (fi (top)      .withFlex (0.15f).withMargin (juce::FlexItem::Margin (5.f, 0.f, 8.f, 0.f)));
    fMain.items.add (fi (mixRow)   .withFlex (1.0f));
    fMain.items.add (fi (detectRow).withFlex (1.0f).withMargin (juce::FlexItem::Margin (4.f, 0.f, 0.f, 0.f)));

    fMain.performLayout (area);
}
