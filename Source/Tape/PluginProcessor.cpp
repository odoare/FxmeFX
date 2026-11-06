/*
  ==============================================================================

    Tape plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeTapeAudioProcessor::FxmeTapeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#else
     : apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    tape.assignParameters (apvts, parameterPrefix);
   #ifndef FXME_PD_BUILD
    // FxmeNet: the VU meters and the hysteresis loop.
    net.telemetry().addMeter ("in",    tape.getInputMeter());
    net.telemetry().addMeter ("out",   tape.getOutputMeter());
    net.telemetry().addTap   ("field", tape.getFieldTap());
    net.telemetry().addTap   ("mag",   tape.getMagnetisationTap());
   #endif
}

FxmeTapeAudioProcessor::~FxmeTapeAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeTapeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Tape::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeTapeAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeTapeAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeTapeAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeTapeAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeTapeAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeTapeAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeTapeAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeTapeAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeTapeAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeTapeAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeTapeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    tape.checkParameters();   // the oversampling, before prepare sets it up
    tape.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
    setLatencySamples (tape.getLatencySamples());
}

void FxmeTapeAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeTapeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeTapeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    tape.checkParameters();
    // The oversampling changed: tell the host its new delay.
    if (tape.getLatencySamples() != getLatencySamples())
        setLatencySamples (tape.getLatencySamples());
    tape.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeTapeAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeTapeAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeTapeAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeTapeAudioProcessor::createEditor()
{
    return new FxmeTapeAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeTapeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeTapeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            // States written before versioning report 1. Nothing to migrate yet.
            const int stateVersion = xml->getIntAttribute ("stateVersion", 1);
            juce::ignoreUnused (stateVersion);

            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeTapeAudioProcessor();
}
