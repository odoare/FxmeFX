/*
  ==============================================================================

    NeuralAmpComponent.h

    The NeuralAmp's controls. Two layouts:

      standard   the On switch, the title, Channels, Calibrate and Normalize
                 on top; the model panel (its name, metadata, a CPU hint, the
                 status) over the models browser, Load and Clear; two rows of
                 knobs, the drive side (Input, Gate with its LED, Level) over
                 the tone stack (Bass, Mid, Treble); input and output VU
                 meters, Mix and Output faders on the right. For the
                 plugin's own editor.

    Level is greyed out when it cannot act: Calibrate off, or a model that
    does not say at what level it was captured. In an FxmeNet mirror the
    status is the server's (NeuralAmp::ModelStatus::remote).
      compact    no title, no meters, flat knobs: the model line over one
                 row of seven knobs (Mix and Output included), for a host
                 that gives the effect a wide and short slot.

    A model loads from the browser (the .nam and .json files under
    FX-Mechanics/Models/NAM, rescanned each time it opens), from Load (a file
    chooser), or by dropping a file anywhere on the component.

    Loading goes through onLoadFile / onClear when set (a plugin's processor,
    to tell FxmeNet), else straight to the effect.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Common/Theme.h"
#include "NeuralAmp.h"

class NeuralAmpComponent : public juce::Component,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    NeuralAmpComponent (NeuralAmp& ampToControl,
                        juce::AudioProcessorValueTreeState& apvts,
                        const juce::String& prefix,
                        bool showTitle = true,
                        bool compact = false);
    ~NeuralAmpComponent() override;

    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /** Set by a plugin to go through its processor; default: the effect. */
    std::function<bool (const juce::File&, juce::String&)> onLoadFile;
    std::function<void()> onClear;

    static juce::Colour tint() { return fxmefx::theme::neuralAmp; }

private:
    static constexpr float kKnobRowHeight = 96.0f;
    static constexpr float kVuZeroDbfs = -18.0f;

    /** The model's name, its metadata, the CPU hint, the status. */
    class ModelPanel : public juce::Component
    {
    public:
        void setStatus (const NeuralAmp::ModelStatus& s, const juce::String& localError);
        void paint (juce::Graphics& g) override;
        juce::Colour accent { juce::Colours::orange };
        bool compact = false;
    private:
        NeuralAmp::ModelStatus status;
        juce::String localError;
    };

    /** Open (green), hold (amber), closed (dim red), or off (grey). */
    class GateLed : public juce::Component
    {
    public:
        void setState (int newState);
        void paint (juce::Graphics& g) override;
    private:
        int state = -1;
    };

    /** The browser: rescans the models folder each time it opens. */
    class ModelBox : public juce::ComboBox
    {
    public:
        std::function<void()> beforePopup;
        void showPopup() override
        {
            if (beforePopup)
                beforePopup();
            juce::ComboBox::showPopup();
        }
    };

    void timerCallback() override;
    void setupRotary (fxme::FxmeSlider& slider, const juce::String& text, const juce::String& suffix,
                      const juce::String& tooltip);
    void rescanModels();
    void loadFile (const juce::File& f);
    /** A model bundled as binary data (FxmeNamModels), by index. */
    void loadEmbedded (int index);
    static constexpr int embeddedBaseId = 100000;   // above the browsed files' ids
    void chooseFile();
    void clear();
    void refreshStatus();
    void updateLevelEnablement (const NeuralAmp::ModelStatus& s);

    NeuralAmp& amp;
    juce::AudioProcessorValueTreeState& apvts;
    const bool compact;

    juce::Label titleLabel;
    fxme::FxmeButton onButton, calibButton, normButton;
    juce::ComboBox chanBox;
    ModelPanel modelPanel;
    ModelBox modelBox;
    juce::TextButton loadButton { "Load..." }, clearButton { "Clear" };
    fxme::FxmeSlider inSlider, gateSlider, bassSlider, midSlider, trebleSlider, levelSlider, mixSlider, outSlider;
    GateLed gateLed;
    fxme::VuMeterComponent inVu, outVu;
    juce::Label inLabel, outLabel;

    juce::Array<juce::File> browsed;          // the browser's files, by item id - 2
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String localError;                  // a file that was not a model
    int seenGeneration = -1, seenModelState = -1;
    std::atomic<float>* calibParam = nullptr;
    bool lastCalib = true;
    bool dragOver = false;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> chanAtt;

    fxme::FxmeLookAndFeel fxmeLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralAmpComponent)
};
