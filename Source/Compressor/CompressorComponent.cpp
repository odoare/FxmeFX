/*
  ==============================================================================

    CompressorComponent.cpp

  ==============================================================================
*/

#include "CompressorComponent.h"
#include "../Common/Theme.h"
#include "../Common/Controls.h"
#include "../Common/TopBar.h"

void CompressorComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

CompressorComponent::CompressorComponent (Compressor& comp, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle, bool compactLayout, bool hasSidechain)
    : compressor (comp), apvts (state), compact (compactLayout),
      onButton (state, prefix + "_Comp_On", "On", fxmefx::theme::compressor)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (fxmefx::theme::compressor);

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel(&fxmeLookAndFeel);

    addChildComponent (titleLabel);
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Compressor", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    fxmefx::setupGainFader (*this, preGainSlider, "Pre Gain", fxmefx::theme::compressor, fxmeLookAndFeel);
    setupSlider (attackSlider, attackLabel, "Attack (ms)", 0.1, 100.0, 10.0);
    setupSlider (releaseSlider, releaseLabel, "Release (ms)", 10.0, 1000.0, 100.0);
    setupSlider (threshSlider, threshLabel, "Thresh (dB)", -60.0, 0.0, 0.0);
    setupSlider (ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, 1.0);
    setupSlider (kneeSlider, kneeLabel, "Knee (dB)", 0.0, 24.0, 0.0);
    setupSlider (peakRmsSlider, peakRmsLabel, "Peak / RMS", 0.0, 1.0, 0.0);
    setupSlider (scHighPassSlider, scHighPassLabel, "SC HP", Compressor::kScHighPassOffHz, 2000.0, Compressor::kScHighPassOffHz);
    setupSlider (scLowPassSlider, scLowPassLabel, "SC LP", 500.0, Compressor::kScLowPassOffHz, Compressor::kScLowPassOffHz);
    scHighPassSlider.setTooltip ("Sidechain high-pass. \n On the detector only: the lows stop triggering the compressor, so "
                                 "a bass or a kick is not pumped by its own weight. Off at 20 Hz.");
    scLowPassSlider.setTooltip ("Sidechain low-pass. \n On the detector only: the highs stop triggering the compressor. Off at 20 kHz.");
    scHighPassSlider.setTextValueSuffix (" Hz");
    scLowPassSlider.setTextValueSuffix (" Hz");
    fxmefx::setupGainFader (*this, gainSlider, "Gain", fxmefx::theme::compressor, fxmeLookAndFeel);

    if (hasSidechain)
    {
        extButton = std::make_unique<fxme::FxmeButton> (state, prefix + "_Comp_ExtSc", "Ext SC", fxmefx::theme::compressor);
        addAndMakeVisible (*extButton);
        extButton->setLookAndFeel (&fxmeLookAndFeel);
        extButton->button.setTooltip ("External side-chain. \n The compressor reacts to the plugin's side-chain input instead of the "
                                      "signal it compresses (ducking under a kick or a voice). Route a track to the side-chain input "
                                      "in your host. Without one, it keeps listening to its input.");
    }

    addAndMakeVisible (relModeBox);
    relModeBox.setLookAndFeel (&fxmeLookAndFeel);
    relModeBox.setColour (juce::ComboBox::outlineColourId, fxmefx::theme::compressor.darker());
    relModeBox.setColour (juce::ComboBox::arrowColourId,   fxmefx::theme::compressor.brighter (0.3f));
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
    scHighPassSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_ScHighPass", scHighPassSlider));
    scLowPassSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Comp_ScLowPass", scLowPassSlider));
    scHighPassSlider.setLogarithmic();
    scLowPassSlider.setLogarithmic();
    scHighPassSlider.setNumDecimalPlacesToDisplay (0);
    scLowPassSlider.setNumDecimalPlacesToDisplay (0);

    addAndMakeVisible (grMeter);
    grMeter.setMeterColour (fxmefx::theme::gainReduction);
    grMeter.setRange (-30.0f, 0.0f);
    grMeter.setTooltip ("Gain reduction, in dB. The number above holds the deepest reduction for a moment.");
    startTimerHz (30);
}

CompressorComponent::~CompressorComponent()
{
}

void CompressorComponent::setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = fxmefx::theme::compressor;

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


void CompressorComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), fxmefx::theme::compressor);
}

void CompressorComponent::resized()
{
    auto area = getLocalBounds().reduced (5.f);
    using fi = juce::FlexItem;

    if (compact)
    {
        // The switch and the release mode on one line; the input bar, the
        // eight knobs, the reduction meter and the output bar on one row.
        juce::FlexBox top, row, column;
        top.flexDirection    = juce::FlexBox::Direction::row;
        row.flexDirection    = juce::FlexBox::Direction::row;
        column.flexDirection = juce::FlexBox::Direction::column;
        top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 0.f)));
        top.items.add (fi().withFlex (1.0f));
        if (extButton != nullptr)
            top.items.add (fi (*extButton).withWidth (76.0f));
        top.items.add (fi (relModeBox).withWidth (140.0f));

        row.items.add (fi (preGainSlider).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 6.f, 0.f, 0.f)));
        for (auto* s : { &attackSlider, &releaseSlider, &kneeSlider, &threshSlider, &ratioSlider, &peakRmsSlider,
                         &scHighPassSlider, &scLowPassSlider })
            row.items.add (fi (*s).withFlex (1.0f));
        row.items.add (fi (grMeter).withWidth (38.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));
        row.items.add (fi (gainSlider).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 6.f, 0.f, 0.f)));

        column.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        column.items.add (fi (row).withFlex (1.0f));
        column.performLayout (area);
        return;
    }
    juce::FlexBox f1, f2, f3, f4, f5, fMain;
    f1.flexDirection = juce::FlexBox::Direction::row;
    f2.flexDirection = juce::FlexBox::Direction::row;
    f3.flexDirection = juce::FlexBox::Direction::row;
    f4.flexDirection = juce::FlexBox::Direction::column;
    f5.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    f1.items.add(fi(onButton).withWidth(fxmefx::kOnButtonWidth));
    f1.items.add(fi(titleLabel).withFlex(1.0f));
    if (extButton != nullptr)
        f1.items.add(fi(*extButton).withWidth(76.0f));
    f1.items.add(fi(relModeBox).withFlex(0.7f).withMargin(juce::FlexItem::Margin(0.f, 4.f, 0.f, 4.f)));
    f2.items.add(fi(attackSlider).withFlex(1.f));
    f2.items.add(fi(releaseSlider).withFlex(1.f));
    f2.items.add(fi(kneeSlider).withFlex(1.f));
    f2.items.add(fi(scHighPassSlider).withFlex(1.f));
    f3.items.add(fi(threshSlider).withFlex(1.f));
    f3.items.add(fi(ratioSlider).withFlex(1.f));
    f3.items.add(fi(peakRmsSlider).withFlex(1.f));
    f3.items.add(fi(scLowPassSlider).withFlex(1.f));
    f4.items.add(fi(f2).withFlex(1.f));
    f4.items.add(fi(f3).withFlex(1.f));
    f5.items.add(fi(preGainSlider).withFlex(0.15f));
    f5.items.add(fi(f4).withFlex(1.f));
    f5.items.add(fi(grMeter).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 0)));
    f5.items.add(fi(gainSlider).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f, fxmefx::kEdgeMargin, 0.f, 0.f)));

    fMain.items.add(fi(f1).withHeight(fxmefx::kHeaderRowHeight)
                          .withMinHeight(fxmefx::kHeaderRowHeight)
                          .withMargin(juce::FlexItem::Margin(5.f, 0.f, 10.f, 0)));
    fMain.items.add(fi(f5).withFlex(1.f));

    fMain.performLayout(area);
}

void CompressorComponent::timerCallback()
{
    grMeter.setActive (compressor.isOn());
    grMeter.setValue (compressor.getGrMeter().getPeak());
}