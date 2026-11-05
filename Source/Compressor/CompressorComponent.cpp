/*
  ==============================================================================

    CompressorComponent.cpp

  ==============================================================================
*/

#include "CompressorComponent.h"
#include "../Common/Theme.h"
#include "../Common/TopBar.h"

void CompressorComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

CompressorComponent::CompressorComponent (Compressor& comp, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle, bool compactLayout)
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

    setupFader (preGainSlider, "Pre Gain", -24.0, 24.0, 0.0);
    setupSlider (attackSlider, attackLabel, "Attack (ms)", 0.1, 100.0, 10.0);
    setupSlider (releaseSlider, releaseLabel, "Release (ms)", 10.0, 1000.0, 100.0);
    setupSlider (threshSlider, threshLabel, "Thresh (dB)", -60.0, 0.0, 0.0);
    setupSlider (ratioSlider, ratioLabel, "Ratio", 1.0, 20.0, 1.0);
    setupSlider (kneeSlider, kneeLabel, "Knee (dB)", 0.0, 24.0, 0.0);
    setupSlider (peakRmsSlider, peakRmsLabel, "Peak / RMS", 0.0, 1.0, 0.0);
    setupFader (gainSlider, "Gain", -24.0, 24.0, 0.0);

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

/** A console fader for a ±dB gain: a scale beside its slot, the value under it. */
void CompressorComponent::setupFader (fxme::FxmeSlider& slider, const juce::String& text, double min, double max, double def)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.getProperties().set ("linearStyle", "console");
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setCentralValue (0.0);   // bipolar ±dB: the fill grows from 0 dB
    slider.setNumDecimalPlacesToDisplay (1);
    slider.setTextValueSuffix (" dB");
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, fxmefx::theme::compressor);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff2b2b2b));   // the cap's body
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
        // The switch and the release mode on one line; the input bar, the six
        // knobs, the reduction meter and the output bar on one row.
        juce::FlexBox top, row, column;
        top.flexDirection    = juce::FlexBox::Direction::row;
        row.flexDirection    = juce::FlexBox::Direction::row;
        column.flexDirection = juce::FlexBox::Direction::column;
        top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 0.f)));
        top.items.add (fi().withFlex (1.0f));
        top.items.add (fi (relModeBox).withWidth (140.0f));

        row.items.add (fi (preGainSlider).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 6.f, 0.f, 0.f)));
        for (auto* s : { &attackSlider, &releaseSlider, &kneeSlider, &threshSlider, &ratioSlider, &peakRmsSlider })
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
    f5.items.add(fi(gainSlider).withFlex(0.15f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 0.f)));   // off the right edge

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