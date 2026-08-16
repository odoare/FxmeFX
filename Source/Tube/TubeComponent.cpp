/*
  ==============================================================================

    TubeComponent.cpp

  ==============================================================================
*/

#include "TubeComponent.h"
#include "../Common/TopBar.h"

void TubeComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

TubeComponent::TubeComponent (Tube& t, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle, bool singleRow)
    : tube (t), apvts (state), knobsInSingleRow (singleRow)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (juce::Colours::orange);

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel(&fxmeLookAndFeel);
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, prefix + "_Tube_On", onButton);

    onButton.onClick = [this] {
        int size = 0;
        const char* data = nullptr;
        if (onButton.getToggleState())
            data = BinaryData::getNamedResource("tube2_png", size);
        else
            data = BinaryData::getNamedResource("tube2_bw_png", size);

        if (data)
            tubeImage.setImage(juce::ImageCache::getFromMemory(data, size));
    };
    onButton.onClick();

    addAndMakeVisible(tubeImage);
    tubeImage.toBack();

    addAndMakeVisible (modelBox);
    modelBox.setLookAndFeel (&fxmeLookAndFeel);
    modelBox.addItem ("Standard", 1);
    modelBox.addItem ("Dynamic",  2);
    modelBox.addItem ("Triode",   3);
    modelBox.addItem ("Class AB", 4);
    modelBox.setTooltip ("Tube Model. \n Standard: tanh. \n Dynamic: tanh + power-supply sag. \n Triode: asymmetric Dempwolf-style 12AX7 curve. \n Class AB: push-pull with crossover behaviour.");
    modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Tube_Model", modelBox);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Tube Saturation", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

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
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setCentralValue (0.0);   // bipolar ±dB: the bar grows from 0 dB
    slider.setTextValueSuffix ("dB");
    slider.setTooltip (text);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void TubeComponent::paint (juce::Graphics& g)
{
    fxmefx::paintComponentBackground (g, getLocalBounds().toFloat(), juce::Colours::orange);
}

void TubeComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;
    juce::FlexBox f1, f2, knobRow1, knobRow2, knobs, fMain;
    f1.flexDirection =  juce::FlexBox::Direction::row;
    f2.flexDirection = juce::FlexBox::Direction::row;
    knobRow1.flexDirection = juce::FlexBox::Direction::row;
    knobRow2.flexDirection = juce::FlexBox::Direction::row;
    knobs.flexDirection = juce::FlexBox::Direction::column;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    f1.items.add(fi(onButton).withWidth(fxmefx::kOnButtonWidth));
    f1.items.add(fi(titleLabel).withFlex(1.f));
    f1.items.add(fi(modelBox).withFlex(0.5f));

    // In a wide but short slot every vertical pixel counts, so the header keeps
    // its height (the button and combo still need it) but gives back its
    // margins.
    const float headerTopMargin    = knobsInSingleRow ?  2.f :  5.f;
    const float headerBottomMargin = knobsInSingleRow ?  4.f : 10.f;

    // The tube artwork spans the full content height at its native aspect
    // ratio; the header (plus its vertical margins) is what it can't have.
    const float contentHeight = juce::jmax (0.0f, (float) area.getHeight()
                                                  - fxmefx::kHeaderRowHeight
                                                  - headerTopMargin - headerBottomMargin);
    float imageWidth = 0.0f;
    if (auto img = tubeImage.getImage(); img.isValid() && img.getHeight() > 0)
        imageWidth = contentHeight * (float) img.getWidth() / (float) img.getHeight();

    const auto knobMargin = juce::FlexItem::Margin (5.f);

    if (knobsInSingleRow)
    {
        // One row: each knob gets the whole content height rather than half of
        // it. The slot is wide, so width is not the binding constraint.
        knobRow1.items.add(fi(driveSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow1.items.add(fi(toneSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow1.items.add(fi(biasSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow1.items.add(fi(sagSlider).withFlex(1.f).withMargin(knobMargin));
        knobs.items.add(fi(knobRow1).withFlex(1.f));
    }
    else
    {
        knobRow1.items.add(fi(driveSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow1.items.add(fi(toneSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow2.items.add(fi(biasSlider).withFlex(1.f).withMargin(knobMargin));
        knobRow2.items.add(fi(sagSlider).withFlex(1.f).withMargin(knobMargin));
        knobs.items.add(fi(knobRow1).withFlex(1.f));
        knobs.items.add(fi(knobRow2).withFlex(1.f));
    }

    f2.items.add(fi(tubeImage).withWidth(imageWidth));
    f2.items.add(fi(knobs).withFlex(1.f));
    f2.items.add(fi(outSlider).withWidth(60.f).withMargin(juce::FlexItem::Margin(0.f, 0.f, 0.f, 5.f)));

    fMain.items.add(fi(f1).withHeight(fxmefx::kHeaderRowHeight)
                          .withMinHeight(fxmefx::kHeaderRowHeight)
                          .withMargin(juce::FlexItem::Margin(headerTopMargin, 0.f, headerBottomMargin, 0.f)));
    fMain.items.add(fi(f2).withFlex(1.f));

    fMain.performLayout(area);
}