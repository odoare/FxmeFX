/*
  ==============================================================================

    TapeComponent.cpp

  ==============================================================================
*/

#include "TapeComponent.h"
#include "../Common/TopBar.h"
#include "../Common/Controls.h"

//==============================================================================
// HysteresisLoopView

HysteresisLoopView::HysteresisLoopView (fxme::WaveformTap& field, fxme::WaveformTap& magnetisation)
    : fieldTap (field), magTap (magnetisation)
{
    h.assign ((size_t) numPoints, 0.0f);
    m.assign ((size_t) numPoints, 0.0f);
    startTimerHz (30);
}

HysteresisLoopView::~HysteresisLoopView()
{
    stopTimer();
}

void HysteresisLoopView::timerCallback()
{
    // The two taps are pushed one after the other in each block: take both
    // snapshots between the same two pushes, or try again.
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        const auto before = fieldTap.getTotalPushed();
        fieldTap.snapshot (h.data(), numPoints);
        magTap.snapshot (m.data(), numPoints);
        if (fieldTap.getTotalPushed() == before && magTap.getTotalPushed() == before)
            break;
    }

    float extent = 0.0f;
    for (float v : h)
        extent = juce::jmax (extent, std::abs (v));
    // Grows at once, shrinks slowly, so the view does not pump.
    const float target = juce::jmax (0.5f, extent * 1.1f);
    hRange = target > hRange ? target : hRange + 0.1f * (target - hRange);
    repaint();
}

void HysteresisLoopView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0e0d14));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colour (0xff3a3a4c));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto plot = r.reduced (8.0f);
    const float cx = plot.getCentreX(), cy = plot.getCentreY();
    g.setColour (juce::Colour (0x66555555));
    g.drawHorizontalLine ((int) cy, plot.getX(), plot.getRight());
    g.drawVerticalLine ((int) cx, plot.getY(), plot.getBottom());
    for (float level : { 1.0f, -1.0f })   // saturation
        g.drawHorizontalLine ((int) (cy - level / 1.1f * plot.getHeight() * 0.5f), plot.getX(), plot.getRight());

    g.setColour (juce::Colour (0xff9a9aa8));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("H", plot.removeFromRight (12.0f).withTrimmedTop (plot.getHeight() * 0.5f).withHeight (12.0f),
                juce::Justification::centred, false);
    g.drawText ("M", juce::Rectangle<float> (cx + 3.0f, r.getY() + 3.0f, 12.0f, 12.0f), juce::Justification::centredLeft, false);

    plot = r.reduced (8.0f);
    const float sx = plot.getWidth() * 0.5f / hRange, sy = plot.getHeight() * 0.5f / 1.1f;
    juce::Path loop;
    for (int i = 0; i < numPoints; ++i)
    {
        const float x = cx + h[(size_t) i] * sx;
        const float y = cy - juce::jlimit (-1.2f, 1.2f, m[(size_t) i]) * sy;
        if (i == 0) loop.startNewSubPath (x, y);
        else        loop.lineTo (x, y);
    }
    auto c = active ? accent : accent.withMultipliedSaturation (0.2f).withMultipliedAlpha (0.5f);
    g.setColour (c);
    g.strokePath (loop, juce::PathStrokeType (1.4f));
}

//==============================================================================
// TapeComponent

void TapeComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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

void TapeComponent::setupCombo (juce::ComboBox& box, const juce::StringArray& items, const juce::String& paramId,
                                const juce::String& tooltip)
{
    addAndMakeVisible (box);
    box.setLookAndFeel (&fxmeLookAndFeel);
    box.setColour (juce::ComboBox::outlineColourId, tint().darker());
    box.setColour (juce::ComboBox::arrowColourId,   tint().brighter (0.3f));
    box.addItemList (items, 1);
    box.setTooltip (tooltip);
    comboAtts.push_back (std::make_unique<ComboBoxAttachment> (apvts, paramId, box));
}

TapeComponent::TapeComponent (Tape& t, juce::AudioProcessorValueTreeState& state,
                              const juce::String& prefix, bool showTitle, bool compactLayout)
    : tape (t), apvts (state), compact (compactLayout),
      onButton   (state, prefix + "_Tape_On",   "On",        tint()),
      autoButton (state, prefix + "_Tape_Auto", "Auto gain", tint()),
      loopView (t.getFieldTap(), t.getMagnetisationTap())
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Tape", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    for (auto* b : { &onButton, &autoButton })
    {
        addAndMakeVisible (*b);
        b->setLookAndFeel (&fxmeLookAndFeel);
    }
    autoButton.button.setTooltip ("Auto gain. \n Turns the output down as Drive turns the input up, "
                                  "so Drive changes the colour rather than the level.");

    setupCombo (typeBox, { "Modern", "Vintage", "High output" }, prefix + "_Tape_Type",
                "Tape type. \n Modern: the reference. Vintage: older ferric tape, saturates 4 dB sooner, "
                "wider loop, more hiss. High output: 4 dB more headroom, tighter, quieter.");
    setupCombo (speedBox, { "7.5 ips", "15 ips", "30 ips" }, prefix + "_Tape_Speed",
                "Speed. \n Slower: a lower, bigger head bump, duller top, more emphasis. "
                "Faster: a higher, smaller bump and an open top.");
    setupCombo (eqBox, { "NAB", "IEC" }, prefix + "_Tape_EQ",
                "EQ standard. \n NAB (American): also boosts the deep lows into the tape. "
                "IEC (European): highs only. At 30 ips both are AES.");
    setupCombo (osBox, { "2x", "4x" }, prefix + "_Tape_OS",
                "Oversampling. \n 4x is cleaner when driven hard, at twice the CPU.");

    setupRotary (driveSlider, "Drive", " dB",
                 "Drive. \n The level into the record head. 0 dB puts a -18 dBFS signal at 0 VU.");
    setupRotary (satSlider, "Saturation", " %",
                 "Saturation. \n How hard the tape is hit, at a constant output level.");
    setupRotary (biasSlider, "Bias", " %",
                 "Bias. \n 50 % is calibrated. Less: brighter, gritty, distorted even when quiet "
                 "(under-biased). More: cleaner and darker.");
    setupRotary (hissSlider, "Hiss", "",
                 "Hiss. \n Tape noise, its level in dBFS. Off at the bottom.");

    if (compact)
    {
        setupRotary (mixSlider, "Mix", " %", "Mix. \n Balance between the dry signal and the tape.");
        setupRotary (outSlider, "Output", " dB", "Output level.");
    }
    else
    {
        fxmefx::setupFader (*this, mixSlider, "Mix", tint(), fxmeLookAndFeel);
        mixSlider.setTextValueSuffix (" %");
        mixSlider.setNumDecimalPlacesToDisplay (0);
        mixSlider.setTooltip ("Mix. \n Balance between the dry signal and the tape.");
        fxmefx::setupGainFader (*this, outSlider, "Output", tint(), fxmeLookAndFeel);
        outSlider.setTooltip ("Output level.");
    }
    outSlider.setCentralValue (0.0);
    driveSlider.setCentralValue (0.0);
    biasSlider.setCentralValue (50.0);

    auto attach = [&] (fxme::FxmeSlider& s, const char* id)
    {
        s.setAttachment (new SliderAttachment (apvts, prefix + "_Tape_" + id, s));
    };
    attach (driveSlider, "Drive");
    attach (satSlider,   "Sat");
    attach (biasSlider,  "Bias");
    attach (hissSlider,  "Hiss");
    attach (mixSlider,   "Mix");
    attach (outSlider,   "Out");
    for (auto* s : { &satSlider, &biasSlider })
        s->setNumDecimalPlacesToDisplay (0);

    speedParam = apvts.getRawParameterValue (prefix + "_Tape_Speed");

    if (! compact)
    {
        tape.getFieldTap()       .setEnabled (true);
        tape.getMagnetisationTap().setEnabled (true);
        loopView.accent = tint();
        addAndMakeVisible (loopView);

        for (auto* vu : { &inVu, &outVu })
        {
            vu->setMeterColor (tint());
            vu->setRange (-48.0f, 0.0f);
            vu->setZeroLevel (kVuZeroDbfs);
            addAndMakeVisible (*vu);
        }
        for (auto* l : { &inLabel, &outLabel })
        {
            // The column is a meter's width: no border, and squeeze rather
            // than truncate.
            l->setJustificationType (juce::Justification::centred);
            l->setFont (juce::FontOptions (11.0f));
            l->setBorderSize ({});
            l->setMinimumHorizontalScale (0.7f);
            addAndMakeVisible (*l);
        }
        inLabel.setText ("In", juce::dontSendNotification);
        outLabel.setText ("Out", juce::dontSendNotification);
    }

    timerCallback();
    startTimerHz (30);
}

TapeComponent::~TapeComponent()
{
    stopTimer();
    if (! compact)
    {
        tape.getFieldTap()       .setEnabled (false);
        tape.getMagnetisationTap().setEnabled (false);
    }
}

void TapeComponent::timerCallback()
{
    const bool at30 = speedParam != nullptr && juce::roundToInt (speedParam->load()) == 2;
    if (eqBox.isEnabled() == at30)
        eqBox.setEnabled (! at30);

    if (! compact)
    {
        inVu .setValue (tape.getInputMeter() .getRMS());
        outVu.setValue (tape.getOutputMeter().getRMS());
        loopView.active = tape.isOn();
    }
}

void TapeComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void TapeComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, knobs, left, meters, inCol, outCol, body, fMain;
    top.flexDirection    = juce::FlexBox::Direction::row;
    knobs.flexDirection  = juce::FlexBox::Direction::row;
    left.flexDirection   = juce::FlexBox::Direction::column;
    meters.flexDirection = juce::FlexBox::Direction::row;
    inCol.flexDirection  = juce::FlexBox::Direction::column;
    outCol.flexDirection = juce::FlexBox::Direction::column;
    body.flexDirection   = juce::FlexBox::Direction::row;
    fMain.flexDirection  = juce::FlexBox::Direction::column;

    const auto boxMargin = juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f);
    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));
    top.items.add (fi (typeBox).withWidth (110.0f).withMargin (boxMargin));
    top.items.add (fi (speedBox).withWidth (84.0f).withMargin (boxMargin));
    top.items.add (fi (eqBox).withWidth (70.0f).withMargin (boxMargin));
    top.items.add (fi (osBox).withWidth (60.0f).withMargin (boxMargin));
    top.items.add (fi (autoButton).withWidth (96.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 8.f)));

    for (auto* s : { &driveSlider, &satSlider, &biasSlider, &hissSlider })
        knobs.items.add (fi (*s).withFlex (1.0f));

    if (compact)
    {
        knobs.items.add (fi (mixSlider).withFlex (1.0f));
        knobs.items.add (fi (outSlider).withFlex (1.0f));
        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        fMain.items.add (fi (knobs).withFlex (1.0f));
        fMain.performLayout (area);
        return;
    }

    left.items.add (fi (knobs).withHeight (kKnobRowHeight));
    left.items.add (fi (loopView).withFlex (1.0f).withMargin (juce::FlexItem::Margin (6.f, 0.f, 0.f, 0.f)));

    inCol.items.add  (fi (inVu).withFlex (1.0f));
    inCol.items.add  (fi (inLabel).withHeight (16.0f));
    outCol.items.add (fi (outVu).withFlex (1.0f));
    outCol.items.add (fi (outLabel).withHeight (16.0f));
    meters.items.add (fi (inCol).withWidth (21.0f).withMargin (juce::FlexItem::Margin (0.f, 2.f, 0.f, 0.f)));
    meters.items.add (fi (outCol).withWidth (21.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 2.f)));

    body.items.add (fi (left).withFlex (1.0f));
    body.items.add (fi (meters).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 10.f)));
    body.items.add (fi (mixSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 5.f)));
    body.items.add (fi (outSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 5.f)));

    fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                             .withMinHeight (fxmefx::kHeaderRowHeight)
                             .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (body).withFlex (1.0f));
    fMain.performLayout (area);
}
