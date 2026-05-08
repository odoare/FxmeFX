/*
  ==============================================================================

    StereoDelayComponent.cpp

  ==============================================================================
*/

#include "StereoDelayComponent.h"

void StereoDelayComponent::setSliderColours(juce::Slider& s, juce::Colour c)
{
    s.setColour(juce::Slider::trackColourId, c);
    s.setColour(juce::Slider::thumbColourId, c);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, c.darker(1.0f));
}

void StereoDelayComponent::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    juce::Colour color = juce::Colours::green;

    juce::ignoreUnused (label);

    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setTooltip(text);
    slider.setName(text);
    slider.getProperties().set("showLabel", true);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void StereoDelayComponent::setupBarSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    juce::Colour color = juce::Colours::green;

    addAndMakeVisible(label);
    label.setText(text, juce::NotificationType::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setTooltip(text);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

StereoDelayComponent::StereoDelayComponent(StereoDelay& d, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : delay(d), apvts(state)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Stereo Delay", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(16.0f, juce::Font::bold));

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::green);
    onButton.setLookAndFeel(&fxmeLookAndFeel);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, prefix + "_Del_On", onButton);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    startTimer(200);

    setupSlider(delayLSlider, delayLLabel, "Delay L");
    delayLSlider.setTextValueSuffix(" bt");
    delayLSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_DelayL", delayLSlider));

    setupSlider(delayRSlider, delayRLabel, "Delay R");
    delayRSlider.setTextValueSuffix(" bt");
    delayRSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_DelayR", delayRSlider));

    setupSlider(fdbkLSlider, fdbkLLabel, "Fdbk L");
    fdbkLSlider.setTextValueSuffix(" dB");
    fdbkLSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_FdbkL", fdbkLSlider));

    setupSlider(fdbkRSlider, fdbkRLabel, "Fdbk R");
    fdbkRSlider.setTextValueSuffix(" dB");
    fdbkRSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_FdbkR", fdbkRSlider));

    setupSlider(crossFdbkSlider, crossFdbkLabel, "Cross");
    crossFdbkSlider.setTextValueSuffix(" dB");
    crossFdbkSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_CrossFdbk", crossFdbkSlider));

    setupSlider(cutoffSlider, cutoffLabel, "Cutoff");
    cutoffSlider.setTextValueSuffix(" Hz");
    cutoffSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_FilterCutoff", cutoffSlider));

    setupSlider(qSlider, qLabel, "Q");
    qSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_FilterQ", qSlider));

    setupBarSlider(dryGainSlider, dryGainLabel, "Dry");
    dryGainSlider.setTextValueSuffix(" dB");
    dryGainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_DryGain", dryGainSlider));

    setupBarSlider(wetGainSlider, wetGainLabel, "Wet");
    wetGainSlider.setTextValueSuffix(" dB");
    wetGainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_WetGain", wetGainSlider));
}

StereoDelayComponent::~StereoDelayComponent()
{
    stopTimer();
}

void StereoDelayComponent::paint(juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin(juce::degreesToRadians(270.0f)) / length;
    auto height = float(getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colour::fromFloatRGBA(0.15f, 0.25f, 0.15f, 1.0f);
    juce::ColourGradient grad(bluegreengrey.darker().darker().darker(), perpendicular * height,
        bluegreengrey, perpendicular * -height, false);
    g.setGradientFill(grad);
    g.fillAll();
}

void StereoDelayComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    using fi = juce::FlexItem;
    juce::FlexBox fMain, fTop, fSliders1, fSliders2;
    fMain.flexDirection = juce::FlexBox::Direction::column;
    fTop.flexDirection = juce::FlexBox::Direction::row;
    fSliders1.flexDirection = juce::FlexBox::Direction::row;
    fSliders2.flexDirection = juce::FlexBox::Direction::row;

    fTop.items.add(fi(onButton).withFlex(0.15f));
    fTop.items.add(fi(titleLabel).withFlex(1.f));
    fTop.items.add(fi(bpmLabel).withFlex(0.3f));

    auto barSliderBox = [](juce::FlexBox& box, juce::Label& label, juce::Slider& slider)
    {
        box.flexDirection = juce::FlexBox::Direction::column;
        box.items.add(fi(label).withFlex(0.2f));
        box.items.add(fi(slider).withFlex(0.8f));
    };

    juce::FlexBox b8, b9;
    barSliderBox(b8, dryGainLabel, dryGainSlider);
    barSliderBox(b9, wetGainLabel, wetGainSlider);

    fSliders1.items.add(fi(delayLSlider).withFlex(1.f));
    fSliders1.items.add(fi(delayRSlider).withFlex(1.f));
    fSliders1.items.add(fi(fdbkLSlider).withFlex(1.f));
    fSliders1.items.add(fi(fdbkRSlider).withFlex(1.f));
    fSliders2.items.add(fi(crossFdbkSlider).withFlex(1.f));
    fSliders2.items.add(fi(cutoffSlider).withFlex(1.f));
    fSliders2.items.add(fi(qSlider).withFlex(1.f));
    fSliders2.items.add(fi(b8).withFlex(0.25f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 5.f)));
    fSliders2.items.add(fi(b9).withFlex(0.25f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 5.f)));

    fMain.items.add(fi(fTop).withFlex(0.28f));
    fMain.items.add(fi(fSliders1).withFlex(0.85f));
    fMain.items.add(fi(fSliders2).withFlex(0.85f));
    
    fMain.performLayout(area);
}

void StereoDelayComponent::timerCallback()
{
    bpmLabel.setText(juce::String(delay.getBPM(), 1) + " BPM", juce::NotificationType::dontSendNotification);
}