/*
  ==============================================================================

    StereoDelayComponent.cpp

  ==============================================================================
*/

#include "StereoDelayComponent.h"
#include "../Common/TopBar.h"

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
    slider.setTooltip(text);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void StereoDelayComponent::setupCombo (juce::ComboBox& box, juce::Label& label, const juce::String& text)
{
    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centredRight);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));

    addAndMakeVisible (box);
    box.setLookAndFeel (&fxmeLookAndFeel);
    box.addItemList (StereoDelay::delayModeChoices(), 1);
    box.setTooltip ("How this side reads its delay value. \n Seconds: the value is "
                    "the delay in seconds. \n DAW sync: a proportion of a whole note, "
                    "so 0.25 is a quarter note. \n MIDI note: a multiple of the period "
                    "of the last note played, so 1 tunes the line to it.");
    box.setColour (juce::ComboBox::outlineColourId, juce::Colours::green.darker());
    box.setColour (juce::ComboBox::arrowColourId,   juce::Colours::green.brighter (0.3f));
}

juce::String StereoDelayComponent::resolvedTimeText() const
{
    const auto ms = [this] (bool right)
    {
        return juce::String (delay.resolvedSeconds (right) * 1000.0f, 1) + " ms";
    };

    juce::String text = "L " + ms (false) + "    R " + ms (true);

    // The note mode has nothing to lock to until something has been played, and
    // silently reading the value as seconds would look like a bug.
    const auto usesNote = [this] (const juce::ComboBox& box)
    {
        return box.getSelectedItemIndex() == (int) fxme::DelayTimeMode::notePeriod;
    };
    if ((usesNote (modeLBox) || usesNote (modeRBox)) && delay.getLastNoteHz() <= 0.0f)
        text += "    (no MIDI note yet)";

    return text;
}

StereoDelayComponent::StereoDelayComponent(StereoDelay& d, juce::AudioProcessorValueTreeState& state, const juce::String& prefix, bool showTitle)
    : delay(d), apvts(state),
      onButton (state, prefix + "_Del_On", "On", juce::Colours::green)
{
    // Tints this component's drop-down menus and tooltips; both are their own
    // windows and cannot see the widget that opened them.
    fxmeLookAndFeel.setAccentColour (juce::Colours::green);

    addChildComponent(titleLabel);
    titleLabel.setVisible(showTitle);
    titleLabel.setText("Stereo Delay", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel(&fxmeLookAndFeel);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setJustificationType(juce::Justification::centredRight);

    setupCombo (modeLBox, modeLLabel, "Mode L");
    setupCombo (modeRBox, modeRLabel, "Mode R");
    modeLAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Del_ModeL", modeLBox);
    modeRAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Del_ModeR", modeRBox);

    addAndMakeVisible (resolvedLabel);
    resolvedLabel.setJustificationType (juce::Justification::centred);
    resolvedLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    resolvedText = resolvedTimeText();
    resolvedLabel.setText (resolvedText, juce::NotificationType::dontSendNotification);

    startTimer(100);

    // No suffix on the two delay knobs: the number is unitless now, and what it
    // means is the matching Mode's business. The read-out beside the mode boxes
    // is what says how long the delay actually is.
    setupSlider(delayLSlider, delayLLabel, "Delay L");
    delayLSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Del_DelayL", delayLSlider));

    setupSlider(delayRSlider, delayRLabel, "Delay R");
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
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), juce::Colours::green);
}

void StereoDelayComponent::resized()
{
    auto area = getLocalBounds().reduced(5);
    using fi = juce::FlexItem;
    juce::FlexBox fMain, fTop, fModes, fSliders1, fSliders2;
    fMain.flexDirection = juce::FlexBox::Direction::column;
    fTop.flexDirection = juce::FlexBox::Direction::row;
    fModes.flexDirection = juce::FlexBox::Direction::row;
    fSliders1.flexDirection = juce::FlexBox::Direction::row;
    fSliders2.flexDirection = juce::FlexBox::Direction::row;

    fTop.items.add(fi(onButton).withWidth(fxmefx::kOnButtonWidth));
    fTop.items.add(fi(titleLabel).withFlex(1.f));
    fTop.items.add(fi(bpmLabel).withFlex(0.3f));

    fModes.items.add (fi (modeLLabel).withWidth (52.0f));
    fModes.items.add (fi (modeLBox)  .withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 12.f, 0.f, 4.f)));
    fModes.items.add (fi (modeRLabel).withWidth (52.0f));
    fModes.items.add (fi (modeRBox)  .withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 12.f, 0.f, 4.f)));
    fModes.items.add (fi (resolvedLabel).withFlex (1.2f));

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

    fMain.items.add(fi(fTop).withHeight(fxmefx::kHeaderRowHeight)
                            .withMinHeight(fxmefx::kHeaderRowHeight)
                            .withMargin(juce::FlexItem::Margin(5.f, 0.f, 6.f, 0.f)));
    fMain.items.add(fi(fModes).withHeight(26.f).withMinHeight(24.f)
                              .withMargin(juce::FlexItem::Margin(0.f, 4.f, 6.f, 4.f)));
    fMain.items.add(fi(fSliders1).withFlex(0.85f));
    fMain.items.add(fi(fSliders2).withFlex(0.85f));
    
    fMain.performLayout(area);
}

void StereoDelayComponent::timerCallback()
{
    bpmLabel.setText(juce::String(delay.getBPM(), 1) + " BPM", juce::NotificationType::dontSendNotification);

    // Only repaint the read-out when it actually changes: it follows the knobs,
    // the transport tempo and the last note, so it moves on its own.
    if (const auto text = resolvedTimeText(); text != resolvedText)
    {
        resolvedText = text;
        resolvedLabel.setText (text, juce::NotificationType::dontSendNotification);
    }
}