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
class FrequencyResponseGraph : public juce::Component,
                               private juce::Timer
{
public:
    FrequencyResponseGraph() = default;

    /** Enables the live input/output spectrum overlay. Pass the DSP taps and a
        provider for the current sample rate; call with nullptrs to disable. */
    void setSpectrumTaps (fxme::SpectrumTap* input, fxme::SpectrumTap* output,
                          std::function<double()> sampleRateProvider);

    /** Per-band references the graph reads to compute the response. */
    struct BandRefs
    {
        juce::ToggleButton* on   = nullptr;
        juce::ComboBox* type = nullptr;
        juce::Slider*   freq = nullptr;
        juce::Slider*   q    = nullptr;
        juce::Slider*   gain = nullptr;
        juce::Colour    colour;
        juce::String    label;
    };

    void setReferences (std::array<BandRefs, Equalizer::MaxBands> bands,
                        int numActiveBands,
                        juce::Slider& postG,
                        juce::ToggleButton& onB);

    void paint (juce::Graphics& g) override;
    void updateCurve();

    void mouseDown      (const juce::MouseEvent& e) override;
    void mouseDrag      (const juce::MouseEvent& e) override;
    void mouseUp        (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseMove      (const juce::MouseEvent& e) override;
    void mouseExit      (const juce::MouseEvent& e) override;

private:
    std::array<BandRefs, Equalizer::MaxBands> bands;
    int                 numBands = 0;
    juce::Slider*       postGain = nullptr;
    juce::ToggleButton* onBtn    = nullptr;

    juce::Path                                          curvePath;
    std::array<juce::Path, Equalizer::MaxBands>         bandPaths;

    int draggedHandle = -1;
    int hoveredHandle = -1;

    // --- Live spectrum overlay (optional) ---
    void timerCallback() override;
    void drawSpectrum (juce::Graphics& g,
                       const std::array<float, fxme::SpectrumAnalyzer::numPoints>& db,
                       juce::Colour colour, float thickness, bool fill) const;

    fxme::SpectrumTap*       inputTap  = nullptr;
    fxme::SpectrumTap*       outputTap = nullptr;
    std::function<double()>  sampleRateProvider;
    fxme::SpectrumAnalyzer   analyzer;
    std::array<float, fxme::SpectrumAnalyzer::numPoints> inputDb  { };
    std::array<float, fxme::SpectrumAnalyzer::numPoints> outputDb { };

    // Fixed dB window the overlaid spectrum is mapped into (independent of the
    // ±24 dB EQ-gain grid; the spectrum is a background reference).
    static constexpr float spectrumMaxDb =   0.0f;
    static constexpr float spectrumMinDb = -90.0f;
    float spectrumDbToY (float db) const noexcept;

    Equalizer::BandType getBandType (int i) const noexcept;
    bool bandIsOn (int i) const noexcept;

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
                        const juce::String& prefix,
                        bool showTitle = true);
    ~EqualizerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    Equalizer& equalizer;
    juce::AudioProcessorValueTreeState& apvts;
    const int numBands;

    juce::ToggleButton onButton;
    juce::Label        titleLabel;

    std::array<juce::ToggleButton, Equalizer::MaxBands> bandOnButton;
    std::array<juce::ComboBox,     Equalizer::MaxBands> bandType;
    std::array<fxme::FxmeSlider,   Equalizer::MaxBands> bandFreq;
    std::array<fxme::FxmeSlider,   Equalizer::MaxBands> bandQ;
    std::array<fxme::FxmeSlider,   Equalizer::MaxBands> bandGain;
    std::array<juce::Label,        Equalizer::MaxBands> bandTypeLabel; // unused placeholder, kept for future
    fxme::FxmeSlider postGainSlider;

    FrequencyResponseGraph responseGraph;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment> onAtt;
    std::array<std::unique_ptr<ButtonAttachment>,   Equalizer::MaxBands> bandOnAtt;
    std::array<std::unique_ptr<ComboBoxAttachment>, Equalizer::MaxBands> bandTypeAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    void setSliderColours (juce::Slider& s, juce::Colour c);
    void updateBandVisibility (int bandIndex);
    bool bandShowsQ    (int bandIndex) const;
    bool bandShowsGain (int bandIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqualizerComponent)
};
