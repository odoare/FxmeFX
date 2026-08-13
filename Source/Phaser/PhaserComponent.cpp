/*
  ==============================================================================

    PhaserComponent.cpp

  ==============================================================================
*/

#include "PhaserComponent.h"
#include "../Common/TopBar.h"
#include <FxmeTools/dsp/Lfo.h>

namespace
{
    const auto phaserTint = juce::Colour::fromRGB (230, 100, 180); // magenta
}

void PhaserComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void PhaserComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
                                   double min, double max, double interval,
                                   const juce::String& suffix)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max, interval);
    slider.setTextValueSuffix (suffix);
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, phaserTint);
}

void PhaserComponent::setupCombo (juce::ComboBox& box, juce::Label& label,
                                  const juce::String& text, const juce::StringArray& items)
{
    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centredRight);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));

    addAndMakeVisible (box);
    box.setLookAndFeel (&fxmeLookAndFeel);
    box.addItemList (items, 1);
    box.setTooltip (text);
    box.setColour (juce::ComboBox::outlineColourId, phaserTint.darker());
    box.setColour (juce::ComboBox::arrowColourId,   phaserTint.brighter (0.3f));
}

PhaserComponent::PhaserComponent (Phaser& p,
                                  juce::AudioProcessorValueTreeState& state,
                                  const juce::String& prefix,
                                  bool showTitle)
    : phaser (p), apvts (state)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (phaserTint);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Phaser", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setColour (juce::ToggleButton::tickColourId, phaserTint);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Phaser_On", onButton);

    addAndMakeVisible (syncButton);
    syncButton.setButtonText ("Sync");
    syncButton.setTooltip ("Sync. \n Locks the LFO to the host tempo: the rate becomes the "
                           "musical division on the right, and the sweep is re-anchored to "
                           "the timeline on every block while the transport rolls.");
    syncButton.setColour (juce::ToggleButton::tickColourId, phaserTint);
    syncButton.setLookAndFeel (&fxmeLookAndFeel);
    syncAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Phaser_Sync", syncButton);

    setupCombo (shapeBox,  shapeLabel,  "Shape",  fxme::Lfo::shapeChoices());
    setupCombo (stagesBox, stagesLabel, "Stages", Phaser::stageChoices());
    setupCombo (divBox,    divLabel,    "Div",    fxme::Lfo::syncDivisionChoices());
    stagesBox.setTooltip ("Stages. \n Allpass sections in the chain. Each pair puts one notch "
                          "in the sum with the dry signal: 4 is the familiar pedal voice, "
                          "10 or 12 the thick resonant sweep.");
    shapeAtt  = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Phaser_Shape",  shapeBox);
    stagesAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Phaser_Stages", stagesBox);
    divAtt    = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Phaser_Div",    divBox);

    setupRotary (rateSlider,     "Rate",     0.01,  10.0,   0.001, " Hz");
    setupRotary (depthSlider,    "Depth",    0.0,   100.0,  0.1,   " %");
    setupRotary (freqSlider,     "Freq",     60.0,  4000.0, 1.0,   " Hz");
    setupRotary (feedbackSlider, "Feedback", -95.0, 95.0,   0.1,   " %");
    setupRotary (widthSlider,    "Width",    0.0,   100.0,  0.1,   " %");
    setupRotary (mixSlider,      "Mix",      0.0,   100.0,  0.1,   " %");
    setupRotary (outSlider,      "Output",   -24.0, 24.0,   0.1,   " dB");
    feedbackSlider.setCentralValue (0.0);
    outSlider     .setCentralValue (0.0);

    freqSlider.setTooltip ("Freq. \n Centre of the sweep. Depth sets how far either side of it "
                           "the notches travel (up to 2.5 octaves each way).");
    feedbackSlider.setTooltip ("Feedback. \n Recirculates the allpass chain, sharpening the "
                               "notches into resonant peaks. Negative values invert it for the "
                               "hollower variant.");
    widthSlider.setTooltip ("Width. \n Phase offset between the left and right channels' "
                            "modulation. At 0 % both sides sweep together (mono phasing).");

    rateSlider    .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Rate",     rateSlider));
    depthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Depth",    depthSlider));
    freqSlider    .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Freq",     freqSlider));
    feedbackSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Feedback", feedbackSlider));
    widthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Width",    widthSlider));
    mixSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Mix",      mixSlider));
    outSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Out",      outSlider));

    syncParam = apvts.getRawParameterValue (prefix + "_Phaser_Sync");
    lastSyncState = ! (syncParam != nullptr && *syncParam > 0.5f);   // force the first update
    startTimerHz (10);
}

PhaserComponent::~PhaserComponent() = default;

void PhaserComponent::timerCallback()
{
    // Only one of the two rate controls is meaningful at a time; the parameter
    // can also move from host automation, hence polling rather than onClick.
    const bool synced = (syncParam != nullptr && *syncParam > 0.5f);
    if (synced == lastSyncState)
        return;

    lastSyncState = synced;
    rateSlider.setEnabled (! synced);
    divBox.setEnabled (synced);
    divLabel.setEnabled (synced);
}

void PhaserComponent::paint (juce::Graphics& g)
{
    fxmefx::paintComponentBackground (g, getLocalBounds().toFloat(), phaserTint);
}

void PhaserComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, comboRow, knobRow1, knobRow2, fMain;
    top     .flexDirection = juce::FlexBox::Direction::row;
    comboRow.flexDirection = juce::FlexBox::Direction::row;
    knobRow1.flexDirection = juce::FlexBox::Direction::row;
    knobRow2.flexDirection = juce::FlexBox::Direction::row;
    fMain   .flexDirection = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));
    top.items.add (fi (syncButton).withWidth (70.0f));

    comboRow.items.add (fi (shapeLabel) .withWidth (44.0f));
    comboRow.items.add (fi (shapeBox)   .withFlex (1.0f) .withMargin (juce::FlexItem::Margin (0.f, 10.f, 0.f, 4.f)));
    comboRow.items.add (fi (stagesLabel).withWidth (48.0f));
    comboRow.items.add (fi (stagesBox)  .withFlex (0.7f) .withMargin (juce::FlexItem::Margin (0.f, 10.f, 0.f, 4.f)));
    comboRow.items.add (fi (divLabel)   .withWidth (30.0f));
    comboRow.items.add (fi (divBox)     .withFlex (0.9f) .withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f)));

    knobRow1.items.add (fi (rateSlider)    .withFlex (1.0f));
    knobRow1.items.add (fi (depthSlider)   .withFlex (1.0f));
    knobRow1.items.add (fi (freqSlider)    .withFlex (1.0f));
    knobRow1.items.add (fi (feedbackSlider).withFlex (1.0f));

    // Half-width spacers either side keep the three knobs of the second row the
    // same size as the four above them.
    knobRow2.items.add (fi().withFlex (0.5f));
    knobRow2.items.add (fi (widthSlider).withFlex (1.0f));
    knobRow2.items.add (fi (mixSlider)  .withFlex (1.0f));
    knobRow2.items.add (fi (outSlider)  .withFlex (1.0f));
    knobRow2.items.add (fi().withFlex (0.5f));

    fMain.items.add (fi (top)     .withHeight (fxmefx::kHeaderRowHeight)
                                  .withMinHeight (fxmefx::kHeaderRowHeight)
                                  .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (comboRow).withHeight (26.0f).withMinHeight (24.0f)
                                  .withMargin (juce::FlexItem::Margin (0.f, 4.f, 6.f, 4.f)));
    fMain.items.add (fi (knobRow1).withFlex (1.0f));
    fMain.items.add (fi (knobRow2).withFlex (1.0f));

    fMain.performLayout (area);
}
