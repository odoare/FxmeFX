/*
  ==============================================================================

    CrusherComponent.cpp

  ==============================================================================
*/

#include "CrusherComponent.h"
#include "../Common/TopBar.h"
#include "../Common/Controls.h"

void CrusherComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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

CrusherComponent::CrusherComponent (Crusher& c, juce::AudioProcessorValueTreeState& state,
                                    const juce::String& prefix, bool showTitle, bool compactLayout)
    : crusher (c), apvts (state), compact (compactLayout),
      onButton     (state, prefix + "_Crush_On",     "On",     tint()),
      cleanButton  (state, prefix + "_Crush_Clean",  "Clean",  tint()),
      smoothButton (state, prefix + "_Crush_Smooth", "Smooth", tint())
{
    // Tints this component's combo-box drop-downs; a menu is its own window
    // and cannot see the box that opened it.
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Crusher", juce::NotificationType::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    for (auto* b : { &onButton, &cleanButton, &smoothButton })
    {
        addAndMakeVisible (*b);
        b->setLookAndFeel (&fxmeLookAndFeel);
    }
    cleanButton.button.setTooltip ("Clean. \n A steep low-pass before the sample-rate reduction, so the highs are cut "
                                   "instead of folding back down as aliasing. Off (dirty), the aliasing is the sound.");
    smoothButton.button.setTooltip ("Smooth. \n A low-pass after the sample-rate reduction (an old sampler's "
                                    "reconstruction filter): rounds the steps off, for a duller, warmer result.");

    addAndMakeVisible (modeBox);
    modeBox.setLookAndFeel (&fxmeLookAndFeel);
    modeBox.setColour (juce::ComboBox::outlineColourId, tint().darker());
    modeBox.setColour (juce::ComboBox::arrowColourId,   tint().brighter (0.3f));
    modeBox.addItem ("Mid-tread", 1);
    modeBox.addItem ("Mid-rise",  2);
    modeBox.addItem ("Mu-law",    3);
    modeBox.setTooltip ("Quantizer. \n Mid-tread: silence stays silent. Mid-rise: 0 sits between two levels, so even "
                        "a quiet signal buzzes. Mu-law: fine steps near 0, coarse ones near full scale (telephone, "
                        "early samplers).");
    modeAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_Crush_Mode", modeBox);

    setupRotary (driveSlider, "Drive", " dB",
                 "Drive. \n Gain into the converter, which clips at full scale.");
    setupRotary (bitsSlider, "Bits", "",
                 "Bits. \n The bit depth, 1 to 24 (fractional values glide between depths).");
    setupRotary (rateSlider, "Rate", "",
                 "Rate. \n The sample rate it is reduced to. At the top of the range (Off) the rate is not reduced.");
    setupRotary (jitterSlider, "Jitter", " %",
                 "Jitter. \n Randomness on the length of each held sample, which blurs the fixed aliasing "
                 "pattern into something closer to noise.");
    setupRotary (noiseSlider, "Noise", " %",
                 "Noise. \n Dither before the quantizer (up to one step either way): the distortion of a low "
                 "bit depth turns into hiss.");
    bitsSlider.setNumDecimalPlacesToDisplay (1);

    if (compact)
    {
        setupRotary (mixSlider, "Mix", " %", "Mix. \n Balance between the dry and the crushed signal.");
        setupRotary (outSlider, "Output", " dB", "Output level.");
    }
    else
    {
        fxmefx::setupFader (*this, mixSlider, "Mix", tint(), fxmeLookAndFeel);
        mixSlider.setTextValueSuffix (" %");
        mixSlider.setNumDecimalPlacesToDisplay (0);
        mixSlider.setTooltip ("Mix. \n Balance between the dry and the crushed signal.");
        fxmefx::setupGainFader (*this, outSlider, "Output", tint(), fxmeLookAndFeel);
        outSlider.setTooltip ("Output level.");
    }
    outSlider.setCentralValue (0.0);

    auto attach = [&] (fxme::FxmeSlider& s, const char* id)
    {
        s.setAttachment (new SliderAttachment (apvts, prefix + "_Crush_" + id, s));
    };
    attach (driveSlider,  "Drive");
    attach (bitsSlider,   "Bits");
    attach (rateSlider,   "Rate");
    attach (jitterSlider, "Jitter");
    attach (noiseSlider,  "Noise");
    attach (mixSlider,    "Mix");
    attach (outSlider,    "Out");
    rateSlider.setLogarithmic();

    // The steps, input against output. Not in the compact layout: a short
    // slot leaves it no height.
    if (! compact)
    {
        crusher.getInputTap() .setEnabled (true);
        crusher.getOutputTap().setEnabled (true);
        waveform.setTaps ({ &crusher.getInputTap(), &crusher.getOutputTap() });
        waveform.setChannelColours ({ juce::Colour (0xff8a8a96), tint() });
        waveform.setChannelNames ({ "In", "Out" });
        waveform.setAmplitudeRange (1.0f);
        addAndMakeVisible (waveform);
    }
}

CrusherComponent::~CrusherComponent()
{
    if (! compact)
    {
        crusher.getInputTap() .setEnabled (false);
        crusher.getOutputTap().setEnabled (false);
    }
}

void CrusherComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void CrusherComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, knobs, left, body, fMain;
    top.flexDirection   = juce::FlexBox::Direction::row;
    knobs.flexDirection = juce::FlexBox::Direction::row;
    left.flexDirection  = juce::FlexBox::Direction::column;
    body.flexDirection  = juce::FlexBox::Direction::row;
    fMain.flexDirection = juce::FlexBox::Direction::column;

    top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
    top.items.add (fi (titleLabel).withFlex (1.0f));
    top.items.add (fi (cleanButton).withWidth (76.0f));
    top.items.add (fi (smoothButton).withWidth (84.0f));
    top.items.add (fi (modeBox).withWidth (120.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

    if (compact)
    {
        for (auto* s : { &driveSlider, &bitsSlider, &rateSlider, &jitterSlider, &noiseSlider, &mixSlider, &outSlider })
            knobs.items.add (fi (*s).withFlex (1.0f));

        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 4.f, 0.f)));
        fMain.items.add (fi (knobs).withFlex (1.0f));
        fMain.performLayout (area);
        return;
    }

    for (auto* s : { &driveSlider, &bitsSlider, &rateSlider, &jitterSlider, &noiseSlider })
        knobs.items.add (fi (*s).withFlex (1.0f));
    // The knobs at a fixed, modest height; the waveform takes the rest.
    left.items.add (fi (knobs).withHeight (kKnobRowHeight));
    left.items.add (fi (waveform).withFlex (1.0f).withMargin (juce::FlexItem::Margin (6.f, 0.f, 0.f, 0.f)));

    body.items.add (fi (left).withFlex (1.0f));
    body.items.add (fi (mixSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 10.f)));
    body.items.add (fi (outSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 5.f)));

    fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                             .withMinHeight (fxmefx::kHeaderRowHeight)
                             .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
    fMain.items.add (fi (body).withFlex (1.0f));
    fMain.performLayout (area);
}
