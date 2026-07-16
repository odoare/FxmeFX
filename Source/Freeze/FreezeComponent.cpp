/*
  ==============================================================================

    FreezeComponent.cpp

  ==============================================================================
*/

#include "FreezeComponent.h"
#include "../Common/TopBar.h"

namespace
{
    const auto freezeTint = juce::Colour::fromRGB (140, 100, 220); // violet, same as Oct
}

//==============================================================================
SpectrumDisplay::SpectrumDisplay (Freeze& freezeToWatch, juce::Colour accentColour)
    : freeze (freezeToWatch), accent (accentColour)
{
    for (int i = 0; i < kFftSize; ++i)
        hannWindow[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                         * (float) i / (float) kFftSize);
    smoothedDb.fill (kFloorDb);
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

SpectrumDisplay::~SpectrumDisplay() = default;

void SpectrumDisplay::timerCallback()
{
    // Always drain the tap so it never backs up while frozen.
    float chunk[512];
    int n;
    while ((n = freeze.pullDisplaySamples (chunk, 512)) > 0)
        for (int i = 0; i < n; ++i)
        {
            sampleRing[(size_t) ringPos] = chunk[i];
            ringPos = (ringPos + 1) & (kFftSize - 1);
        }

    const bool frozen = freeze.isEngaged();
    if (frozen)
    {
        if (! wasFrozen)
        {
            wasFrozen = true;
            repaint();   // one last repaint to show the FROZEN state
        }
        return;
    }
    wasFrozen = false;

    updateSpectrum();
    repaint();
}

void SpectrumDisplay::updateSpectrum()
{
    for (int i = 0; i < kFftSize; ++i)
        fftData[(size_t) i] = sampleRing[(size_t) ((ringPos + i) & (kFftSize - 1))]
                              * hannWindow[(size_t) i];
    std::fill (fftData.begin() + kFftSize, fftData.end(), 0.0f);

    fft.performFrequencyOnlyForwardTransform (fftData.data());

    // Hann coherent gain is 0.5 and a full-scale sine peaks at N/2 in the
    // magnitude spectrum, so 4/N maps it to 0 dB.
    const float magToGain = 4.0f / (float) kFftSize;
    const double sr       = freeze.getSampleRate();
    const double binPerHz = (double) kFftSize / sr;

    for (int p = 0; p < kPoints; ++p)
    {
        const double f0 = kMinHz * std::pow (kMaxHz / kMinHz, ((double) p - 0.5) / (kPoints - 1));
        const double f1 = kMinHz * std::pow (kMaxHz / kMinHz, ((double) p + 0.5) / (kPoints - 1));
        const int b0 = juce::jlimit (1, kFftSize / 2 - 1, (int) std::floor (f0 * binPerHz));
        const int b1 = juce::jlimit (1, kFftSize / 2 - 1, (int) std::ceil  (f1 * binPerHz));

        float mag = 0.0f;
        for (int b = b0; b <= b1; ++b)
            mag = juce::jmax (mag, fftData[(size_t) b]);

        const float db = juce::Decibels::gainToDecibels (mag * magToGain, kFloorDb);

        // Fast rise, slower fall — keeps the plot lively but readable.
        auto& s = smoothedDb[(size_t) p];
        s += (db > s ? 0.6f : 0.2f) * (db - s);
    }
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const bool frozen = freeze.isEngaged();

    g.setColour (juce::Colour (0xff0e0d12));
    g.fillRoundedRectangle (bounds, 6.0f);

    auto plot = bounds.reduced (8.0f, 8.0f);

    // Faint grid: decades and 20 dB steps.
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    for (float hz : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = plot.getX() + plot.getWidth()
                        * (float) (std::log (hz / kMinHz) / std::log (kMaxHz / kMinHz));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
    }
    for (float db = -20.0f; db > kFloorDb; db -= 20.0f)
    {
        const float y = juce::jmap (db, kFloorDb, 0.0f, plot.getBottom(), plot.getY());
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
    }

    // Spectrum curve.
    juce::Path curve;
    for (int p = 0; p < kPoints; ++p)
    {
        const float x = plot.getX() + plot.getWidth() * (float) p / (float) (kPoints - 1);
        const float y = juce::jmap (juce::jlimit (kFloorDb, 0.0f, smoothedDb[(size_t) p]),
                                    kFloorDb, 0.0f, plot.getBottom(), plot.getY());
        if (p == 0) curve.startNewSubPath (x, y);
        else        curve.lineTo (x, y);
    }

    const auto lineColour = frozen ? accent.interpolatedWith (juce::Colours::white, 0.55f)
                                   : accent;

    juce::Path fill (curve);
    fill.lineTo (plot.getRight(), plot.getBottom());
    fill.lineTo (plot.getX(),     plot.getBottom());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (lineColour.withAlpha (frozen ? 0.40f : 0.30f),
                                             plot.getX(), plot.getY(),
                                             lineColour.withAlpha (0.02f),
                                             plot.getX(), plot.getBottom(), false));
    g.fillPath (fill);

    // Soft glow under a crisp line.
    g.setColour (lineColour.withAlpha (0.25f));
    g.strokePath (curve, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (lineColour.brighter (0.3f));
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    g.setColour (lineColour.withAlpha (frozen ? 0.9f : 0.35f));
    g.drawRoundedRectangle (bounds.reduced (0.75f), 6.0f, 1.5f);

    if (frozen)
    {
        g.setColour (lineColour);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText ("FROZEN", getLocalBounds().reduced (12, 8).removeFromTop (14),
                    juce::Justification::topRight);
    }
}

//==============================================================================
void FreezeComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

void FreezeComponent::setupRotary (fxme::FxmeSlider& slider, juce::Label& label,
                                   const juce::String& text,
                                   double min, double max, double def,
                                   const juce::String& suffix)
{
    juce::ignoreUnused (label);
    addAndMakeVisible (slider);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (def);
    slider.setTextValueSuffix (suffix);
    slider.setTooltip (text);
    slider.setName (text);
    slider.setShowLabel (true);
    slider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (slider, freezeTint);
}

FreezeComponent::FreezeComponent (Freeze& f,
                                  juce::AudioProcessorValueTreeState& state,
                                  const juce::String& prefix)
    : freeze (f), apvts (state), spectrumDisplay (f, freezeTint)
{
    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Freeze", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setColour (juce::ToggleButton::tickColourId, freezeTint);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_Freeze_On", onButton);

    addAndMakeVisible (spectrumDisplay);

    setupRotary (mixSlider,   mixLabel,   "Mix",   0.0, 100.0,  50.0, " %");
    setupRotary (widthSlider, widthLabel, "Width", 0.0, 100.0, 100.0, " %");

    mixSlider  .setAttachment (new SliderAttachment (apvts, prefix + "_Freeze_Mix",   mixSlider));
    widthSlider.setAttachment (new SliderAttachment (apvts, prefix + "_Freeze_Width", widthSlider));
}

FreezeComponent::~FreezeComponent() = default;

void FreezeComponent::paint (juce::Graphics& g)
{
    fxmefx::paintComponentBackground (g, getLocalBounds().toFloat(), freezeTint);
}

void FreezeComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, mixRow, fMain;
    top.flexDirection    = juce::FlexBox::Direction::row;
    mixRow.flexDirection = juce::FlexBox::Direction::row;
    fMain.flexDirection  = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));

    mixRow.items.add (fi().withFlex (1.0f));
    mixRow.items.add (fi (mixSlider).withWidth (150.0f));
    mixRow.items.add (fi().withFlex (0.5f));
    mixRow.items.add (fi (widthSlider).withWidth (150.0f));
    mixRow.items.add (fi().withFlex (1.0f));

    fMain.items.add (fi (top)            .withHeight (fxmefx::kHeaderRowHeight)
                                         .withMinHeight (fxmefx::kHeaderRowHeight)
                                         .withMargin (juce::FlexItem::Margin (5.f, 0.f, 8.f, 0.f)));
    fMain.items.add (fi (spectrumDisplay).withFlex (1.0f)
                                         .withMargin (juce::FlexItem::Margin (0.f, 2.f, 0.f, 2.f)));
    fMain.items.add (fi (mixRow)         .withHeight (110.0f)
                                         .withMargin (juce::FlexItem::Margin (6.f, 0.f, 0.f, 0.f)));

    fMain.performLayout (area);
}
