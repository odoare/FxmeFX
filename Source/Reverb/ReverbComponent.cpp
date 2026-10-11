/*
  ==============================================================================

    ReverbComponent.cpp

  ==============================================================================
*/

#include "ReverbComponent.h"
#include "../Common/TopBar.h"

void ReverbComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
                                   const juce::String& suffix, const juce::String& tooltip)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setTextValueSuffix (suffix);
    slider.setTooltip (tooltip);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    slider.setColour (juce::Slider::trackColourId, tint().darker());
    slider.setColour (juce::Slider::thumbColourId, tint());
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, tint().darker (2.0f));
}

ReverbComponent::ReverbComponent (ReverbEffect& r, juce::AudioProcessorValueTreeState& state,
                                  const juce::String& prefix, bool showTitle, bool compactLayout)
    : reverb (r), apvts (state), compact (compactLayout),
      onButton (state, prefix + "_Verb_On", "On", tint())
{
    juce::ignoreUnused (reverb);
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Reverb", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

    setupRotary (preDelaySlider, "Pre-delay", " ms",
                 "Pre-delay. \n Time before the tail starts, so it separates from the dry attack.");
    setupRotary (sizeSlider, "Size", " %",
                 "Size. \n How long the tail lasts: a small room to a large hall.");
    setupRotary (dampSlider, "Damping", " %",
                 "Damping. \n How fast the high end of the tail dies away: 0 keeps it bright.");
    setupRotary (widthSlider, "Width", " %",
                 "Width. \n Stereo spread of the tail: 0 is mono.");
    setupRotary (lowCutSlider, "Low cut", " Hz",
                 "Low cut. \n High-pass on the reverb only, to keep the tail out of the bass.");
    setupRotary (highCutSlider, "High cut", " Hz",
                 "High cut. \n Low-pass on the reverb only, to take the fizz off the tail.");
    setupRotary (mixSlider, "Mix", " %",
                 "Mix. \n Balance between the dry signal and the reverb.");
    setupRotary (outSlider, "Output", " dB", "Output level.");
    outSlider.setCentralValue (0.0);

    preDelaySlider.setAttachment (new SliderAttachment (apvts, prefix + "_Verb_PreDelay", preDelaySlider));
    sizeSlider    .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_Size",     sizeSlider));
    dampSlider    .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_Damp",     dampSlider));
    widthSlider   .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_Width",    widthSlider));
    lowCutSlider  .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_LowCut",   lowCutSlider));
    highCutSlider .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_HighCut",  highCutSlider));
    mixSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_Mix",      mixSlider));
    outSlider     .setAttachment (new SliderAttachment (apvts, prefix + "_Verb_Out",      outSlider));
}

ReverbComponent::~ReverbComponent() = default;

void ReverbComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void ReverbComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, row1, row2, fMain;
    top.flexDirection  = juce::FlexBox::Direction::row;
    row1.flexDirection = juce::FlexBox::Direction::row;
    row2.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    fxme::FxmeSlider* first[] = { &preDelaySlider, &sizeSlider, &dampSlider, &widthSlider };
    fxme::FxmeSlider* second[] = { &lowCutSlider, &highCutSlider, &mixSlider, &outSlider };
    for (auto* s : first)
        row1.items.add (fi (*s).withFlex (1.0f));
    for (auto* s : second)
        (compact ? row1 : row2).items.add (fi (*s).withFlex (1.0f));

    if (compact)
    {
        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        fMain.items.add (fi (row1).withFlex (1.0f));
    }
    else
    {
        fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                                 .withMinHeight (fxmefx::kHeaderRowHeight)
                                 .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
        fMain.items.add (fi (row1).withFlex (1.0f));
        fMain.items.add (fi (row2).withFlex (1.0f));
    }
    fMain.performLayout (area);
}
