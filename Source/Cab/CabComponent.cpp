/*
  ==============================================================================

    CabComponent.cpp

  ==============================================================================
*/

#include "CabComponent.h"
#include "../Common/TopBar.h"

namespace
{
    const auto cabTint = juce::Colours::orange;
}

void CabIRPlot::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    const auto area = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (tint.withAlpha (0.25f));
    g.fillRect (area);

    path.clear();
    const auto ir = cab.getIR (channel);
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
    g.setColour (tint);
    g.strokePath (path, juce::PathStrokeType (1.0f));
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

void CabComponent::setupGainSlider (fxme::FxmeSlider& slider, juce::Label& label,
                                        const juce::String& text)
{
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setName (text);
    slider.setTooltip (text);
    slider.setTextValueSuffix (" dB");
    slider.setCentralValue (0.0);   // bipolar ±dB: the bar grows from 0 dB
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, cabTint);

    addAndMakeVisible (label);
    label.setText ("Gain", juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
}

CabComponent::CabComponent (Cab& c,
                                    juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefix,
                                    bool showTitle)
    : cab (c), apvts (state),
      onButton (state, prefix + "_Cab_On", "On", cabTint),
      irLPlot (c, 0, cabTint),
      irRPlot (c, 1, cabTint.brighter (0.4f))
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (cabTint);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Cab", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);

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
    irLBox.setLookAndFeel (&fxmeLookAndFeel);
    irLBox.setColour (juce::ComboBox::outlineColourId, cabTint.darker());
    irLBox.setColour (juce::ComboBox::arrowColourId,   cabTint.brighter (0.3f));
    fillCombo (irLBox);
    irLBox.onChange = [this] { graphNeedsUpdate = true; };
    irLAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRL", irLBox);

    addAndMakeVisible (irRLabel);
    irRLabel.setText ("Right IR", juce::NotificationType::dontSendNotification);
    irRLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irRBox);
    irRBox.setLookAndFeel (&fxmeLookAndFeel);
    irRBox.setColour (juce::ComboBox::outlineColourId, cabTint.darker());
    irRBox.setColour (juce::ComboBox::arrowColourId,   cabTint.brighter (0.3f));
    fillCombo (irRBox);
    irRBox.onChange = [this] { graphNeedsUpdate = true; };
    irRAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRR", irRBox);

    setupGainSlider (gainLSlider, gainLLabel, "Left Gain");
    gainLSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Cab_GainL", gainLSlider));

    setupGainSlider (gainRSlider, gainRLabel, "Right Gain");
    gainRSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Cab_GainR", gainRSlider));

    addAndMakeVisible (irLPlot);
    addAndMakeVisible (irRPlot);

    startTimerHz (24);
}

CabComponent::~CabComponent() = default;

void CabComponent::timerCallback()
{
    // Redraw when a control moved, and again when the loader thread has actually
    // published the IRs it selected — the two are a frame or so apart.
    const int generation = cab.getIrGeneration();
    if (graphNeedsUpdate.exchange (false) || generation != lastIrGeneration)
    {
        lastIrGeneration = generation;
        irLPlot.updateGraph();
        irRPlot.updateGraph();
    }
}

void CabComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), cabTint);
}

void CabComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, cols, colL, colR, gainRowL, gainRowR, fMain;
    top.flexDirection      = juce::FlexBox::Direction::row;
    cols.flexDirection     = juce::FlexBox::Direction::row;
    colL.flexDirection     = juce::FlexBox::Direction::column;
    colR.flexDirection     = juce::FlexBox::Direction::column;
    gainRowL.flexDirection = juce::FlexBox::Direction::row;
    gainRowR.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection    = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    auto buildColumn = [] (juce::FlexBox& col, juce::FlexBox& gainRow,
                           juce::Label& header, juce::ComboBox& box, CabIRPlot& plot,
                           juce::Label& gainLabel, fxme::FxmeSlider& gainSlider)
    {
        gainRow.items.add (fi (gainLabel).withFlex (0.25f));
        gainRow.items.add (fi (gainSlider).withFlex (0.75f));

        // The three control rows are sized by their minimum, not by their share
        // of the column: a maximum alone caps a row that has room to spare and
        // does nothing for one that has not, which in a short panel left the IR
        // chooser about 12 px tall. Flex-shrink 0 keeps them at that minimum
        // even when the column cannot fit everything.
        //
        // The plot is the only row that yields — it degrades into a smaller
        // waveform, while the others degrade into being unusable.
        col.items.add (fi (header).withFlex (0.10f, 0.0f)
                                  .withMinHeight (18.0f).withMaxHeight (24.0f));
        col.items.add (fi (box).withFlex (0.12f, 0.0f)
                               .withMinHeight (28.0f).withMaxHeight (32.0f)
                               .withMargin (juce::FlexItem::Margin (4.f, 0.f, 4.f, 0.f)));
        col.items.add (fi (plot).withFlex (1.0f, 4.0f).withMinHeight (0.0f));
        col.items.add (fi (gainRow).withFlex (0.18f, 0.0f)
                                   .withMinHeight (30.0f).withMaxHeight (36.0f)
                                   .withMargin (juce::FlexItem::Margin (8.f, 0.f, 0.f, 0.f)));
    };

    buildColumn (colL, gainRowL, irLLabel, irLBox, irLPlot, gainLLabel, gainLSlider);
    buildColumn (colR, gainRowR, irRLabel, irRBox, irRPlot, gainRLabel, gainRSlider);

    cols.items.add (fi (colL).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 0.f)));
    cols.items.add (fi (colR).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 5.f)));

    fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                             .withMinHeight (fxmefx::kHeaderRowHeight)
                             .withMargin (juce::FlexItem::Margin (5.f, 0.f, 8.f, 0.f)));
    fMain.items.add (fi (cols).withFlex (1.0f));

    fMain.performLayout (area);
}
