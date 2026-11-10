/*
  ==============================================================================

    NeuralAmpComponent.cpp

  ==============================================================================
*/

#include "NeuralAmpComponent.h"
#include "../Common/TopBar.h"
#include "../Common/Controls.h"

//==============================================================================
// ModelPanel

void NeuralAmpComponent::ModelPanel::setStatus (const NeuralAmp::ModelStatus& s, const juce::String& error)
{
    status = s;
    localError = error;
    repaint();
}

void NeuralAmpComponent::ModelPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0e0d14));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colour (0xff3a3a4c));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    auto area = r.reduced (8.0f, compact ? 1.0f : 5.0f).toNearestInt();

    const auto& info = status.info;
    juce::String name = juce::String (info.displayName());
    if (name.isEmpty())
        name = juce::File::createFileWithoutCheckingPath (status.fileName).getFileNameWithoutExtension();

    // The details: architecture, rate, loudness, author, gear.
    juce::StringArray details;
    if (status.hasModel)
    {
        juce::String arch = info.sizeClass.empty() ? juce::String (info.architecture) : juce::String (info.sizeClass);
        if (arch.isNotEmpty())
            details.add (arch);
        const double khz = info.sampleRate / 1000.0;
        details.add (juce::String (khz, std::abs (khz - std::round (khz)) > 0.01 ? 1 : 0) + " kHz");
        if (info.loudnessDb)
            details.add ("loudness " + juce::String (*info.loudnessDb, 1) + " dB");
        if (info.inputLevelDbu)
            details.add ("captured at " + juce::String (*info.inputLevelDbu, 1) + " dBu");
        else
            details.add ("no capture level");
        if (! info.modeledBy.empty())
            details.add ("by " + juce::String (info.modeledBy));
        if (! info.gearType.empty())
            details.add (juce::String (info.gearType));
        if (! info.toneType.empty())
            details.add (juce::String (info.toneType));
    }

    // The status line.
    juce::String state;
    juce::Colour stateColour (0xff9a9aa8);
    if (localError.isNotEmpty())
    {
        state = localError;
        stateColour = juce::Colour (0xffe05a50);
    }
    else if (status.error.isNotEmpty())
    {
        state = status.error;
        stateColour = juce::Colour (0xffe05a50);
    }
    else if (status.loading)
    {
        state = status.remote ? "Loading on the FxmeNet server..." : "Loading...";
        stateColour = accent;
    }
    else if (status.ready)
    {
        static const char* const cpu[] { "light", "medium", "heavy" };
        const int c = info.cpuClass();
        state = status.remote ? "Playing on the FxmeNet server" : "Playing";
        if (c >= 0)
            state << "  |  CPU: " << cpu[c];
        if (! info.backend.empty())
            state << "  |  " << juce::String (info.backend);
        stateColour = accent;
    }
    else if (status.hasModel)
        state = status.remote ? "Plays on the FxmeNet server" : "Not loaded";

    if (! status.hasModel)
    {
        g.setColour (juce::Colour (0xff9a9aa8));
        g.setFont (juce::FontOptions (compact ? 12.0f : 14.0f));
        g.drawText (localError.isNotEmpty() ? localError
                                            : juce::String ("No model: drop a .nam file here, or Load..."),
                    area, juce::Justification::centredLeft, true);
        return;
    }

    if (compact)
    {
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        const int w = juce::jmin (area.getWidth() / 2, juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), name) + 12);
        g.drawText (name, area.removeFromLeft (w), juce::Justification::centredLeft, true);
        g.setColour (stateColour);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (state, area, juce::Justification::centredLeft, true);
        return;
    }

    const int lineH = area.getHeight() / 3;
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    g.drawText (name, area.removeFromTop (lineH), juce::Justification::centredLeft, true);
    g.setColour (juce::Colour (0xffb8b8c4));
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (details.joinIntoString ("  |  "), area.removeFromTop (lineH), juce::Justification::centredLeft, true);
    g.setColour (stateColour);
    g.drawText (state, area, juce::Justification::centredLeft, true);
}

//==============================================================================
// GateLed

void NeuralAmpComponent::GateLed::setState (int newState)
{
    if (newState != state)
    {
        state = newState;
        repaint();
    }
}

void NeuralAmpComponent::GateLed::paint (juce::Graphics& g)
{
    // -1 off, then GateStateMachine::State: 0 closed, 1 open, 2 hold.
    static const juce::Colour colours[] { juce::Colour (0xffc8443c), juce::Colour (0xff4cd06a), juce::Colour (0xffe8b040) };
    auto led = getLocalBounds().toFloat().reduced (2.0f);
    const float d = juce::jmin (led.getWidth(), led.getHeight());
    led = led.withSizeKeepingCentre (d, d);

    juce::Colour c = state < 0 ? juce::Colour (0xff4a4a54) : colours[juce::jlimit (0, 2, state)];
    if (state == 0)
        c = c.withMultipliedBrightness (0.6f);
    if (state > 0)
    {
        g.setColour (c.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.0f));
    }
    g.setColour (c);
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (led, 1.0f);
}

//==============================================================================
// NeuralAmpComponent

void NeuralAmpComponent::setupRotary (fxme::FxmeSlider& slider, const juce::String& text,
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

NeuralAmpComponent::NeuralAmpComponent (NeuralAmp& a, juce::AudioProcessorValueTreeState& state,
                                        const juce::String& prefix, bool showTitle, bool compactLayout)
    : amp (a), apvts (state), compact (compactLayout),
      onButton    (state, prefix + "_NAmp_On",    "On",        tint()),
      calibButton (state, prefix + "_NAmp_Calib", "Calibrate", tint()),
      normButton  (state, prefix + "_NAmp_Norm",  "Normalize", tint())
{
    fxmeLookAndFeel.setAccentColour (tint());
    if (compact)
        fxmeLookAndFeel.setKnobStyle (fxme::FxmeLookAndFeel::KnobStyle::flat);

    addChildComponent (titleLabel);
    titleLabel.setVisible (showTitle && ! compact);
    titleLabel.setText ("Neural Amp", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));

    addAndMakeVisible (onButton);
    onButton.setLookAndFeel (&fxmeLookAndFeel);
    for (auto* b : { &calibButton, &normButton })
    {
        addChildComponent (*b);
        b->setVisible (! compact);
        b->setLookAndFeel (&fxmeLookAndFeel);
    }
    calibButton.button.setTooltip ("Calibrate. \n When the model says at what level it was captured, "
                                   "match it from Level (your interface's full scale in dBu), so the "
                                   "model is driven as the real amp was.");
    normButton.button.setTooltip ("Normalize. \n Bring every model to the same loudness (-18 dB), "
                                  "so switching models does not jump in level.");

    addAndMakeVisible (chanBox);
    chanBox.setLookAndFeel (&fxmeLookAndFeel);
    chanBox.setColour (juce::ComboBox::outlineColourId, tint().darker());
    chanBox.setColour (juce::ComboBox::arrowColourId, tint().brighter (0.3f));
    chanBox.addItemList ({ "Dual mono", "Mono" }, 1);
    chanBox.setTooltip ("Channels. \n Mono: one model on the sum of the two sides, the same on both "
                        "(a guitar amp, half the CPU). Dual mono: one model per side, the image kept.");
    chanAtt = std::make_unique<ComboBoxAttachment> (apvts, prefix + "_NAmp_Chan", chanBox);

    modelPanel.accent = tint();
    modelPanel.compact = compact;
    addAndMakeVisible (modelPanel);

    addAndMakeVisible (modelBox);
    modelBox.setLookAndFeel (&fxmeLookAndFeel);
    modelBox.setColour (juce::ComboBox::outlineColourId, tint().darker());
    modelBox.setColour (juce::ComboBox::arrowColourId, tint().brighter (0.3f));
    modelBox.setTextWhenNothingSelected ("Models folder");
    modelBox.setTooltip ("The models in FX-Mechanics/Models/NAM (subfolders included). "
                         "Put your .nam files there.");
    modelBox.beforePopup = [this] { rescanModels(); };
    modelBox.onChange = [this]
    {
        const int id = modelBox.getSelectedId();
        modelBox.setSelectedId (0, juce::dontSendNotification);
        if (id == 1)
        {
            const auto folder = NeuralAmp::getModelsFolder();
            folder.createDirectory();
            folder.startAsProcess();
        }
        else if (id >= 2 && id - 2 < browsed.size())
            loadFile (browsed[id - 2]);
    };
    rescanModels();

    for (auto* b : { &loadButton, &clearButton })
    {
        addAndMakeVisible (*b);
        b->setLookAndFeel (&fxmeLookAndFeel);
        b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff24232e));
        b->setColour (juce::ComboBox::outlineColourId, tint().darker());
    }
    loadButton.setTooltip ("Load a model file (.nam, or a Keras .json). It is kept in the session and "
                           "in presets: the file is not needed afterwards. You can also drop a file here.");
    clearButton.setTooltip ("Remove the model: the signal goes through dry (with the tone stack).");
    loadButton.onClick = [this] { chooseFile(); };
    clearButton.onClick = [this] { clear(); };

    setupRotary (inSlider, "Input", " dB", "Input. \n The drive into the model.");
    setupRotary (gateSlider, "Gate", "",
                 "Gate. \n A noise gate before the model, its threshold: a high-gain model "
                 "amplifies the noise floor. Off at the bottom.");
    setupRotary (bassSlider, "Bass", " dB", "Bass. \n A low shelf at 150 Hz, after the model.");
    setupRotary (midSlider, "Mid", " dB", "Mid. \n A peak at 425 Hz, after the model.");
    setupRotary (trebleSlider, "Treble", " dB", "Treble. \n A high shelf at 1.8 kHz, after the model.");
    if (! compact)
        setupRotary (levelSlider, "Level", " dBu",
                     "Level. \n Your interface's full scale in dBu (the level a 0 dBFS input "
                     "stands for, in its manual; 12 dBu when unsure). With Calibrate on, the input "
                     "is brought to the level the model was captured at. Only models that give "
                     "that level (\"captured at\" in the panel) use it: greyed out otherwise.");

    if (compact)
    {
        setupRotary (mixSlider, "Mix", " %", "Mix. \n Balance between the dry signal and the amp.");
        setupRotary (outSlider, "Output", " dB", "Output level.");
    }
    else
    {
        fxmefx::setupFader (*this, mixSlider, "Mix", tint(), fxmeLookAndFeel);
        mixSlider.setTextValueSuffix (" %");
        mixSlider.setNumDecimalPlacesToDisplay (0);
        mixSlider.setTooltip ("Mix. \n Balance between the dry signal and the amp.");
        fxmefx::setupGainFader (*this, outSlider, "Output", tint(), fxmeLookAndFeel);
        outSlider.setTooltip ("Output level.");
    }
    for (auto* s : { &inSlider, &bassSlider, &midSlider, &trebleSlider, &outSlider })
        s->setCentralValue (0.0);

    auto attach = [&] (fxme::FxmeSlider& s, const char* id)
    {
        s.setAttachment (new SliderAttachment (apvts, prefix + "_NAmp_" + id, s));
    };
    attach (inSlider,     "In");
    attach (gateSlider,   "Gate");
    attach (bassSlider,   "Bass");
    attach (midSlider,    "Mid");
    attach (trebleSlider, "Treble");
    attach (mixSlider,    "Mix");
    attach (outSlider,    "Out");
    if (! compact)
        attach (levelSlider, "Level");

    addAndMakeVisible (gateLed);
    calibParam = apvts.getRawParameterValue (prefix + "_NAmp_Calib");

    if (! compact)
    {
        for (auto* vu : { &inVu, &outVu })
        {
            vu->setMeterColor (tint());
            vu->setRange (-48.0f, 0.0f);
            vu->setZeroLevel (kVuZeroDbfs);
            addAndMakeVisible (*vu);
        }
        for (auto* l : { &inLabel, &outLabel })
        {
            l->setJustificationType (juce::Justification::centred);
            l->setFont (juce::FontOptions (11.0f));
            l->setBorderSize ({});
            l->setMinimumHorizontalScale (0.7f);
            addAndMakeVisible (*l);
        }
        inLabel.setText ("In", juce::dontSendNotification);
        outLabel.setText ("Out", juce::dontSendNotification);
    }

    refreshStatus();
    timerCallback();
    startTimerHz (30);
}

NeuralAmpComponent::~NeuralAmpComponent()
{
    stopTimer();
}

//==============================================================================
void NeuralAmpComponent::rescanModels()
{
    modelBox.clear (juce::dontSendNotification);
    browsed.clear();
    modelBox.addItem ("Open the models folder", 1);
    modelBox.addSeparator();

    const auto folder = NeuralAmp::getModelsFolder();
    if (folder.isDirectory())
    {
        auto files = folder.findChildFiles (juce::File::findFiles, true, "*.nam;*.json");
        std::sort (files.begin(), files.end(), [] (const juce::File& x, const juce::File& y)
        {
            return x.getFullPathName().compareNatural (y.getFullPathName()) < 0;
        });
        browsed = files;
    }
    for (int i = 0; i < browsed.size(); ++i)
        modelBox.addItem (browsed[i].getRelativePathFrom (folder), i + 2);
    if (browsed.isEmpty())
        modelBox.addSectionHeading ("(no models in FX-Mechanics/Models/NAM yet)");
}

void NeuralAmpComponent::loadFile (const juce::File& f)
{
    juce::String error;
    const bool ok = onLoadFile ? onLoadFile (f, error) : amp.loadModelFile (f, error);
    localError = ok ? juce::String() : error;
    refreshStatus();
}

void NeuralAmpComponent::chooseFile()
{
    const auto folder = NeuralAmp::getModelsFolder();
    chooser = std::make_unique<juce::FileChooser> ("Load a model",
                                                   folder.isDirectory() ? folder
                                                                        : juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                   "*.nam;*.json");
    juce::Component::SafePointer<NeuralAmpComponent> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  safe->loadFile (f);
                          });
}

void NeuralAmpComponent::clear()
{
    if (onClear)
        onClear();
    else
        amp.clearModel();
    localError.clear();
    refreshStatus();
}

void NeuralAmpComponent::refreshStatus()
{
    seenGeneration = amp.getStatusGeneration();
    const auto s = amp.getModelStatus();
    modelPanel.setStatus (s, localError);
    clearButton.setEnabled (s.hasModel);
    seenModelState = juce::roundToInt (amp.getModelStateValue().load());
    updateLevelEnablement (s);
}

void NeuralAmpComponent::updateLevelEnablement (const NeuralAmp::ModelStatus& s)
{
    // Level acts only through Calibrate, and only on a model that says at
    // what level it was captured.
    lastCalib = calibParam != nullptr && calibParam->load() > 0.5f;
    levelSlider.setEnabled (lastCalib && s.info.inputLevelDbu.has_value());
}

void NeuralAmpComponent::timerCallback()
{
    if (amp.getStatusGeneration() != seenGeneration
        || juce::roundToInt (amp.getModelStateValue().load()) != seenModelState)
        refreshStatus();
    else if (calibParam != nullptr && (calibParam->load() > 0.5f) != lastCalib)
        updateLevelEnablement (amp.getModelStatus());

    gateLed.setState (juce::roundToInt (amp.getGateState().load()));
    if (! compact)
    {
        inVu .setValue (amp.getInputMeter() .getRMS());
        outVu.setValue (amp.getOutputMeter().getRMS());
    }
}

//==============================================================================
bool NeuralAmpComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
        if (NeuralAmp::isModelFile (juce::File (f)))
            return true;
    return false;
}

void NeuralAmpComponent::fileDragEnter (const juce::StringArray&, int, int)
{
    dragOver = true;
    repaint();
}

void NeuralAmpComponent::fileDragExit (const juce::StringArray&)
{
    dragOver = false;
    repaint();
}

void NeuralAmpComponent::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    // Not here: on Linux, JUCE calls this inside the drop, before the reply
    // that tells the file manager the drop is over has left (XdndFinished,
    // queued unflushed). Loading here (parsing, compressing into the state,
    // telling FxmeNet) kept the file manager frozen until it was done. The
    // drop ends first; the model loads from the message loop just after.
    for (const auto& path : files)
        if (const juce::File f (path); NeuralAmp::isModelFile (f))
        {
            juce::Component::SafePointer<NeuralAmpComponent> safe (this);
            juce::Timer::callAfterDelay (50, [safe, f]
            {
                if (safe != nullptr)
                    safe->loadFile (f);
            });
            return;
        }
}

//==============================================================================
void NeuralAmpComponent::paint (juce::Graphics& g)
{
    fxme::paintComponentBackground (g, getLocalBounds().toFloat(), tint());
}

void NeuralAmpComponent::paintOverChildren (juce::Graphics& g)
{
    if (! dragOver)
        return;
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (tint().withAlpha (0.12f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (tint());
    g.drawRoundedRectangle (r, 6.0f, 2.0f);
}

void NeuralAmpComponent::resized()
{
    using fi = juce::FlexItem;
    auto area = getLocalBounds().reduced (5);

    juce::FlexBox top, modelRow, modelControls, modelButtons, knobs, left, meters, inCol, outCol, body, fMain;
    top.flexDirection           = juce::FlexBox::Direction::row;
    modelRow.flexDirection      = juce::FlexBox::Direction::row;
    modelControls.flexDirection = juce::FlexBox::Direction::column;
    modelButtons.flexDirection  = juce::FlexBox::Direction::row;
    knobs.flexDirection         = juce::FlexBox::Direction::row;
    left.flexDirection          = juce::FlexBox::Direction::column;
    meters.flexDirection        = juce::FlexBox::Direction::row;
    inCol.flexDirection         = juce::FlexBox::Direction::column;
    outCol.flexDirection        = juce::FlexBox::Direction::column;
    body.flexDirection          = juce::FlexBox::Direction::row;
    fMain.flexDirection         = juce::FlexBox::Direction::column;

    const auto gap = juce::FlexItem::Margin (0.f, 0.f, 0.f, 4.f);

    if (compact)
    {
        top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
        top.items.add (fi (chanBox).withWidth (96.0f).withMargin (gap));
        top.items.add (fi (modelBox).withFlex (1.0f).withMargin (gap));
        top.items.add (fi (loadButton).withWidth (64.0f).withMargin (gap));
        top.items.add (fi (clearButton).withWidth (52.0f).withMargin (gap));

        for (auto* s : { &inSlider, &gateSlider, &bassSlider, &midSlider, &trebleSlider, &mixSlider, &outSlider })
            knobs.items.add (fi (*s).withFlex (1.0f));

        fMain.items.add (fi (top).withHeight (24.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 3.f, 0.f)));
        fMain.items.add (fi (modelPanel).withHeight (22.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 3.f, 0.f)));
        fMain.items.add (fi (knobs).withFlex (1.0f));
        fMain.performLayout (area);
    }
    else
    {
        top.items.add (fi (onButton).withWidth (fxmefx::kOnButtonWidth));
        top.items.add (fi (titleLabel).withFlex (1.0f));
        top.items.add (fi (chanBox).withWidth (104.0f).withMargin (gap));
        top.items.add (fi (calibButton).withWidth (96.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 8.f)));
        top.items.add (fi (normButton).withWidth (96.0f).withMargin (juce::FlexItem::Margin (0.f, 4.f, 0.f, 4.f)));

        // The model: its panel over the browser and its buttons.
        modelButtons.items.add (fi (modelBox).withFlex (1.0f));
        modelButtons.items.add (fi (loadButton).withWidth (72.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 6.f)));
        modelButtons.items.add (fi (clearButton).withWidth (60.0f).withMargin (gap));
        modelControls.items.add (fi (modelPanel).withHeight (64.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 6.f, 0.f)));
        modelControls.items.add (fi (modelButtons).withHeight (26.0f));
        modelRow.items.add (fi (modelControls).withFlex (1.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 0.f)));

        // Knobs on two rows: the drive side, then the tone stack.
        juce::FlexBox row1, row2;
        row1.flexDirection = juce::FlexBox::Direction::row;
        row2.flexDirection = juce::FlexBox::Direction::row;
        for (auto* s : { &inSlider, &gateSlider, &levelSlider })
            row1.items.add (fi (*s).withFlex (1.0f));
        for (auto* s : { &bassSlider, &midSlider, &trebleSlider })
            row2.items.add (fi (*s).withFlex (1.0f));
        left.items.add (fi().withFlex (0.5f));
        left.items.add (fi (row1).withHeight (kKnobRowHeight));
        left.items.add (fi().withHeight (6.0f));
        left.items.add (fi (row2).withHeight (kKnobRowHeight));
        left.items.add (fi().withFlex (0.5f));

        inCol.items.add  (fi (inVu).withFlex (1.0f));
        inCol.items.add  (fi (inLabel).withHeight (16.0f));
        outCol.items.add (fi (outVu).withFlex (1.0f));
        outCol.items.add (fi (outLabel).withHeight (16.0f));
        meters.items.add (fi (inCol).withWidth (21.0f).withMargin (juce::FlexItem::Margin (0.f, 2.f, 0.f, 0.f)));
        meters.items.add (fi (outCol).withWidth (21.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 0.f, 2.f)));

        body.items.add (fi (left).withFlex (1.0f));
        body.items.add (fi (meters).withWidth (48.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 10.f)));
        body.items.add (fi (mixSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, 5.f, 0.f, 5.f)));
        body.items.add (fi (outSlider).withWidth (56.0f).withMargin (juce::FlexItem::Margin (0.f, fxmefx::kEdgeMargin, 0.f, 5.f)));

        fMain.items.add (fi (top).withHeight (fxmefx::kHeaderRowHeight)
                                 .withMinHeight (fxmefx::kHeaderRowHeight)
                                 .withMargin (juce::FlexItem::Margin (5.f, 0.f, 6.f, 0.f)));
        fMain.items.add (fi (modelRow).withHeight (96.0f).withMargin (juce::FlexItem::Margin (0.f, 0.f, 8.f, 0.f)));
        fMain.items.add (fi (body).withFlex (1.0f));
        fMain.performLayout (area);
    }

    // The gate's LED, at the top right of its knob.
    const auto gb = gateSlider.getBounds();
    gateLed.setBounds (gb.getRight() - 18, gb.getY() + 2, 14, 14);
}
