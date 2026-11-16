/*
  ==============================================================================

    CabComponent.cpp

  ==============================================================================
*/

#include "CabComponent.h"
#include "../Common/Theme.h"
#include "../Common/Controls.h"
#include "../Common/TopBar.h"

namespace
{
    const auto cabTint = fxmefx::theme::cab;
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
    // A horizontal console fader (the range comes from the parameter).
    fxmefx::setupGainFader (*this, slider, text, cabTint, fxmeLookAndFeel, -24.0, 24.0, false);

    addAndMakeVisible (label);
    label.setText ("Gain", juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
}

CabComponent::CabComponent (Cab& c,
                                    juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefix,
                                    bool showTitle,
                                    bool compactLayout)
    : cab (c), apvts (state), paramPrefix (prefix), compact (compactLayout),
      onButton (state, prefix + "_Cab_On", "On", cabTint),
      irLPlot (c, 0, cabTint),
      irRPlot (c, 1, cabTint.brighter (0.4f))
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (cabTint);

    addChildComponent (titleLabel);
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);
    titleLabel.setVisible (showTitle && ! compact);
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
        // One past the last built-in: the user's own file (the Load button's).
        box.addItem ("External file", cab.getExternalIndex() + 1);
    };

    addAndMakeVisible (irLLabel);
    irLLabel.setText ("Left IR", juce::NotificationType::dontSendNotification);
    irLLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irLBox);
    irLBox.setLookAndFeel (&fxmeLookAndFeel);
    irLBox.setColour (juce::ComboBox::outlineColourId, cabTint.darker());
    irLBox.setColour (juce::ComboBox::arrowColourId,   cabTint.brighter (0.3f));
    fillCombo (irLBox);
    irLBox.onChange = [this]
    {
        graphNeedsUpdate = true;
        // Picking "External file" with none loaded asks for one.
        if (irLBox.getSelectedId() == cab.getExternalIndex() + 1 && ! cab.hasExternalIR (0))
            openExternalIRChooser (0);
    };

    addAndMakeVisible (irLLoad);
    irLLoad.setLookAndFeel (&fxmeLookAndFeel);
    irLLoad.setColour (juce::TextButton::buttonColourId, cabTint.darker (0.6f));
    irLLoad.setTooltip ("Load an impulse response of your own (WAV, AIFF or FLAC) for this side. It is stored in the session and in presets.");
    irLLoad.onClick = [this] { openExternalIRChooser (0); };
    irLAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRL", irLBox);

    addAndMakeVisible (irRLabel);
    irRLabel.setText ("Right IR", juce::NotificationType::dontSendNotification);
    irRLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irRBox);
    irRBox.setLookAndFeel (&fxmeLookAndFeel);
    irRBox.setColour (juce::ComboBox::outlineColourId, cabTint.darker());
    irRBox.setColour (juce::ComboBox::arrowColourId,   cabTint.brighter (0.3f));
    fillCombo (irRBox);
    irRBox.onChange = [this]
    {
        graphNeedsUpdate = true;
        // Picking "External file" with none loaded asks for one.
        if (irRBox.getSelectedId() == cab.getExternalIndex() + 1 && ! cab.hasExternalIR (1))
            openExternalIRChooser (1);
    };

    addAndMakeVisible (irRLoad);
    irRLoad.setLookAndFeel (&fxmeLookAndFeel);
    irRLoad.setColour (juce::TextButton::buttonColourId, cabTint.darker (0.6f));
    irRLoad.setTooltip ("Load an impulse response of your own (WAV, AIFF or FLAC) for this side. It is stored in the session and in presets.");
    irRLoad.onClick = [this] { openExternalIRChooser (1); };
    irRAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Cab_IRR", irRBox);

    setupGainSlider (gainLSlider, gainLLabel, "Left Gain");
    gainLSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Cab_GainL", gainLSlider));

    setupGainSlider (gainRSlider, gainRLabel, "Right Gain");
    gainRSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Cab_GainR", gainRSlider));

    addAndMakeVisible (irLPlot);
    addAndMakeVisible (irRPlot);

    refreshExternalItemText();
    startTimerHz (24);
}

CabComponent::~CabComponent()
{
    irLLoad.setLookAndFeel (nullptr);
    irRLoad.setLookAndFeel (nullptr);
}

void CabComponent::refreshExternalItemText()
{
    const int id = cab.getExternalIndex() + 1;
    for (int ch = 0; ch < Cab::NumSlots; ++ch)
    {
        const auto name = cab.getExternalIRName (ch);
        const auto text = name.isNotEmpty() ? "Ext: " + juce::File (name).getFileNameWithoutExtension()
                                            : juce::String ("External file");
        auto& shown = ch == 0 ? shownExtL : shownExtR;
        if (text != shown)
        {
            shown = text;
            boxFor (ch).changeItemText (id, text);
        }
    }
}

void CabComponent::openExternalIRChooser (int channel)
{
    chooser = std::make_unique<juce::FileChooser> (
        channel == 0 ? "Select the left impulse response" : "Select the right impulse response",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.wav;*.aif;*.aiff;*.flac");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    // Safe against the editor closing while the chooser is open.
    juce::Component::SafePointer<CabComponent> safe (this);
    chooser->launchAsync (chooserFlags, [safe, channel] (const juce::FileChooser& fc)
    {
        if (safe == nullptr)
            return;
        auto& self = *safe;

        const auto results = fc.getResults();
        const bool loaded = ! results.isEmpty() && self.cab.setExternalIRFile (channel, results[0]);

        const auto* paramId = channel == 0 ? "_Cab_IRL" : "_Cab_IRR";
        auto* param = self.apvts.getParameter (self.paramPrefix + paramId);

        if (! loaded)
        {
            // A cancel or an unreadable file: the box shows what the parameter holds.
            if (param != nullptr)
                self.boxFor (channel).setSelectedId ((int) param->convertFrom0to1 (param->getValue()),
                                                     juce::dontSendNotification);
            return;
        }

        self.refreshExternalItemText();
        self.graphNeedsUpdate = true;
        if (param != nullptr)
            param->setValueNotifyingHost (param->convertTo0to1 ((float) (self.cab.getExternalIndex() + 1)));
        if (self.onExternalLoaded)
            self.onExternalLoaded();
    });
}

void CabComponent::timerCallback()
{
    // Redraw when a control moved, and again when the loader thread has actually
    // published the IRs it selected — the two are a frame or so apart.
    const int generation = cab.getIrGeneration();
    if (graphNeedsUpdate.exchange (false) || generation != lastIrGeneration)
    {
        lastIrGeneration = generation;
        refreshExternalItemText();
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

    juce::FlexBox top, cols, colL, colR, gainRowL, gainRowR, boxRowL, boxRowR, fMain;
    top.flexDirection      = juce::FlexBox::Direction::row;
    cols.flexDirection     = juce::FlexBox::Direction::row;
    colL.flexDirection     = juce::FlexBox::Direction::column;
    colR.flexDirection     = juce::FlexBox::Direction::column;
    gainRowL.flexDirection = juce::FlexBox::Direction::row;
    gainRowR.flexDirection = juce::FlexBox::Direction::row;
    boxRowL.flexDirection  = juce::FlexBox::Direction::row;
    boxRowR.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection    = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    auto buildColumn = [] (juce::FlexBox& col, juce::FlexBox& gainRow, juce::FlexBox& boxRow,
                           juce::Label& header, juce::ComboBox& box, juce::TextButton& load, CabIRPlot& plot,
                           juce::Label& gainLabel, fxme::FxmeSlider& gainSlider)
    {
        gainRow.items.add (fi (gainLabel).withFlex (0.25f));
        gainRow.items.add (fi (gainSlider).withFlex (0.75f));

        // The chooser and, beside it, the button for a file of the user's.
        boxRow.items.add (fi (box).withFlex (1.0f));
        boxRow.items.add (fi (load).withWidth (60.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f)));

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
        col.items.add (fi (boxRow).withFlex (0.12f, 0.0f)
                               .withMinHeight (28.0f).withMaxHeight (32.0f)
                               .withMargin (juce::FlexItem::Margin (4.f, 0.f, 4.f, 0.f)));
        col.items.add (fi (plot).withFlex (1.0f, 4.0f).withMinHeight (0.0f));
        col.items.add (fi (gainRow).withFlex (0.18f, 0.0f)
                                   .withMinHeight (40.0f).withMaxHeight (46.0f)   // the fader's scale under it
                                   .withMargin (juce::FlexItem::Margin (8.f, 0.f, 0.f, 0.f)));
    };

    buildColumn (colL, gainRowL, boxRowL, irLLabel, irLBox, irLLoad, irLPlot, gainLLabel, gainLSlider);
    buildColumn (colR, gainRowR, boxRowR, irRLabel, irRBox, irRLoad, irRPlot, gainRLabel, gainRSlider);

    cols.items.add (fi (colL).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 0.f)));
    cols.items.add (fi (colR).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 5.f)));

    const float headerHeight = compact ? 24.0f : fxmefx::kHeaderRowHeight;
    fMain.items.add (fi (top).withHeight (headerHeight)
                             .withMinHeight (headerHeight)
                             .withMargin (juce::FlexItem::Margin (compact ? 0.f : 5.f, 0.f, compact ? 4.f : 8.f, 0.f)));
    fMain.items.add (fi (cols).withFlex (1.0f));

    fMain.performLayout (area);
}
