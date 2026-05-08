/*
  ==============================================================================

    TubeComponent.cpp

  ==============================================================================
*/

#include "TubeComponent.h"

void TubeComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

TubeComponent::TubeComponent (Tube& t, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : tube (t), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel(&fxmeLookAndFeel);
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, prefix + "_Tube_On", onButton);

    onButton.onClick = [this] {
        int size = 0;
        const char* data = nullptr;
        if (onButton.getToggleState())
            data = BinaryData::getNamedResource("tube_png", size);
        else
            data = BinaryData::getNamedResource("tube_bw_png", size);

        if (data)
            tubeImage.setImage(juce::ImageCache::getFromMemory(data, size));
    };
    onButton.onClick();

    addAndMakeVisible(tubeImage);
    tubeImage.toBack();

    addAndMakeVisible (modelBox);
    modelBox.addItem ("Standard", 1);
    modelBox.addItem ("Dynamic",  2);
    modelBox.addItem ("Triode",   3);
    modelBox.addItem ("Class AB", 4);
    modelBox.setTooltip ("Tube Model. \n Standard: tanh. \n Dynamic: tanh + power-supply sag. \n Triode: asymmetric Dempwolf-style 12AX7 curve. \n Class AB: push-pull with crossover behaviour.");
    modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Tube_Model", modelBox);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Tube Saturation", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    setupSlider (driveSlider, driveLabel, "Drive (dB)", 0.0, 40.0, 0.0);
    setupSlider (biasSlider,  biasLabel,  "Bias",        0.0, 0.5,  0.0);
    setupSlider (toneSlider,  toneLabel,  "Tone",       -1.0, 1.0,  0.0);
    setupSlider (sagSlider,   sagLabel,   "Sag",         0.0, 1.0,  0.5);
    setupBarSlider (outSlider, outLabel, "Output (dB)", -20.0, 20.0, 0.0);

    toneSlider.setTooltip ("Tone. \n Pre-emphasis before saturation, matching cut after. \n Positive = bright/edgy, negative = warm/dark.");
    sagSlider.setTooltip  ("Sag. \n Power-supply rail droop. \n Affects Dynamic, Triode, and Class AB models. Standard ignores it.");

    driveSlider.setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Tube_Drive", driveSlider));
    biasSlider .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Tube_Bias",  biasSlider));
    toneSlider .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Tube_Tone",  toneSlider));
    sagSlider  .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Tube_Sag",   sagSlider));
    outSlider  .setAttachment (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Tube_Out",   outSlider));
}

TubeComponent::~TubeComponent() {}

void TubeComponent::setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::orange;

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
    setSliderColours(slider, color);
}

void TubeComponent::setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::orange;

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTextValueSuffix ("dB");
    slider.setTooltip (text);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void TubeComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    //auto bluegreengrey = juce::Colour::fromFloatRGBA (0.15f, 0.15f, 0.25f, 1.0f);
    auto bluegreengrey = juce::Colours::orange.darker(4.f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                           bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void TubeComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;    
    juce::FlexBox f1, f2, fMain;
    f1.flexDirection =  juce::FlexBox::Direction::row;
    f2.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    f1.items.add(fi(onButton).withFlex(0.2f));
    f1.items.add(fi(titleLabel).withFlex(1.f));
    f1.items.add(fi(modelBox).withFlex(0.5f));
    f2.items.add(fi(tubeImage).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 5.f, 0.f)));
    f2.items.add(fi(driveSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 5.f, 5.f)));
    f2.items.add(fi(biasSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 5.f, 5.f)));
    f2.items.add(fi(toneSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 5.f, 5.f)));
    f2.items.add(fi(sagSlider).withFlex(1.f).withMargin(juce::FlexItem::Margin(5.f, 5.f, 5.f, 5.f)));
    f2.items.add(fi(outSlider).withFlex(0.3f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 5.f, 5.f)));

    fMain.items.add(fi(f1).withFlex(0.2f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 10.f, 0)));
    fMain.items.add(fi(f2).withFlex(0.9f));

    fMain.performLayout(area);
}