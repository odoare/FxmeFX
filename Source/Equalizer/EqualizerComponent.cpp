/*
  ==============================================================================

    EqualizerComponent.cpp

  ==============================================================================
*/

#include "EqualizerComponent.h"

#include <complex>

//==============================================================================
// FrequencyResponseGraph Implementation
//==============================================================================

void FrequencyResponseGraph::setReferences (std::array<BandRefs, Equalizer::NumBands> b,
                                            juce::Slider& postG,
                                            juce::ToggleButton& onB)
{
    bands    = b;
    postGain = &postG;
    onBtn    = &onB;
    updateCurve();
}

Equalizer::BandType FrequencyResponseGraph::getBandType (int i) const noexcept
{
    if (bands[i].type == nullptr)
        return Equalizer::BandType::Peak;
    int idx = bands[i].type->getSelectedItemIndex();
    if (idx < 0) idx = (int) Equalizer::BandType::Peak;
    return (Equalizer::BandType) idx;
}

static double evalMagnitude (double b0, double b1, double b2, double a1, double a2, double freq, double sr)
{
    double w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
    std::complex<double> z (std::cos (w), std::sin (w));
    std::complex<double> z_1 = std::conj (z);
    std::complex<double> z_2 = z_1 * z_1;
    std::complex<double> num = b0 + b1 * z_1 + b2 * z_2;
    std::complex<double> den = 1.0 + a1 * z_1 + a2 * z_2;
    return std::abs (num) / std::abs (den);
}

namespace
{
    struct BiquadCoeffs { double b0, b1, b2, a1, a2; };

    BiquadCoeffs computeCoeffs (Equalizer::BandType type, double f, double q, double g, double sr)
    {
        BiquadCoeffs c { 1.0, 0.0, 0.0, 0.0, 0.0 };
        double w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
        double cos_w0 = std::cos (w0);
        double sin_w0 = std::sin (w0);

        switch (type)
        {
            case Equalizer::BandType::Lowpass:
            {
                juce::ignoreUnused (g);
                double alpha = sin_w0 / (2.0 * q);
                double a0 = 1.0 + alpha;
                double b0 = (1.0 - cos_w0) * 0.5;
                double b1 =  1.0 - cos_w0;
                double b2 = (1.0 - cos_w0) * 0.5;
                c = { b0 / a0, b1 / a0, b2 / a0, -2.0 * cos_w0 / a0, (1.0 - alpha) / a0 };
                break;
            }
            case Equalizer::BandType::Highpass:
            {
                juce::ignoreUnused (g);
                double alpha = sin_w0 / (2.0 * q);
                double a0 = 1.0 + alpha;
                double b0 =  (1.0 + cos_w0) * 0.5;
                double b1 = -(1.0 + cos_w0);
                double b2 =  (1.0 + cos_w0) * 0.5;
                c = { b0 / a0, b1 / a0, b2 / a0, -2.0 * cos_w0 / a0, (1.0 - alpha) / a0 };
                break;
            }
            case Equalizer::BandType::Peak:
            {
                double A = std::pow (10.0, g / 40.0);
                double alpha = sin_w0 / (2.0 * q);
                double a0 = 1.0 + alpha / A;
                c = { (1.0 + alpha * A) / a0, -2.0 * cos_w0 / a0, (1.0 - alpha * A) / a0,
                      -2.0 * cos_w0 / a0, (1.0 - alpha / A) / a0 };
                break;
            }
            case Equalizer::BandType::Lowshelf:
            {
                double A = std::pow (10.0, g / 40.0);
                double alpha = sin_w0 / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / 1.0 - 1.0) + 2.0);
                double a0 = (A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha;
                double b0 =       A * ((A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha);
                double b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
                double b2 =       A * ((A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha);
                double a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0);
                double a2 =        (A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha;
                c = { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
                break;
            }
            case Equalizer::BandType::Highshelf:
            {
                double A = std::pow (10.0, g / 40.0);
                double alpha = sin_w0 / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / 1.0 - 1.0) + 2.0);
                double a0 = (A + 1.0) - (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha;
                double b0 =       A * ((A + 1.0) + (A - 1.0) * cos_w0 + 2.0 * std::sqrt (A) * alpha);
                double b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
                double b2 =       A * ((A + 1.0) + (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha);
                double a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0);
                double a2 =       (A + 1.0) - (A - 1.0) * cos_w0 - 2.0 * std::sqrt (A) * alpha;
                c = { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
                break;
            }
        }
        return c;
    }
}

float  FrequencyResponseGraph::freqToX (double freq) const noexcept
{
    return (float) (getWidth() * std::log (freq / 20.0) / std::log (20000.0 / 20.0));
}

double FrequencyResponseGraph::xToFreq (float x) const noexcept
{
    return 20.0 * std::pow (20000.0 / 20.0, (double) x / getWidth());
}

float  FrequencyResponseGraph::gainToY (double gainDb) const noexcept
{
    return (float) juce::jmap (gainDb, -24.0, 24.0, (double) getHeight(), 0.0);
}

double FrequencyResponseGraph::yToGain (float y) const noexcept
{
    return juce::jmap ((double) y, 0.0, (double) getHeight(), 24.0, -24.0);
}

juce::Rectangle<float> FrequencyResponseGraph::getHandleRect (int i) const noexcept
{
    if (bands[i].freq == nullptr) return {};
    constexpr float size = 14.0f;
    float x = freqToX (bands[i].freq->getValue());
    auto type = getBandType (i);
    bool isPass = (type == Equalizer::BandType::Lowpass || type == Equalizer::BandType::Highpass);
    double yGain = (isPass || bands[i].gain == nullptr) ? 0.0 : bands[i].gain->getValue();
    float y = gainToY (yGain);
    return { x - size * 0.5f, y - size * 0.5f, size, size };
}

int FrequencyResponseGraph::findHandleAt (juce::Point<int> pos) const noexcept
{
    for (int i = 0; i < Equalizer::NumBands; ++i)
        if (bands[i].freq != nullptr
            && getHandleRect (i).expanded (5.0f).contains (pos.toFloat()))
            return i;
    return -1;
}

void FrequencyResponseGraph::updateCurve()
{
    if (bands[0].freq == nullptr) return;

    curvePath.clear();

    const double sr = 44100.0;
    int w = getWidth();
    if (w <= 0) w = 1;

    std::array<BiquadCoeffs, Equalizer::NumBands> coeffs;
    for (int i = 0; i < Equalizer::NumBands; ++i)
    {
        double f = bands[i].freq->getValue();
        double q = bands[i].q != nullptr ? bands[i].q->getValue() : 1.0;
        double g = bands[i].gain != nullptr ? bands[i].gain->getValue() : 0.0;
        coeffs[i] = computeCoeffs (getBandType (i), f, q, g, sr);
    }

    for (int x = 0; x < w; ++x)
    {
        double freq = 20.0 * std::pow (20000.0 / 20.0, x / (double) w);
        double mag = 1.0;
        for (auto& c : coeffs)
            mag *= evalMagnitude (c.b0, c.b1, c.b2, c.a1, c.a2, freq, sr);

        double db = juce::Decibels::gainToDecibels (mag);
        double y = juce::jmap (db, -24.0, 24.0, (double) getHeight(), 0.0);

        if (x == 0) curvePath.startNewSubPath ((float) x, (float) y);
        else        curvePath.lineTo ((float) x, (float) y);
    }

    for (int i = 0; i < Equalizer::NumBands; ++i)
    {
        bandPaths[i].clear();
        const auto& c = coeffs[i];
        for (int x = 0; x < w; ++x)
        {
            double freq = 20.0 * std::pow (20000.0 / 20.0, x / (double) w);
            double mag = evalMagnitude (c.b0, c.b1, c.b2, c.a1, c.a2, freq, sr);
            double db = juce::Decibels::gainToDecibels (mag);
            double y = juce::jmap (db, -24.0, 24.0, (double) getHeight(), 0.0);
            if (x == 0) bandPaths[i].startNewSubPath ((float) x, (float) y);
            else        bandPaths[i].lineTo ((float) x, (float) y);
        }
    }

    repaint();
}

void FrequencyResponseGraph::paint (juce::Graphics& g)
{
    constexpr float corner = 6.0f;
    auto bg = getLocalBounds().toFloat();

    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (bg, corner);

    {
        // Clip everything inside to the rounded background.
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bg, corner);
        g.reduceClipRegion (clip);

        // Major freq grid (100, 1k, 10k)
        g.setColour (juce::Colours::grey);
        for (float freq : { 100.0f, 1000.0f, 10000.0f })
        {
            float x = (float) getWidth() * (std::log (freq / 20.0f) / std::log (20000.0f / 20.0f));
            g.drawVerticalLine ((int) x, 0.0f, (float) getHeight());
        }

        // Minor freq grid
        g.setColour (juce::Colours::darkgrey.darker());
        for (float freq : { 20.0f, 30.f, 40.f, 50.f, 60.f, 70.f, 80.f, 90.f,
                            200.0f, 300.f, 400.f, 500.f, 600.f, 700.f, 800.f, 900.f,
                            2000.0f, 3000.f, 4000.f, 5000.f, 6000.f, 7000.f, 8000.f, 9000.f })
        {
            float x = (float) getWidth() * (std::log (freq / 20.0f) / std::log (20000.0f / 20.0f));
            g.drawVerticalLine ((int) x, 0.0f, (float) getHeight());
        }

        // 0 dB reference line
        g.setColour (juce::Colours::grey);
        {
            float y = juce::jmap (0.0f, -24.0f, 24.0f, (float) getHeight(), 0.0f);
            g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
        }

        // ±6, ±12, ±18 dB grid
        g.setColour (juce::Colours::darkgrey.darker());
        for (float amp : { -18.f, -12.f, -6.f, 6.f, 12.f, 18.f })
        {
            float y = juce::jmap (amp, -24.0f, 24.0f, (float) getHeight(), 0.0f);
            g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
        }

        // Frequency labels at the bottom, just to the right of the major lines.
        g.setFont (juce::Font (10.0f));
        g.setColour (juce::Colours::lightgrey.withAlpha (0.75f));
        const std::pair<float, const char*> freqLabels[] = {
            { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" }
        };
        for (auto& fl : freqLabels)
        {
            float x = (float) getWidth() * (std::log (fl.first / 20.0f) / std::log (20000.0f / 20.0f));
            g.drawText (fl.second, (int) x + 3, getHeight() - 14, 28, 12, juce::Justification::left);
        }

        bool eqOn = (onBtn && onBtn->getToggleState());

        for (int i = 0; i < Equalizer::NumBands; ++i)
        {
            juce::Colour c = eqOn ? bands[i].colour : juce::Colours::grey;
            g.setColour (c.withAlpha (0.5f));
            g.strokePath (bandPaths[i], juce::PathStrokeType (1.0f));
        }

        g.setColour (eqOn ? juce::Colours::cyan : juce::Colours::grey);
        g.strokePath (curvePath, juce::PathStrokeType (2.0f));

        for (int i = 0; i < Equalizer::NumBands; ++i)
        {
            if (bands[i].freq == nullptr) continue;
            auto rect = getHandleRect (i);
            bool hovered = (i == hoveredHandle);
            bool dragged = (i == draggedHandle);

            float alpha = dragged ? 1.0f : (hovered ? 0.9f : 0.65f);
            g.setColour (bands[i].colour.withAlpha (alpha));
            g.fillRect (rect);
            g.setColour (juce::Colours::white.withAlpha (alpha));
            g.drawRect (rect, 1.5f);

            g.setFont (juce::Font (8.0f, juce::Font::bold));
            g.setColour (juce::Colours::black.withAlpha (alpha));
            g.drawFittedText (bands[i].label, rect.toNearestInt(), juce::Justification::centred, 1);
        }
    }

    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (bg.reduced (0.5f), corner, 1.0f);
}

void FrequencyResponseGraph::mouseDown (const juce::MouseEvent& e)
{
    draggedHandle = findHandleAt (e.getPosition());
    repaint();
}

void FrequencyResponseGraph::mouseDrag (const juce::MouseEvent& e)
{
    if (draggedHandle < 0) return;
    auto& h = bands[draggedHandle];

    if (h.freq != nullptr)
    {
        double newFreq = juce::jlimit (h.freq->getMinimum(), h.freq->getMaximum(),
                                       xToFreq ((float) e.x));
        h.freq->setValue (newFreq, juce::sendNotification);
    }

    auto type = getBandType (draggedHandle);
    bool isPass = (type == Equalizer::BandType::Lowpass || type == Equalizer::BandType::Highpass);
    if (h.gain != nullptr && ! isPass)
    {
        double newGain = juce::jlimit (h.gain->getMinimum(), h.gain->getMaximum(),
                                       yToGain ((float) e.y));
        h.gain->setValue (newGain, juce::sendNotification);
    }
}

void FrequencyResponseGraph::mouseUp (const juce::MouseEvent&)
{
    draggedHandle = -1;
    repaint();
}

void FrequencyResponseGraph::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    int h = findHandleAt (e.getPosition());
    if (h < 0) return;
    auto* qSlider = bands[h].q;
    if (qSlider == nullptr) return;
    auto type = getBandType (h);
    if (type == Equalizer::BandType::Lowshelf || type == Equalizer::BandType::Highshelf)
        return; // shelves ignore Q

    double q = qSlider->getValue();
    q = juce::jlimit (qSlider->getMinimum(), qSlider->getMaximum(),
                      q + (double) wheel.deltaY * 1.5);
    qSlider->setValue (q, juce::sendNotification);
}

void FrequencyResponseGraph::mouseMove (const juce::MouseEvent& e)
{
    int h = findHandleAt (e.getPosition());
    if (h != hoveredHandle)
    {
        hoveredHandle = h;
        repaint();
        setMouseCursor (h >= 0 ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor);
    }
}

void FrequencyResponseGraph::mouseExit (const juce::MouseEvent&)
{
    if (hoveredHandle >= 0)
    {
        hoveredHandle = -1;
        repaint();
        setMouseCursor (juce::MouseCursor::NormalCursor);
    }
}

//==============================================================================
// EqualizerComponent Implementation
//==============================================================================

void EqualizerComponent::setSliderColours (juce::Slider& s, juce::Colour c)
{
    s.setColour (juce::Slider::trackColourId, c.darker());
    s.setColour (juce::Slider::thumbColourId, c);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, c.darker (2.0f));
}

bool EqualizerComponent::bandShowsQ (int i) const
{
    int idx = bandType[i].getSelectedItemIndex();
    if (idx < 0) return true;
    auto t = (Equalizer::BandType) idx;
    return t != Equalizer::BandType::Lowshelf && t != Equalizer::BandType::Highshelf;
}

bool EqualizerComponent::bandShowsGain (int i) const
{
    int idx = bandType[i].getSelectedItemIndex();
    if (idx < 0) return true;
    auto t = (Equalizer::BandType) idx;
    return t != Equalizer::BandType::Lowpass && t != Equalizer::BandType::Highpass;
}

void EqualizerComponent::updateBandVisibility (int i)
{
    bandFreq[i].setVisible (true);
    bandQ[i]   .setVisible (bandShowsQ    (i));
    bandGain[i].setVisible (bandShowsGain (i));
}

EqualizerComponent::EqualizerComponent (Equalizer& eq, juce::AudioProcessorValueTreeState& state, const juce::String& prefix)
    : equalizer (eq), apvts (state)
{
    addAndMakeVisible (onButton);
    onButton.setButtonText ("On");
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    onButton.setColour (juce::ToggleButton::tickColourId, juce::Colours::cyan);
    onAtt = std::make_unique<ButtonAttachment> (apvts, prefix + "_EQ_On", onButton);
    onButton.onClick = [this] { responseGraph.updateCurve(); };

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Equalizer", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));

    static const juce::Colour bandColours[Equalizer::NumBands] = {
        juce::Colour::fromRGB (100, 180, 255),
        juce::Colour::fromRGB (255, 220,   0),
        juce::Colour::fromRGB ( 80, 220,  80),
        juce::Colour::fromRGB (255, 140,   0),
        juce::Colour::fromRGB (255,  80, 180)
    };
    static const char* bandLabels[Equalizer::NumBands] = { "L", "1", "2", "3", "H" };

    juce::Colour color = juce::Colours::cyan;
    auto typeNames = Equalizer::getBandTypeNames();

    auto setupSlider = [&](fxme::FxmeSlider& s, double mn, double mx, double def, const juce::String& tip) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange (mn, mx);
        s.setValue (def);
        s.setTooltip (tip);
        s.setName (tip);
        s.setShowLabel (true);
        s.setLookAndFeel (&fxmeLookAndFeel);
        setSliderColours (s, color);
        s.onValueChange = [this] { responseGraph.updateCurve(); };
    };

    for (int i = 0; i < Equalizer::NumBands; ++i)
    {
        const auto& cfg = Equalizer::getBandConfig (i);
        juce::String pid = prefix + "_EQ_" + cfg.suffix;

        addAndMakeVisible (bandType[i]);
        bandType[i].addItemList (typeNames, 1);
        bandType[i].setLookAndFeel (&fxmeLookAndFeel);
        bandTypeAtt[i] = std::make_unique<ComboBoxAttachment> (apvts, pid + "_Type", bandType[i]);
        bandType[i].onChange = [this, i] {
            updateBandVisibility (i);
            resized();
            responseGraph.updateCurve();
        };

        setupSlider (bandFreq[i], cfg.minFreq, cfg.maxFreq, cfg.defFreq,
                     juce::String (cfg.suffix) + " Freq");
        bandFreq[i].setName ("Freq");
        bandFreq[i].setAttachment (new SliderAttachment (apvts, pid + "_Freq", bandFreq[i]));

        setupSlider (bandQ[i], 0.1, 10.0, 1.0, juce::String (cfg.suffix) + " Q");
        bandQ[i].setName ("Q");
        bandQ[i].setAttachment (new SliderAttachment (apvts, pid + "_Q", bandQ[i]));

        setupSlider (bandGain[i], -24.0, 24.0, 0.0, juce::String (cfg.suffix) + " Gain");
        bandGain[i].setName ("Gain");
        bandGain[i].setAttachment (new SliderAttachment (apvts, pid + "_Gain", bandGain[i]));

        updateBandVisibility (i);
    }

    addAndMakeVisible (postGainSlider);
    postGainSlider.setSliderStyle (juce::Slider::LinearBarVertical);
    postGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 15);
    postGainSlider.setRange (-24.0, 24.0, 0.1);
    postGainSlider.setValue (0.0);
    postGainSlider.setTextValueSuffix ("dB");
    postGainSlider.setTooltip ("Post Gain");
    postGainSlider.setLookAndFeel (&fxmeLookAndFeel);
    setSliderColours (postGainSlider, color);
    postGainSlider.onValueChange = [this] { responseGraph.updateCurve(); };
    postGainSlider.setAttachment (new SliderAttachment (apvts, prefix + "_EQ_PostGain", postGainSlider));

    addAndMakeVisible (responseGraph);

    std::array<FrequencyResponseGraph::BandRefs, Equalizer::NumBands> refs;
    for (int i = 0; i < Equalizer::NumBands; ++i)
        refs[i] = { &bandType[i], &bandFreq[i], &bandQ[i], &bandGain[i],
                    bandColours[i], bandLabels[i] };
    responseGraph.setReferences (refs, postGainSlider, onButton);
}

EqualizerComponent::~EqualizerComponent()
{
}

void EqualizerComponent::paint (juce::Graphics& g)
{
    auto diagonale = (getLocalBounds().getTopLeft() - getLocalBounds().getBottomRight()).toFloat();
    auto length = diagonale.getDistanceFromOrigin();
    auto perpendicular = diagonale.rotatedAboutOrigin (juce::degreesToRadians (270.0f)) / length;
    auto height = float (getWidth() * getHeight()) / length;
    auto bluegreengrey = juce::Colours::cyan.darker (3.f);
    juce::ColourGradient grad (bluegreengrey.darker().darker().darker(), perpendicular * height,
                               bluegreengrey, perpendicular * -height, false);
    g.setGradientFill (grad);
    g.fillAll();
}

void EqualizerComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    using fi = juce::FlexItem;

    juce::FlexBox fmain;
    fmain.flexDirection = juce::FlexBox::Direction::column;

    juce::FlexBox ftop;
    ftop.flexDirection = juce::FlexBox::Direction::row;
    ftop.items.add (fi (onButton).withFlex (0.2f));
    ftop.items.add (fi (titleLabel).withFlex (1.5f));

    juce::FlexBox fmiddle;
    fmiddle.flexDirection = juce::FlexBox::Direction::row;

    juce::FlexBox columns[Equalizer::NumBands];
    for (int i = 0; i < Equalizer::NumBands; ++i)
    {
        columns[i].flexDirection = juce::FlexBox::Direction::column;
        columns[i].items.add (fi (bandType[i]).withFlex (0.35f).withMargin (juce::FlexItem::Margin (2.f)));
        // All three slider rows are always added so freq / Q / gain stay at the
        // same vertical position across columns; setVisible(false) blanks the
        // ones that don't apply to the current band type.
        columns[i].items.add (fi (bandFreq[i]).withFlex (1.f));
        columns[i].items.add (fi (bandQ[i])   .withFlex (1.f));
        columns[i].items.add (fi (bandGain[i]).withFlex (1.f));
        fmiddle.items.add (fi (columns[i]).withFlex (1.f));
    }
    // Output post-gain bar moved to the right of the band controls.
    fmiddle.items.add (fi (postGainSlider).withFlex (0.25f)
                                          .withMargin (juce::FlexItem::Margin (0.f, 10.f, 0.f, 15.f)));

    juce::FlexBox fbottom;
    fbottom.flexDirection = juce::FlexBox::Direction::row;
    fbottom.items.add (fi (responseGraph).withFlex (1.f));

    fmain.items.add (fi (ftop).withFlex (0.13f).withMargin (juce::FlexItem::Margin (5.f, 0.f, 10.f, 0.f)));
    fmain.items.add (fi (fmiddle).withFlex (1.f));
    fmain.items.add (fi (fbottom).withFlex (0.85f).withMargin (juce::FlexItem::Margin (10.f, 0.f, 0.f, 0.f)));

    fmain.performLayout (area);
}
