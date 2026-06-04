/*
  ==============================================================================

    CabComponent.cpp

  ==============================================================================
*/

#include "CabComponent.h"

namespace
{
    const auto cabTint = juce::Colours::orange;
}

void CabIRPlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    auto draw = [&] (juce::Path& path, const juce::AudioBuffer<float>& ir,
                     juce::Colour colour, juce::Rectangle<float> area)
    {
        g.setColour (colour.withAlpha (0.25f));
        g.fillRect (area);

        path.clear();
        const int n = ir.getNumSamples();
        if (n <= 0 || area.getWidth() <= 0.0f) return;

        const auto* data = ir.getReadPointer (0);
        float peak = 1.0e-9f;
        for (int i = 0; i < n; ++i)
            peak = juce::jmax (peak, std::abs (data[i]));

        const int   res    = (int) area.getWidth();
        const float midY   = area.getCentreY();
        const float halfH  = area.getHeight() * 0.45f;
        const int   step   = juce::jmax (1, n / juce::jmax (1, res));

        path.startNewSubPath (area.getX(), midY);
        for (int x = 0; x < res; ++x)
        {
            const int start = x * step;
            const int end   = juce::jmin (n, start + step);
            float lo = 0.0f, hi = 0.0f;
            for (int i = start; i < end; ++i)
            {
                lo = juce::jmin (lo, data[i]);
                hi = juce::jmax (hi, data[i]);
            }
            const float yHi = midY - (hi / peak) * halfH;
            const float yLo = midY - (lo / peak) * halfH;
            path.lineTo (area.getX() + (float) x, yHi);
            path.lineTo (area.getX() + (float) x, yLo);
        }
        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.0f));
    };

    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    auto top = bounds.removeFromTop (bounds.getHeight() * 0.5f);
    draw (leftPath,  cab.getIR (0), cabTint, top);
    draw (rightPath, cab.getIR (1), cabTint.brighter (0.4f), bounds);
}

void CabIRPlot::updateGraph()
{
    repaint();
}

void CabComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void CabComponent::setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label,
                                       const juce::String& text,
                                       double min, double max, double def)
{
    juce::ignoreUnused (label);
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setTextValueSuffix (" dB");
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, cabTint);
}

CabComponent::CabComponent (Cab& c,
                                    juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefix)
    : cab (c), apvts (state), irPlot (c)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Cab", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setColour (juce::ToggleButton::tickColourId, cabTint);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Cab_On", onButton);

    auto fillCombo = [&] (juce::ComboBox& box)
    {
        const auto& names = cab.getImpulseNames();
        for (int i = 0; i < names.size(); ++i)
            box.addItem (juce::File (names[i]).getFileNameWithoutExtension(), i + 1);
    };

    addAndMakeVisible (irLLabel);
    irLLabel.setText ("Left IR", juce::NotificationType::dontSendNotification);
    irLLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irLBox);
    fillCombo (irLBox);
    irLBox.onChange = [this] { graphNeedsUpdate = true; };
    irLAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRL", irLBox);

    addAndMakeVisible (irRLabel);
    irRLabel.setText ("Right IR", juce::NotificationType::dontSendNotification);
    irRLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irRBox);
    fillCombo (irRBox);
    irRBox.onChange = [this] { graphNeedsUpdate = true; };
    irRAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRR", irRBox);

    setupBarSlider (gainSlider, gainLabel, "Output Gain", -24.0, 24.0, 0.0);
    gainSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Cab_Gain", gainSlider));

    addAndMakeVisible (gainLabel);
    gainLabel.setText ("Gain", juce::NotificationType::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (irPlot);

    startTimerHz (24);
}

CabComponent::~CabComponent() = default;

void CabComponent::timerCallback()
{
    if (graphNeedsUpdate.exchange (false))
    {
        cab.checkParameters();
        irPlot.updateGraph();
    }
}

void CabComponent::paint (juce::Graphics& g)
{
    auto diagonale     = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length        = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height        = float (getWidth() * getHeight()) / length;
    auto base          = cabTint.darker (4.f);
    juce::ColourGradient grad (base.darker().darker(),  perpendicular *  height,
                               base,                    perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillAll();
}

void CabComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, slots, slotL, slotR, mid, gainCol, fMain;
    top.flexDirection     = juce::FlexBox::Direction::row;
    slots.flexDirection   = juce::FlexBox::Direction::row;
    slotL.flexDirection   = juce::FlexBox::Direction::column;
    slotR.flexDirection   = juce::FlexBox::Direction::column;
    mid.flexDirection     = juce::FlexBox::Direction::row;
    gainCol.flexDirection = juce::FlexBox::Direction::column;
    fMain.flexDirection   = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withFlex (0.2f));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    slotL.items.add (fi (irLLabel).withFlex (0.4f));
    slotL.items.add (fi (irLBox).withFlex (0.6f).withMargin (juce::FlexItem::Margin (4.f, 0.f, 4.f, 0.f)));
    slotR.items.add (fi (irRLabel).withFlex (0.4f));
    slotR.items.add (fi (irRBox).withFlex (0.6f).withMargin (juce::FlexItem::Margin (4.f, 0.f, 4.f, 0.f)));

    slots.items.add (fi (slotL).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 0.f)));
    slots.items.add (fi (slotR).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 5.f)));

    gainCol.items.add (fi (gainLabel).withFlex (0.15f));
    gainCol.items.add (fi (gainSlider).withFlex (0.85f));

    mid.items.add (fi (irPlot).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 8.f, 0.f, 0.f)));
    mid.items.add (fi (gainCol).withFlex (0.18f));

    fMain.items.add (fi (top).withFlex (0.12f).withMargin (juce::FlexItem::Margin (5.f, 0.f, 8.f, 0.f)));
    fMain.items.add (fi (slots).withFlex (0.25f));
    fMain.items.add (fi (mid).withFlex (1.0f).withMargin (juce::FlexItem::Margin (8.f, 0.f, 0.f, 0.f)));

    fMain.performLayout (area);
}
