/*
  ==============================================================================

    ConvolReverbComponent.cpp

  ==============================================================================
*/

#include "ConvolReverbComponent.h"
#include "../Common/TopBar.h"

void ConvolReverbComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c);
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (1.0f));
}

void ConvolReverbComponent::setupSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::yellowgreen;

    juce::ignoreUnused (label);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setName (text);
    slider.getProperties().set ("showLabel", true);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

void ConvolReverbComponent::setupBarSlider (juce::Slider& slider, juce::Label& label, const juce::String& text, double min, double max, double def)
{
    juce::Colour color = juce::Colours::yellowgreen;

    addAndMakeVisible (label);
    label.setText (text, juce::NotificationType::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::LinearBarVertical);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTooltip (text);
    slider.setLookAndFeel(&fxmeLookAndFeel);
    setSliderColours(slider, color);
}

ImpulseResponsePlot::ImpulseResponsePlot(ConvolReverb& r) : reverb(r)
{
}

void ImpulseResponsePlot::updateGraph()
{
    irPlotPathL.clear();
    irPlotPathR.clear();
    irMaxPathL.clear();
    irMaxPathR.clear();

    const auto buffer = reverb.getModifiedIR();
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (numSamples == 0) return;
    
    float w = (float)getWidth();
    float graphTop = 0.0f;
    float graphHeight = (float)getHeight();

    if (w <= 0 || graphHeight <= 0) return;

    auto drawChannel = [&](int channel, float xOffset, float plotWidth, float centerY, float height)
    {
        juce::Path& p = (channel == 0) ? irPlotPathL : irPlotPathR;
        juce::Path& pMax = (channel == 0) ? irMaxPathL : irMaxPathR;
        auto* data = buffer.getReadPointer(channel);
 
        const float minDb = -90.0f;
        const float topY = centerY - height / 2.0f;
        const float bottomY = centerY + height / 2.0f;
 
        pMax.startNewSubPath (xOffset, bottomY);
 
        int resolution = juce::jmin(numSamples, (int)plotWidth);
        int step = numSamples / resolution;
        if (step == 0) step = 1;

        float maxVal = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            maxVal = juce::jmax(maxVal, std::abs(data[i]));
        if (maxVal == 0.0f) maxVal = 1.0f;
 
        for (int i = 0; i < resolution; ++i)
        {
            int start = i * step;
            int end = juce::jmin (start + step, numSamples);
            
            float sumSq = 0.0f;
            float maxAbs = 0.0f;
            for (int k = start; k < end; ++k)
            {
                maxAbs = juce::jmax(maxAbs, std::abs(data[k]));
                sumSq += data[k] * data[k];
            }
            
            float val = (end > start) ? std::sqrt (sumSq / (float) (end - start)) : 0.0f;

            float valDb = juce::Decibels::gainToDecibels (val / maxVal, minDb);
            float maxAbsDb = juce::Decibels::gainToDecibels (maxAbs / maxVal, minDb);

            float x = xOffset + (float)i / (float)resolution * plotWidth;
            float y = juce::jmap (valDb, minDb, 0.0f, bottomY, topY);
            float yMax = juce::jmap (maxAbsDb, minDb, 0.0f, bottomY, topY);
            
            if (i == 0)
                p.startNewSubPath (x, y);
            else
                p.lineTo (x, y);

            pMax.lineTo (x, yMax);
        }
 
        pMax.lineTo (xOffset + plotWidth, bottomY);
        pMax.closeSubPath();
    };

    if (numChannels == 1)
    {
        drawChannel(0, 0.f, w, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
    }
    else
    {
        float halfW = w * 0.5f;
        drawChannel(0, 0.f, halfW, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
        drawChannel(1, halfW, halfW, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
    }
}

void ImpulseResponsePlot::paint(juce::Graphics& g)
{
    // Draw grid lines for dB scale
    float graphTop = 0.0f;
    float graphHeight = (float)getHeight();
    const float minDb = -90.0f;
    const float maxDb = 0.0f;

    auto drawDbGrid = [&](float x, float width, float centerY, float height)
    {
        const float topY = centerY - height / 2.0f;
        const float bottomY = centerY + height / 2.0f;

        g.setColour (juce::Colours::white.withAlpha(0.3f));
        g.drawHorizontalLine (juce::roundToInt(topY), x, x + width); // 0dB

        for (float db = -18.0f; db > minDb; db -= 18.0f)
        {
            float y = juce::jmap (db, minDb, maxDb, bottomY, topY);
            g.setColour (juce::Colours::white.withAlpha (0.1f));
            g.drawHorizontalLine (juce::roundToInt(y), x, x + width);
        }
    };

    float w = (float)getWidth();
    if (irPlotPathR.isEmpty())
        drawDbGrid(0.f, w, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
    else
    {
        float halfW = w * 0.5f;
        drawDbGrid(0.f, halfW, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
        drawDbGrid(halfW, halfW, graphTop + graphHeight * 0.5f, graphHeight * 0.9f);
        g.setColour (juce::Colours::white.withAlpha(0.3f));
        g.drawVerticalLine (juce::roundToInt(halfW), graphTop, graphTop + graphHeight);
    }

    g.setColour (juce::Colours::lightgrey.withAlpha(0.5f));
    g.fillPath (irMaxPathL);
    if (!irMaxPathR.isEmpty())
        g.fillPath (irMaxPathR);

    g.setColour (juce::Colours::yellowgreen.withAlpha(0.7f));
    g.strokePath (irPlotPathL, juce::PathStrokeType (1.5f));
    if (!irPlotPathR.isEmpty())
        g.strokePath (irPlotPathR, juce::PathStrokeType (1.5f));
}

ConvolReverbComponent::ConvolReverbComponent (ConvolReverb& r, juce::AudioProcessorValueTreeState& state, const juce::String& pfx, bool showTitle)
    : reverb (r), apvts (state), irPlot(r), prefix (pfx),
      onButton (state, pfx + "_Rev_On", "On", juce::Colours::yellow)
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (juce::Colours::yellowgreen);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle);
    titleLabel.setText ("Convolution Reverb", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel(&fxmeLookAndFeel);

    addAndMakeVisible (irLabel);
    irLabel.setText ("Impulse", juce::NotificationType::dontSendNotification);
    irLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (irBox);
    irBox.setLookAndFeel (&fxmeLookAndFeel);
    irBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::yellowgreen.darker());
    irBox.setColour (juce::ComboBox::arrowColourId,   juce::Colours::yellowgreen.brighter (0.3f));
    const auto& names = reverb.getImpulseNames();
    for (int i = 0; i < names.size(); ++i)
        irBox.addItem (juce::File (names[i]).getFileNameWithoutExtension(), i + 1);
    externalSlotId = names.size() + 1;
    irBox.addItem ("External\xe2\x80\xa6", externalSlotId); // U+2026 horizontal ellipsis
    refreshExternalItemText();

    irBox.addMouseListener (&irBoxClickWatcher, true);

    irBox.onChange = [this]
    {
        graphNeedsUpdate = true;

        if (irBox.getSelectedId() == externalSlotId && userIRPick)
        {
            userIRPick = false;
            openExternalIRChooser();
        }
        else
        {
            userIRPick = false;
        }
    };

    irAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Rev_IR", irBox);

    // If the External slot is selected but there is neither an embedded IR nor
    // a resolvable legacy path, revert silently to the first built-in IR
    // (matches the chosen fallback semantics).
    if (irBox.getSelectedId() == externalSlotId)
    {
        if (! reverb.hasExternalIR())
        {
            reverb.setExternalIRPath ({});
            if (auto* p = apvts.getParameter (prefix + "_Rev_IR"))
                p->setValueNotifyingHost (p->convertTo0to1 (1.0f));
            refreshExternalItemText();
        }
    }

    setupSlider (lengthSlider, lengthLabel, "Length", 0.0, 1.0, 1.0);
    lengthSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Rev_Length", lengthSlider));
    lengthSlider.onValueChange = [this] { graphNeedsUpdate = true; };

    setupSlider (startOffsetSlider, startOffsetLabel, "Offset", -100.0, 100.0, 0.0);
    startOffsetSlider.setTextValueSuffix (" ms");
    startOffsetSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, prefix + "_Rev_StartOffset", startOffsetSlider));
    startOffsetSlider.onValueChange = [this] { graphNeedsUpdate = true; };

    addAndMakeVisible (shapeLabel);
    shapeLabel.setText ("Shape", juce::NotificationType::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shapeBox);
    shapeBox.setLookAndFeel (&fxmeLookAndFeel);
    shapeBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::yellowgreen.darker());
    shapeBox.setColour (juce::ComboBox::arrowColourId,   juce::Colours::yellowgreen.brighter (0.3f));
    shapeBox.addItem ("Fast Exp", 1);
    shapeBox.addItem ("Linear", 2);
    shapeBox.addItem ("Slow Log", 3);
    shapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, prefix + "_Rev_Shape", shapeBox);
    shapeBox.onChange = [this] { graphNeedsUpdate = true; };

    setupBarSlider(dryGainSlider, dryGainLabel, "Dry", -60.0, 6.0, -60.0);
    dryGainSlider.setTextValueSuffix(" dB");
    dryGainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Rev_DryGain", dryGainSlider));

    setupBarSlider(wetGainSlider, wetGainLabel, "Wet", -60.0, 6.0, 0.0);
    wetGainSlider.setTextValueSuffix(" dB");
    wetGainSlider.setAttachment(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, prefix + "_Rev_WetGain", wetGainSlider));
    
    addAndMakeVisible(irPlot);

    startTimerHz (24); // Update graph periodically
    irPlot.updateGraph();
}

ConvolReverbComponent::~ConvolReverbComponent()
{
    irBox.removeMouseListener (&irBoxClickWatcher);
}

void ConvolReverbComponent::refreshExternalItemText()
{
    if (externalSlotId == 0)
        return;

    const auto name = reverb.getExternalIRName();
    if (name.isNotEmpty())
        irBox.changeItemText (externalSlotId, "Ext: " + name);
    else
        irBox.changeItemText (externalSlotId, "External\xe2\x80\xa6");
}

void ConvolReverbComponent::openExternalIRChooser()
{
    const auto startDir = [this]
    {
        const auto cur = reverb.getExternalIRPath();
        if (cur.isNotEmpty())
        {
            juce::File f (cur);
            if (f.existsAsFile())
                return f.getParentDirectory();
        }
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    }();

    chooser = std::make_unique<juce::FileChooser> (
        "Select an impulse response file",
        startDir,
        "*.wav;*.aif;*.aiff;*.flac");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        const auto results = fc.getResults();

        // Embed the picked file into the state (FLAC+Base64) so presets and
        // sessions carry the IR itself. Treat an unreadable file like a cancel.
        const bool loaded = ! results.isEmpty() && reverb.setExternalIRFile (results[0]);

        if (! loaded)
        {
            // Revert combo to whatever the parameter currently holds.
            if (auto* raw = apvts.getRawParameterValue (prefix + "_Rev_IR"))
            {
                int paramId = (int) *raw;
                if (paramId == externalSlotId && ! reverb.hasExternalIR())
                    paramId = 1;
                irBox.setSelectedId (paramId, juce::sendNotificationSync);
            }
            return;
        }

        refreshExternalItemText();
        graphNeedsUpdate = true;

        // Make sure the parameter actually points at the External slot now.
        if (auto* p = apvts.getParameter (prefix + "_Rev_IR"))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) externalSlotId));
    });
}

void ConvolReverbComponent::timerCallback()
{
    // Redraw when a control moved, and again when the loader thread has actually
    // published the IR it selected — the two are a frame or so apart.
    const int generation = reverb.getIrGeneration();
    if (graphNeedsUpdate.exchange(false) || generation != lastIrGeneration)
    {
        lastIrGeneration = generation;
        irPlot.updateGraph();
        irPlot.repaint();
    }
}

void ConvolReverbComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), juce::Colours::yellowgreen);
}

void ConvolReverbComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    using fi = juce::FlexItem;
    juce::FlexBox fMain, fTop, fSliders, fBoxes, fSl1, fSl2,
                    fDry, fWet, fGains;
    fMain.flexDirection = juce::FlexBox::Direction::column;
    fTop.flexDirection = juce::FlexBox::Direction::row;
    fSliders.flexDirection = juce::FlexBox::Direction::row;
    fSl1.flexDirection = juce::FlexBox::Direction::column;
    fSl2.flexDirection = juce::FlexBox::Direction::column;
    fDry.flexDirection = juce::FlexBox::Direction::column;
    fWet.flexDirection = juce::FlexBox::Direction::column;
    fGains.flexDirection = juce::FlexBox::Direction::column;
    fBoxes.flexDirection = juce::FlexBox::Direction::column;

    fTop.items.add(fi(onButton).withWidth(fxmefx::kOnButtonWidth));
    fTop.items.add(fi(titleLabel).withFlex(1.5f));

    fBoxes.items.add(fi(irLabel).withFlex(0.5f));
    fBoxes.items.add(fi(irBox).withFlex(0.8f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 5.f, 0)));
    fBoxes.items.add(fi(shapeLabel).withFlex(0.5f));
    fBoxes.items.add(fi(shapeBox).withFlex(0.8f).withMargin(juce::FlexItem::Margin(5.f, 0.f, 5.f, 0)));
    fDry.items.add(fi(dryGainLabel).withFlex(0.2f));
    fDry.items.add(fi(dryGainSlider).withFlex(0.8f));
    fWet.items.add(fi(wetGainLabel).withFlex(0.2f));
    fWet.items.add(fi(wetGainSlider).withFlex(0.8f));

    fSliders.items.add(fi(fBoxes).withFlex(1.f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 0)));
    fSliders.items.add(fi(lengthSlider).withFlex(1.f));
    fSliders.items.add(fi(startOffsetSlider).withFlex(1.f));
    fSliders.items.add(fi(fDry).withFlex(.25f).withMargin(juce::FlexItem::Margin(0.f, 5.f, 0.f, 10.f)));
    fSliders.items.add(fi(fWet).withFlex(.25f).withMargin(juce::FlexItem::Margin(0.f, 10.f, 0.f, 5.f)));
    

    fMain.items.add(fi(fTop).withHeight(fxmefx::kHeaderRowHeight)
                            .withMinHeight(fxmefx::kHeaderRowHeight)
                            .withMargin(juce::FlexItem::Margin(5.f, 0.f, 8.f, 0.f)));
    fMain.items.add(fi(fSliders).withFlex(0.35f));
    // Graph will take the remaining space
    fMain.items.add(fi(irPlot).withFlex(0.55f));

    fMain.performLayout(area);
    
    irPlot.updateGraph();
}