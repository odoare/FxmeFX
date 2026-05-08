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
     */
    TubeComponent (Tube& tube, juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix);
    ~TubeComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Tube& tube;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::ComboBox modelBox;
    juce::Label titleLabel;
    juce::ImageComponent tubeImage;
    juce::Label driveLabel, biasLabel, toneLabel, sagLabel, outLabel;
    fxme::FxmeSlider driveSlider, biasSlider, toneSlider, sagSlider;
    fxme::FxmeSlider outSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;

    void setupSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setupBarSlider (fxme::FxmeSlider& slider, juce::Label& label, const juce::String& text, double min, double max, double def);
    void setSliderColours (juce::Slider& s, juce::Colour c);

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TubeComponent)
};