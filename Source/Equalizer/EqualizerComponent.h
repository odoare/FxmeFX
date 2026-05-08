/*
  ==============================================================================

    EqualizerComponent.h

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <array>
#include "Equalizer.h"

/**
 * @class FrequencyResponseGraph
 * @brief Component that draws the frequency response curve of the equalizer
 *        and per-band response curves with draggable handles.
 */
class FrequencyResponseGraph : public juce::Component
{
public:
    FrequencyResponseGraph() = default;

    /** Per-band references the graph reads to compute the response. */
    struct BandRefs
    {
        juce::ComboBox* type = nullptr;
        juce::Slider*   freq = nullptr;
        juce::Slider*   q    = nullptr;
        juce::Slider*   gain = nullptr;
        juce::Colour    colour;
        const char*     label = "";
    };

    void setReferences (std::array<BandRefs, Equalizer::NumBands> bands,
                        juce::Slider& postG,
                        juce::ToggleButton& onB);

    void paint (juce::Graphics& g) override;
    void updateCurve();

    void mouseDown      (const juce::MouseEvent& e) override;
    void mouseDrag      (const juce::MouseEvent& e) override;
    void mouseUp        (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMove      (const juce::MouseEvent& e) override;
    void mouseExit      (const juce::MouseEvent& e) override;

private:
    std::array<BandRefs, Equalizer::NumBands> bands;
    juce::Slider*       postGain = nullptr;
    juce::ToggleButton* onBtn    = nullptr;

    juce::Path                                          curvePath;
    std::array<juce::Path, Equalizer::NumBands>         bandPaths;

    int draggedHandle = -1;
    int hoveredHandle = -1;

    Equalizer::BandType getBandType (int i) const noexcept;

    float freqToX (double freq)  const noexcept;
    double xToFreq (float x)     const noexcept;
    float gainToY (double gainDb) const noexcept;
    double yToGain (float y)     const noexcept;
    juce::Rectangle<float> getHandleRect (int index) const noexcept;
    int findHandleAt (juce::Point<int> pos) const noexcept;
};

/**
 * @class EqualizerComponent
 * @brief GUI component for controlling the Equalizer.
 */
class EqualizerComponent : public juce::Component
{
public:
    EqualizerComponent (Equalizer& equalizerToControl,
                        juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& prefix);
    ~EqualizerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Equalizer& equalizer;
    juce::AudioProcessorValueTreeState& apvts;

    juce::ToggleButton onButton;
    juce::Label        titleLabel;

    std::array<juce::ComboBox,     Equalizer::NumBands> bandType;
    std::array<fxme::FxmeSlider,   Equalizer::NumBands> bandFreq;
    std::array<fxme::FxmeSlider,   Equalizer::NumBands> bandQ;
    std::array<fxme::FxmeSlider,   Equalizer::NumBands> bandGain;
    std::array<juce::Label,        Equalizer::NumBands> bandTypeLabel; // unused placeholder, kept for future
    fxme::FxmeSlider postGainSlider;

    FrequencyResponseGraph responseGraph;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;
    std::array<std::unique_ptr<ComboBoxAttachment>, Equalizer::NumBands> bandTypeAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setSliderColours (juce::Slider& s, juce::Colour c);
    void updateBandVisibility (int bandIndex);
    bool bandShowsQ    (int bandIndex) const;
    bool bandShowsGain (int bandIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizerComponent)
};
