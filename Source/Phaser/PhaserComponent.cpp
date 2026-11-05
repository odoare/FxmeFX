/*
  ==============================================================================

    PhaserComponent.cpp

  ==============================================================================
*/

#include "PhaserComponent.h"
#include "../Common/Theme.h"
#include "../Common/Controls.h"
#include "../Common/TopBar.h"
#include <FxmeTools/dsp/Lfo.h>

namespace
{
    const auto phaserTint = fxmefx::theme::phaser;
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
                                  bool showTitle,
                                  bool compactLayout)
    : phaser (p), apvts (state), compact (compactLayout),
      onButton   (state, prefix + "_Phaser_On",   "On",   phaserTint),
      syncButton (state, prefix + "_Phaser_Sync", "Sync", phaserTint)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (phaserTint);

    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Phaser", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

    addAndMakeVisible (syncButton);
    syncButton.setLookAndFeel (&fxmeLookAndFeel);
    syncButton.button.setTooltip ("Sync. \n Locks the LFO to the host tempo: the rate becomes the "
                                  "musical division on the right, and the sweep is re-anchored to "
                                  "the timeline on every block while the transport rolls.");

    setupCombo (shapeBox,  shapeLabel,  "Shape",  juce::StringArray (fxme::Lfo::shapeNames, fxme::Lfo::numShapes));
    setupCombo (stagesBox, stagesLabel, "Stages", Phaser::stageChoices());
    setupCombo (divBox,    divLabel,    "Div",    juce::StringArray (fxme::Lfo::syncDivisionNames, fxme::Lfo::numSyncDivisions));
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
    fxmefx::setupGainFader (*this, outSlider, "Output", phaserTint, fxmeLookAndFeel);
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
    freqSlider    .setLogarithmic();
    feedbackSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Feedback", feedbackSlider));
    widthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Width",    widthSlider));
    mixSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Mix",      mixSlider));
    outSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Phaser_Out",      outSlider));

    addAndMakeVisible (phaseDisplay);
    phaseDisplay.setAccentColour (phaserTint);
    phaseDisplay.setTooltip ("The LFO: its shape, and where the left and right channels read it.");

    syncParam  = apvts.getRawParameterValue (prefix + "_Phaser_Sync");
    onParam    = apvts.getRawParameterValue (prefix + "_Phaser_On");
    shapeParam = apvts.getRawParameterValue (prefix + "_Phaser_Shape");
    widthParam = apvts.getRawParameterValue (prefix + "_Phaser_Width");
    rateParam  = apvts.getRawParameterValue (prefix + "_Phaser_Rate");
    divParam   = apvts.getRawParameterValue (prefix + "_Phaser_Div");
    lastSyncState = ! (syncParam != nullptr && *syncParam > 0.5f);   // force the first update
    startTimerHz (30);
}

PhaserComponent::~PhaserComponent() = default;

void PhaserComponent::timerCallback()
{
    updatePhaseDisplay();

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

void PhaserComponent::updatePhaseDisplay()
{
    // Everything but the phase comes from the parameters, not from the DSP:
    // a mirror (FxmeNet) never processes, and only the phase is sent to it.
    const float width = widthParam != nullptr ? widthParam->load() * 0.01f * 0.5f : 0.5f;   // as Phaser::checkParameters
    phaseDisplay.setDots ({ { 0.0f, "L", false }, { width, "R", false } });

    if (shapeParam != nullptr)
        phaseDisplay.setShape ((int) shapeParam->load());
    phaseDisplay.setActive (onParam == nullptr || onParam->load() > 0.5f);
    phaseDisplay.setPhase (phaser.getDisplayPhase().load (std::memory_order_relaxed));

    const bool synced = syncParam != nullptr && syncParam->load() > 0.5f;
    if (synced && divParam != nullptr)
        phaseDisplay.setCaption (fxme::Lfo::syncDivisionNames[juce::jlimit (0, fxme::Lfo::numSyncDivisions - 1, (int) divParam->load())]);
    else if (rateParam != nullptr)
        phaseDisplay.setCaption (juce::String (rateParam->load(), 2) + " Hz");
}

void PhaserComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), phaserTint);
}

void PhaserComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    if (compact)
    {
        // One line of switches and menus, one row of every knob.
        juce::FlexBox top, knobs, column;
        top.flexDirection    = juce::FlexBox::Direction::row;
        knobs.flexDirection  = juce::FlexBox::Direction::row;
        column.flexDirection = juce::FlexBox::Direction::column;
        top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 0.f)));
        top.items.add (fi (shapeLabel).withWidth (44.0f));
        top.items.add (fi (shapeBox)  .withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 4.f)));
        top.items.add (fi (stagesLabel).withWidth (46.0f));
        top.items.add (fi (stagesBox)  .withFlex (0.7f).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 4.f)));
        top.items.add (fi (divLabel)  .withWidth (30.0f));
        top.items.add (fi (divBox)    .withFlex (0.9f).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 4.f)));
        top.items.add (fi (syncButton).withWidth (66.0f));
        knobs.items.add (fi (phaseDisplay).withFlex (1.6f).withMargin (juce::FlexItem::Margin (2.f, 6.f, 2.f, 0.f)));
        knobs.items.add (fi (rateSlider).withFlex (1.0f));
        knobs.items.add (fi (depthSlider).withFlex (1.0f));
        knobs.items.add (fi (freqSlider).withFlex (1.0f));
        knobs.items.add (fi (feedbackSlider).withFlex (1.0f));
        knobs.items.add (fi (widthSlider).withFlex (1.0f));
        knobs.items.add (fi (mixSlider).withFlex (1.0f));
        knobs.items.add (fi (outSlider).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 6.f, 0.f, 6.f)));
        column.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        column.items.add (fi (knobs).withFlex (1.0f));
        column.performLayout (area);
        return;
    }

    juce::FlexBox top, comboRow, knobRow1, knobRow2, knobs, body, fMain;
    knobs   .flexDirection = juce::FlexBox::Direction::column;
    body    .flexDirection = juce::FlexBox::Direction::row;
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

    // Two rows of three knobs, the output fader on their right.
    knobRow1.items.add (fi (rateSlider)    .withFlex (1.0f));
    knobRow1.items.add (fi (depthSlider)   .withFlex (1.0f));
    knobRow1.items.add (fi (freqSlider).withFlex (1.0f));
    knobRow2.items.add (fi (feedbackSlider).withFlex (1.0f));
    knobRow2.items.add (fi (widthSlider)   .withFlex (1.0f));
    knobRow2.items.add (fi (mixSlider)     .withFlex (1.0f));
    knobs.items.add (fi (knobRow1).withFlex (1.0f));
    knobs.items.add (fi (knobRow2).withFlex (1.0f));
    body.items.add (fi (knobs).withFlex (1.0f));
    body.items.add (fi (outSlider).withWidth (64.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 8.f)));

    fMain.items.add (fi (top)     .withHeight (fxmefx::kHeaderRowHeight)
                                  .withMinHeight (fxmefx::kHeaderRowHeight)
                                  .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (comboRow).withHeight (26.0f).withMinHeight (24.0f)
                                  .withMargin (juce::FlexItem::Margin (0.f, 4.f, 6.f, 4.f)));
    fMain.items.add (fi (phaseDisplay).withFlex (0.55f).withMinHeight (40.0f)
                                      .withMargin (juce::FlexItem::Margin (0.f, 4.f, 6.f, 4.f)));
    fMain.items.add (fi (body).withFlex (2.0f));

    fMain.performLayout (area);
}
