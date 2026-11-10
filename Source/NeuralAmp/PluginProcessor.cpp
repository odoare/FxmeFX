/*
  ==============================================================================

    NeuralAmp plugin processor (FxmeNeuralAmp).

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeNeuralAmpAudioProcessor::FxmeNeuralAmpAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    amp.assignParameters (apvts, parameterPrefix);
    amp.setMirrorCheck ([this] { return net.isMirror(); });

    // The model travels with the effect's presets (in their <Extra>); a
    // preset without one keeps the model playing.
    presets.target.onWriteExtra = [this] (juce::XmlElement& extra) { amp.writePresetExtra (extra); };
    presets.target.onReadExtra = [this] (const juce::XmlElement* extra)
    {
        amp.readPresetExtra (extra);
        net.notifyStateChanged();
    };

    // FxmeNet: the meters and the gate's state.
    net.telemetry().addMeter ("in",   amp.getInputMeter());
    net.telemetry().addMeter ("out",  amp.getOutputMeter());
    net.telemetry().addValue ("gate", amp.getGateState());
    net.telemetry().addValue ("model", amp.getModelStateValue());   // the server's model state, for the mirror
}

FxmeNeuralAmpAudioProcessor::~FxmeNeuralAmpAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeNeuralAmpAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    NeuralAmp::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeNeuralAmpAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeNeuralAmpAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeNeuralAmpAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeNeuralAmpAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeNeuralAmpAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeNeuralAmpAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeNeuralAmpAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeNeuralAmpAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeNeuralAmpAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeNeuralAmpAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeNeuralAmpAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    amp.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
    setLatencySamples (amp.getLatencySamples());
}

void FxmeNeuralAmpAudioProcessor::releaseResources() {}

bool FxmeNeuralAmpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void FxmeNeuralAmpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    amp.process (buffer);
}

bool FxmeNeuralAmpAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeNeuralAmpAudioProcessor::createEditor()
{
    return new FxmeNeuralAmpAudioProcessorEditor (*this);
}

bool FxmeNeuralAmpAudioProcessor::loadModelFile (const juce::File& file, juce::String& error)
{
    if (! amp.loadModelFile (file, error))
        return false;
    net.notifyStateChanged();   // FxmeNet copies the state, model included
    return true;
}

void FxmeNeuralAmpAudioProcessor::clearModel()
{
    amp.clearModel();
    net.notifyStateChanged();
}

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions.
static constexpr int kStateVersion = 1;

void FxmeNeuralAmpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // The model is in the state (fxme::EmbeddedData), so it is saved with it.
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeNeuralAmpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            amp.syncModelFromState();   // the session's model, loaded off the audio thread
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeNeuralAmpAudioProcessor();
}
