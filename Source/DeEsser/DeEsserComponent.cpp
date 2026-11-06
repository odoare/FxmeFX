/*
  ==============================================================================

    DeEsserComponent.cpp

  ==============================================================================
*/

#include "DeEsserComponent.h"
#include "../Common/TopBar.h"
#include "../Common/Controls.h"

//==============================================================================
// DeEsserSpectrum

void DeEsserSpectrum::setBand (float newFreqHz, float newQ, bool newBandPass, float newGainDb, bool newActive)
{
    if (newFreqHz == freqHz && newQ == q && newBandPass == bandPass && newGainDb == gainDb && newActive == active)
        return;
    freqHz = newFreqHz;
    q = newQ;
    bandPass = newBandPass;
    gainDb = newGainDb;
    active = newActive;
    repaint();
}

float DeEsserSpectrum::xToFreq (float x) const
{
    const auto plot = getPlotArea();
    const float t = (x - plot.getX()) / juce::jmax (1.0f, plot.getWidth());
    return getViewLowHz() * std::pow (getViewHighHz() / getViewLowHz(), t);
}

bool DeEsserSpectrum::nearHandle (juce::Point<float> p) const
{
    const auto plot = getPlotArea();
    return plot.contains (p) && std::abs (p.x - freqToX (freqHz, plot)) <= 8.0f;
}

void DeEsserSpectrum::paintBehindTraces (juce::Graphics& g, juce::Rectangle<float> plot)
{
    // The band: one bandwidth wide around the centre for the band-pass
    // (from Q: 2 asinh (1 / 2Q) / ln 2 octaves), everything above for the
    // high-pass. Shaded more strongly as the reduction deepens.
    float lo = freqHz, hi = getViewHighHz();
    if (bandPass)
    {
        const float halfOctaves = std::asinh (1.0f / (2.0f * juce::jmax (0.1f, q))) / std::log (2.0f);
        lo = freqHz * std::pow (2.0f, -halfOctaves);
        hi = freqHz * std::pow (2.0f,  halfOctaves);
    }
    const float x0 = juce::jmax (plot.getX(), freqToX (lo, plot));
    const float x1 = juce::jmin (plot.getRight(), freqToX (hi, plot));
    if (x1 > x0)
    {
        const float depth = juce::jlimit (0.0f, 1.0f, -gainDb / 12.0f);
        auto c = active ? accent : accent.withMultipliedSaturation (0.2f);
        g.setColour (c.withAlpha (0.10f + 0.25f * depth));
        g.fillRect (juce::Rectangle<float> (x0, plot.getY(), x1 - x0, plot.getHeight()));
    }
}

void DeEsserSpectrum::paintOverTraces (juce::Graphics& g, juce::Rectangle<float> plot)
{
    const float x = freqToX (freqHz, plot);
    auto c = active ? accent : accent.withMultipliedSaturation (0.2f).withMultipliedAlpha (0.6f);
    g.setColour (c.withAlpha (0.8f));
    g.drawVerticalLine ((int) std::round (x), plot.getY(), plot.getBottom());

    auto handle = juce::Rectangle<float> (12.0f, 12.0f).withCentre ({ x, plot.getY() + 12.0f });
    g.setColour (c);
    g.fillEllipse (handle);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (handle, 1.0f);

    const auto label = freqHz >= 1000.0f ? juce::String (freqHz * 0.001f, 1) + " kHz"
                                         : juce::String (juce::roundToInt (freqHz)) + " Hz";
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::FontOptions (11.0f));
    const bool labelLeft = x > plot.getRight() - 60.0f;
    g.drawText (label, juce::Rectangle<float> (labelLeft ? x - 62.0f : x + 9.0f, plot.getY() + 4.0f, 54.0f, 16.0f),
                labelLeft ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
}

void DeEsserSpectrum::mouseDown (const juce::MouseEvent& e)
{
    draggingBand = e.mods.isLeftButtonDown() && nearHandle (e.position);
    if (draggingBand)
    {
        if (onFrequencyDragStart) onFrequencyDragStart();
        return;
    }
    SpectrumDisplay::mouseDown (e);
}

void DeEsserSpectrum::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingBand)
    {
        if (onFrequencyDrag) onFrequencyDrag (xToFreq (e.position.x));
        return;
    }
    SpectrumDisplay::mouseDrag (e);
}

void DeEsserSpectrum::mouseUp (const juce::MouseEvent& e)
{
    if (draggingBand)
    {
        draggingBand = false;
        if (onFrequencyDragEnd) onFrequencyDragEnd();
        return;
    }
    SpectrumDisplay::mouseUp (e);
}

void DeEsserSpectrum::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (nearHandle (e.position) ? juce::MouseCursor::LeftRightResizeCursor
                                            : juce::MouseCursor::NormalCursor);
    SpectrumDisplay::mouseMove (e);
}

//==============================================================================
// DeEsserComponent

void DeEsserComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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

void DeEsserComponent::setupCombo (juce::ComboBox& box, const juce::StringArray& items, const juce::String& paramId,
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

void DeEsserComponent::beginGesture (const juce::String& id)
{
    if (auto* p = apvts.getParameter (id)) p->beginChangeGesture();
}

void DeEsserComponent::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (p->getNormalisableRange().snapToLegalValue (value)));
}

void DeEsserComponent::endGesture (const juce::String& id)
{
    if (auto* p = apvts.getParameter (id)) p->endChangeGesture();
}

DeEsserComponent::DeEsserComponent (DeEsser& d, juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefixIn, bool showTitle, bool compactLayout)
    : deEsser (d), apvts (state), prefix (prefixIn), compact (compactLayout),
      onButton       (state, prefixIn + "_DeEss_On",       "On",       tint()),
      listenButton   (state, prefixIn + "_DeEss_Listen",   "Listen",   tint()),
      relativeButton (state, prefixIn + "_DeEss_Relative", "Relative", tint())
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("De-esser", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    for (auto* b : { &onButton, &listenButton, &relativeButton })
    {
        addAndMakeVisible (*b);
        b->setLookAndFeel (&fxmeLookAndFeel);
    }
    listenButton.button.setTooltip ("Listen. \n Hear only the detector's band, to find the sibilance: "
                                    "sweep the frequency until the esses stand out.");
    relativeButton.button.setTooltip ("Relative. \n The threshold compares the band with the whole signal "
                                      "instead of with full scale, so it holds whatever the input level.");

    setupCombo (modeBox, { "Wideband", "Split band" }, prefix + "_DeEss_Mode",
                "Mode. \n Wideband: the whole signal is turned down while the band is too loud (classic, safe). "
                "Split band: only the band is.");
    setupCombo (filterBox, { "Band-pass", "High-pass" }, prefix + "_DeEss_Filter",
                "Detector. \n Band-pass: listens around the frequency (Q sets the width). "
                "High-pass: listens to everything above it.");
    setupCombo (stereoBox, { "Linked", "Mid" }, prefix + "_DeEss_Stereo",
                "Stereo. \n Linked: both channels alike. Mid: detects on the centre and de-esses only it, "
                "leaving the sides (reverb, wide cymbals) alone.");

    setupRotary (freqSlider, "Frequency", " Hz",
                 "Frequency. \n Where the detector listens (also draggable on the spectrum).");
    setupRotary (qSlider, "Q", "",
                 "Q. \n The detector band's width (band-pass only): higher is narrower.");
    setupRotary (threshSlider, "Threshold", " dB",
                 "Threshold. \n The band's level above which it is turned down (also draggable on the meter).");
    setupRotary (rangeSlider, "Range", " dB",
                 "Range. \n The deepest reduction.");
    setupRotary (speedSlider, "Speed", " %",
                 "Speed. \n How fast the reduction follows the esses: attack and release together.");
    freqSlider.setNumDecimalPlacesToDisplay (0);
    qSlider.setNumDecimalPlacesToDisplay (2);
    speedSlider.setNumDecimalPlacesToDisplay (0);

    auto attach = [&] (fxme::FxmeSlider& s, const char* id)
    {
        s.setAttachment (new SliderAttachment (apvts, prefix + "_DeEss_" + id, s));
    };
    attach (freqSlider,   "Freq");
    attach (qSlider,      "Q");
    attach (threshSlider, "Thresh");
    attach (rangeSlider,  "Range");
    attach (speedSlider,  "Speed");
    freqSlider.setLogarithmic();

    freqParam   = apvts.getRawParameterValue (prefix + "_DeEss_Freq");
    qParam      = apvts.getRawParameterValue (prefix + "_DeEss_Q");
    filterParam = apvts.getRawParameterValue (prefix + "_DeEss_Filter");
    threshParam = apvts.getRawParameterValue (prefix + "_DeEss_Thresh");

    // The meter: the detector's level against the threshold (drag it), and
    // the reduction.
    fxme::DynamicsMeter::Colours mc;
    mc.level     = tint();
    mc.threshold = tint().brighter (0.6f);
    mc.cut       = fxmefx::theme::gainReduction;
    meter.setColours (mc);
    meter.setLevelRange (DeEsser::kMinLevelDb, 0.0f);
    meter.setGainRange (DeEsser::kMaxRangeDb);
    meter.setLabels ("Band", "GR");
    meter.setTooltip ("Band: the detector's level (against the whole signal in Relative mode), with the "
                      "threshold across it; drag the threshold to set it. \n GR: the reduction.");
    const auto threshId = prefix + "_DeEss_Thresh";
    meter.onThresholdDragStart = [this, threshId] { beginGesture (threshId); };
    meter.onThresholdDrag      = [this, threshId] (float db) { setParam (threshId, db); };
    meter.onThresholdDragEnd   = [this, threshId] { endGesture (threshId); };
    addAndMakeVisible (meter);

    // The spectrum, with the band: not in the compact layout.
    if (! compact)
    {
        deEsser.getInputTap() .setEnabled (true);
        deEsser.getOutputTap().setEnabled (true);
        spectrum.accent = tint();
        spectrum.sampleRateProvider = [this] { return deEsser.getSampleRate(); };
        spectrum.addTrace ({ &deEsser.getInputTap(),  juce::Colour (0xff8a8a96), [] { return juce::String ("In"); },  1.2f });
        spectrum.addTrace ({ &deEsser.getOutputTap(), tint(),                     [] { return juce::String ("Out"); }, 1.4f });
        spectrum.setDbRange (-90.0f, 0.0f);
        spectrum.setFreqWindow (200.0f, 20000.0f);
        const auto freqId = prefix + "_DeEss_Freq";
        spectrum.onFrequencyDragStart = [this, freqId] { beginGesture (freqId); };
        spectrum.onFrequencyDrag      = [this, freqId] (float hz)
        {
            setParam (freqId, juce::jlimit (DeEsser::kMinFreqHz, DeEsser::kMaxFreqHz, hz));
        };
        spectrum.onFrequencyDragEnd   = [this, freqId] { endGesture (freqId); };
        addAndMakeVisible (spectrum);
    }

    timerCallback();
    startTimerHz (30);
}

DeEsserComponent::~DeEsserComponent()
{
    stopTimer();
    if (! compact)
    {
        deEsser.getInputTap() .setEnabled (false);
        deEsser.getOutputTap().setEnabled (false);
    }
}

void DeEsserComponent::timerCallback()
{
    const bool on = deEsser.isOn();
    const float threshold = threshParam != nullptr ? threshParam->load() : -30.0f;
    const float gainDb = deEsser.getDisplayGain().load();
    meter.update (deEsser.getDisplayLevel().load(), threshold, gainDb, on);

    const bool bandPass = filterParam == nullptr || filterParam->load() < 0.5f;
    if (qSlider.isEnabled() != bandPass)
        qSlider.setEnabled (bandPass);

    if (! compact)
        spectrum.setBand (freqParam != nullptr ? freqParam->load() : 6000.0f,
                          qParam != nullptr ? qParam->load() : 1.5f,
                          bandPass, gainDb, on);
}

void DeEsserComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void DeEsserComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, knobs, left, body, fMain;
    top.flexDirection   = juce::FlexBox::Direction::row;
    knobs.flexDirection = juce::FlexBox::Direction::row;
    left.flexDirection  = juce::FlexBox::Direction::column;
    body.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    const auto boxMargin = juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f);
    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));
    top.items.add (fi (listenButton).withWidth (76.0f));
    top.items.add (fi (relativeButton).withWidth (88.0f));
    top.items.add (fi (modeBox).withWidth (110.0f).withMargin (boxMargin));
    top.items.add (fi (filterBox).withWidth (100.0f).withMargin (boxMargin));
    top.items.add (fi (stereoBox).withWidth (80.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

    for (auto* s : { &freqSlider, &qSlider, &threshSlider, &rangeSlider, &speedSlider })
        knobs.items.add (fi (*s).withFlex (1.0f));

    if (compact)
    {
        knobs.items.add (fi (meter).withWidth (60.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));
        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        fMain.items.add (fi (knobs).withFlex (1.0f));
        fMain.performLayout (area);
        return;
    }

    // The spectrum takes what the knob row leaves.
    left.items.add (fi (spectrum).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 6.f, 0.f)));
    left.items.add (fi (knobs).withHeight (kKnobRowHeight));

    body.items.add (fi (left).withFlex (1.0f));
    body.items.add (fi (meter).withWidth (76.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 10.f)));

    fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                             .withMinHeight (fxmefx::kHeaderRowHeight)
                             .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (body).withFlex (1.0f));
    fMain.performLayout (area);
}
