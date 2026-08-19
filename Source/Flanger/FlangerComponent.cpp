/*
  ==============================================================================

    FlangerComponent.cpp

  ==============================================================================
*/

#include "FlangerComponent.h"
#include "../Common/TopBar.h"
#include <FxmeTools/dsp/Lfo.h>

namespace
{
    const auto flangerTint = juce::Colour::fromRGB (90, 150, 240); // steel blue
}

void FlangerComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void FlangerComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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
    setSliderColours (slider, flangerTint);
}

void FlangerComponent::setupCombo (juce::ComboBox& box, juce::Label& label,
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
    box.setColour (juce::ComboBox::outlineColourId, flangerTint.darker());
    box.setColour (juce::ComboBox::arrowColourId,   flangerTint.brighter (0.3f));
}

FlangerComponent::FlangerComponent (Flanger& f,
                                    juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefix,
                                    bool showTitle)
    : flanger (f), apvts (state),
      onButton   (state, prefix + "_Flanger_On",   "On",   flangerTint),
      syncButton (state, prefix + "_Flanger_Sync", "Sync", flangerTint)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (flangerTint);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Flanger", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

    addAndMakeVisible (syncButton);
    syncButton.setLookAndFeel (&fxmeLookAndFeel);
    syncButton.button.setTooltip ("Sync. \n Locks the LFO to the host tempo: the rate becomes the "
                                  "musical division on the right, and the sweep is re-anchored to "
                                  "the timeline on every block while the transport rolls.");

    setupCombo (shapeBox, shapeLabel, "Shape", juce::StringArray (fxme::Lfo::shapeNames, fxme::Lfo::numShapes));
    setupCombo (divBox,   divLabel,   "Div",   juce::StringArray (fxme::Lfo::syncDivisionNames, fxme::Lfo::numSyncDivisions));
    shapeAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Flanger_Shape", shapeBox);
    divAtt   = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Flanger_Div",   divBox);

    setupRotary (rateSlider,     "Rate",     0.01,  10.0,  0.001, " Hz");
    setupRotary (depthSlider,    "Depth",    0.0,   100.0, 0.1,   " %");
    setupRotary (delaySlider,    "Delay",    0.1,   10.0,  0.01,  " ms");
    setupRotary (feedbackSlider, "Feedback", -95.0, 95.0,  0.1,   " %");
    setupRotary (widthSlider,    "Width",    0.0,   100.0, 0.1,   " %");
    setupRotary (mixSlider,      "Mix",      0.0,   100.0, 0.1,   " %");
    setupRotary (outSlider,      "Output",   -24.0, 24.0,  0.1,   " dB");
    feedbackSlider.setCentralValue (0.0);
    outSlider     .setCentralValue (0.0);

    delaySlider.setTooltip ("Delay. \n Shortest delay of the sweep (the \"manual\" control on a "
                            "pedal): it places the comb, Depth sets how far above it the LFO travels.");
    feedbackSlider.setTooltip ("Feedback. \n Sharpens the comb into the jet whoosh. Negative "
                               "values invert the feedback, turning the notches into peaks for "
                               "the hollower, more metallic flange.");
    widthSlider.setTooltip ("Width. \n Phase offset between the left and right channels' "
                            "modulation. At 0 % both sides sweep together (mono flange).");
    mixSlider.setTooltip ("Mix. \n The comb is deepest at 50 %, where the dry and the delayed "
                          "signal have equal weight.");

    rateSlider    .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Rate",     rateSlider));
    depthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Depth",    depthSlider));
    delaySlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Delay",    delaySlider));
    feedbackSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Feedback", feedbackSlider));
    widthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Width",    widthSlider));
    mixSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Mix",      mixSlider));
    outSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Flanger_Out",      outSlider));

    syncParam = apvts.getRawParameterValue (prefix + "_Flanger_Sync");
    lastSyncState = ! (syncParam != nullptr && *syncParam > 0.5f);   // force the first update
    startTimerHz (10);
}

FlangerComponent::~FlangerComponent() = default;

void FlangerComponent::timerCallback()
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

void FlangerComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), flangerTint);
}

void FlangerComponent::resized()
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

    comboRow.items.add (fi (shapeLabel).withWidth (48.0f));
    comboRow.items.add (fi (shapeBox)  .withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 12.f, 0.f, 4.f)));
    comboRow.items.add (fi (divLabel)  .withWidth (34.0f));
    comboRow.items.add (fi (divBox)    .withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f)));

    knobRow1.items.add (fi (rateSlider)    .withFlex (1.0f));
    knobRow1.items.add (fi (depthSlider)   .withFlex (1.0f));
    knobRow1.items.add (fi (delaySlider)   .withFlex (1.0f));
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
