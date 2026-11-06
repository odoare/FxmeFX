/*
  ==============================================================================

    GateComponent.cpp

  ==============================================================================
*/

#include "GateComponent.h"
#include "../Common/TopBar.h"
#include "../Common/Controls.h"

//==============================================================================
void GateComponent::StateLed::setState (int newState, bool newActive)
{
    if (newState == state && newActive == active)
        return;
    state = newState;
    active = newActive;
    repaint();
}

void GateComponent::StateLed::paint (juce::Graphics& g)
{
    // 0 closed, 1 open, 2 hold (GateStateMachine::State).
    static const juce::Colour colours[] { juce::Colour (0xffc8443c), juce::Colour (0xff4cd06a), juce::Colour (0xffe8b040) };
    static const char* const words[] { "Closed", "Open", "Hold" };
    const int s = juce::jlimit (0, 2, state);

    auto area = getLocalBounds().toFloat();
    const float d = juce::jmin (12.0f, area.getHeight() - 4.0f);
    auto led = area.removeFromLeft (d + 6.0f).withSizeKeepingCentre (d, d);

    auto c = colours[s];
    if (! active)
        c = c.withMultipliedSaturation (0.2f).withMultipliedAlpha (0.4f);
    else if (s == 0)
        c = c.withMultipliedBrightness (0.6f);

    if (active && s != 0)
    {
        g.setColour (c.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (c);
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (led, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (active ? 0.85f : 0.4f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText (words[s], area, juce::Justification::centredLeft, false);
}

//==============================================================================
void GateComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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

GateComponent::GateComponent (Gate& g, juce::AudioProcessorValueTreeState& state,
                              const juce::String& prefix, bool showTitle, bool compactLayout)
    : gate (g), apvts (state), threshId (prefix + "_Gate_Thresh"), compact (compactLayout),
      onButton (state, prefix + "_Gate_On", "On", tint()),
      listenButton (state, prefix + "_Gate_Listen", "Listen", tint())
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Gate", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    addAndMakeVisible (listenButton);
    listenButton.setLookAndFeel (&fxmeLookAndFeel);
    listenButton.button.setTooltip ("Listen. \n Hear the sidechain (the detector's signal, through its high-pass and low-pass) "
                             "instead of the output, to set the filters by ear.");

    addAndMakeVisible (modeBox);
    modeBox.setLookAndFeel (&fxmeLookAndFeel);
    modeBox.setColour (juce::ComboBox::outlineColourId, tint().darker());
    modeBox.setColour (juce::ComboBox::arrowColourId,   tint().brighter (0.3f));
    modeBox.addItem ("Gate",     1);
    modeBox.addItem ("Expander", 2);
    modeBox.setTooltip ("Mode. \n Gate: closed, the signal drops to the range at once. "
                        "Expander: the further under the threshold, the more it is turned down, along the ratio.");
    modeAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Gate_Mode", modeBox);

    addAndMakeVisible (stateLed);
    stateLed.setTooltip ("The gate's state: open, holding (the level fell, the hold time is running), or closed.");

    setupRotary (threshSlider, "Threshold", " dB",
                 "Threshold. \n The gate opens when the detector's level reaches it.");
    setupRotary (hystSlider, "Hysteresis", " dB",
                 "Hysteresis. \n How far under the threshold the level must fall before the gate closes, "
                 "so a decaying note does not make it chatter.");
    setupRotary (rangeSlider, "Range", " dB",
                 "Range. \n How far down the closed gate turns the signal: -80 dB is silence, "
                 "a few dB only takes the noise down a little.");
    setupRotary (ratioSlider, "Ratio", ":1",
                 "Ratio (Expander mode). \n For each dB under the threshold, the signal goes down ratio dB.");
    setupRotary (lookaheadSlider, "Look-ahead", " ms",
                 "Look-ahead. \n Delays the audio so the gate opens before a transient arrives "
                 "and the attack does not cut it. Reported to the host as latency.");
    setupRotary (attackSlider, "Attack", " ms",
                 "Attack. \n How fast the gate opens.");
    setupRotary (holdSlider, "Hold", " ms",
                 "Hold. \n How long the gate stays open after the level fell, before it starts closing.");
    setupRotary (releaseSlider, "Release", " ms",
                 "Release. \n How fast the gate closes.");
    setupRotary (highPassSlider, "SC HP", " Hz",
                 "Sidechain high-pass. \n On the detector only: keeps low rumble or bleed from opening the gate.");
    setupRotary (lowPassSlider, "SC LP", " Hz",
                 "Sidechain low-pass. \n On the detector only: keeps cymbals or hiss from opening the gate.");

    if (compact)
        setupRotary (outSlider, "Output", " dB", "Output level.");
    else
    {
        fxmefx::setupGainFader (*this, outSlider, "Output", tint(), fxmeLookAndFeel);
        outSlider.setTooltip ("Output level.");
    }
    outSlider.setCentralValue (0.0);

    auto attach = [&] (fxme::FxmeSlider& s, const char* id)
    {
        s.setAttachment (new SliderAttachment (apvts, prefix + "_Gate_" + id, s));
    };
    attach (threshSlider,    "Thresh");
    attach (hystSlider,      "Hyst");
    attach (rangeSlider,     "Range");
    attach (ratioSlider,     "Ratio");
    attach (lookaheadSlider, "Lookahead");
    attach (attackSlider,    "Attack");
    attach (holdSlider,      "Hold");
    attach (releaseSlider,   "Release");
    attach (highPassSlider,  "HighPass");
    attach (lowPassSlider,   "LowPass");
    attach (outSlider,       "Out");
    highPassSlider.setLogarithmic();
    lowPassSlider .setLogarithmic();
    for (auto* s : { &ratioSlider, &lookaheadSlider })
        s->setNumDecimalPlacesToDisplay (2);

    modeParam   = apvts.getRawParameterValue (prefix + "_Gate_Mode");
    threshParam = apvts.getRawParameterValue (threshId);

    // The meter: the detector's level against the threshold (drag it to set
    // the threshold), and the gain the gate applies.
    fxme::DynamicsMeter::Colours mc;
    mc.level     = tint();
    mc.threshold = tint().brighter (0.6f);
    mc.cut       = fxmefx::theme::gainReduction;
    meter.setColours (mc);
    meter.setLevelRange (Gate::kMinLevelDb, 0.0f);
    meter.setGainRange (-Gate::kMinLevelDb);
    meter.setLabels ("Level", "Gain");
    meter.setTooltip ("Level: the detector's level (after the sidechain filters), with the threshold across it; "
                      "drag the threshold to set it. \n Gain: how far down the gate turns the signal.");
    meter.onThresholdDragStart = [this]
    {
        if (auto* p = apvts.getParameter (threshId)) p->beginChangeGesture();
    };
    meter.onThresholdDrag = [this] (float db)
    {
        if (auto* p = apvts.getParameter (threshId))
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (Gate::kMinLevelDb, 0.0f, db)));
    };
    meter.onThresholdDragEnd = [this]
    {
        if (auto* p = apvts.getParameter (threshId)) p->endChangeGesture();
    };
    addAndMakeVisible (meter);

    timerCallback();
    startTimerHz (30);
}

GateComponent::~GateComponent()
{
    stopTimer();
}

void GateComponent::timerCallback()
{
    const bool on = gate.isOn();
    const float threshold = threshParam != nullptr ? threshParam->load() : -40.0f;
    meter.update (gate.getDisplayLevel().load(), threshold, gate.getDisplayGain().load(), on);
    stateLed.setState ((int) gate.getDisplayState().load(), on);

    const bool expander = modeParam != nullptr && modeParam->load() > 0.5f;
    if (ratioSlider.isEnabled() != expander)
        ratioSlider.setEnabled (expander);
}

void GateComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void GateComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, row1, row2, knobs, body, fMain;
    top.flexDirection   = juce::FlexBox::Direction::row;
    row1.flexDirection  = juce::FlexBox::Direction::row;
    row2.flexDirection  = juce::FlexBox::Direction::row;
    knobs.flexDirection = juce::FlexBox::Direction::column;
    body.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (stateLed).withWidth (80.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 8.f)));
    top.items.add (fi (titleLabel).withFlex (1.0f));
    top.items.add (fi (listenButton).withWidth (80.0f));
    top.items.add (fi (modeBox).withWidth (120.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

    if (compact)
    {
        for (auto* s : { &threshSlider, &hystSlider, &rangeSlider, &ratioSlider, &attackSlider,
                         &holdSlider, &releaseSlider, &highPassSlider, &lowPassSlider, &lookaheadSlider })
            row1.items.add (fi (*s).withFlex (1.0f));
        row1.items.add (fi (meter).withWidth (60.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));
        row1.items.add (fi (outSlider).withFlex (1.0f));

        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        fMain.items.add (fi (row1).withFlex (1.0f));
        fMain.performLayout (area);
        return;
    }

    // Level and gain law on the first row, time and detector on the second;
    // the meter and the output on the right.
    for (auto* s : { &threshSlider, &hystSlider, &rangeSlider, &ratioSlider, &lookaheadSlider })
        row1.items.add (fi (*s).withFlex (1.0f));
    for (auto* s : { &attackSlider, &holdSlider, &releaseSlider, &highPassSlider, &lowPassSlider })
        row2.items.add (fi (*s).withFlex (1.0f));
    knobs.items.add (fi (row1).withFlex (1.0f));
    knobs.items.add (fi (row2).withFlex (1.0f));

    body.items.add (fi (knobs).withFlex (1.0f));
    body.items.add (fi (meter).withWidth (76.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 10.f)));
    body.items.add (fi (outSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 5.f)));

    fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                             .withMinHeight (fxmefx::kHeaderRowHeight)
                             .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (body).withFlex (1.0f));
    fMain.performLayout (area);
}
