/*
  ==============================================================================

    TubeComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Tube.h"

/**
 * @class TubeComponent
 * @brief GUI component for controlling the Tube saturation effect.
 */
class TubeComponent : public juce::Component
{
public:
    /**
     * @brief Constructor.
     * @param tube The Tube effect instance.
     * @param apvts The APVTS.
     * @param prefix The parameter ID prefix.
     * @param showTitle Whether to show the "Tube Saturation" title label.
     * @param knobsInSingleRow Lays the four knobs out in one row instead of the
     *        default 2x2 grid, and tightens the header margins. For hosts that
     *        give the component a wide but short slot (an effect tab in a mixer
     *        strip, say), where halving the height makes the knobs tiny.
     *        Leave false for roughly square slots — the standalone plugin and
     *        every other host keep the original layout.
     */
    TubeComponent (Tube& tube, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, bool showTitle = true, bool knobsInSingleRow = false);
    ~TubeComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Tube& tube;
    juce::AudioProcessorValueTreeState& apvts;
    const bool knobsInSingleRow;

    fxme::FxmeButton onButton;
    juce::ComboBox modelBox;
    juce::Label titleLabel;
    juce::ImageComponent tubeImage;
    juce::Label driveLabel, biasLabel, toneLabel, sagLabel, outLabel;
    fxme::FxmeSlider driveSlider, biasSlider, toneSlider, sagSlider;
    fxme::FxmeSlider outSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;

    void setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeComponent)
};