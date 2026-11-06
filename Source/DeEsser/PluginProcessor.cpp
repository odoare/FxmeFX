/*
  ==============================================================================

    De-esser plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeDeEsserAudioProcessor::FxmeDeEsserAudioProcessor()
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
    deEsser.assignParameters (apvts, parameterPrefix);
   #ifndef FXME_PD_BUILD
    // FxmeNet: the meter and the spectrum.
    net.telemetry().addValue ("level", deEsser.getDisplayLevel());
    net.telemetry().addValue ("gain",  deEsser.getDisplayGain());
    net.telemetry().addTap   ("in",    deEsser.getInputTap());
    net.telemetry().addTap   ("out",   deEsser.getOutputTap());
   #endif
}

FxmeDeEsserAudioProcessor::~FxmeDeEsserAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeDeEsserAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    DeEsser::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeDeEsserAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeDeEsserAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeDeEsserAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeDeEsserAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeDeEsserAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeDeEsserAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeDeEsserAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeDeEsserAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeDeEsserAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeDeEsserAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeDeEsserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    deEsser.checkParameters();
    deEsser.prepare (sampleRate, getTotalNumInputChannels());
}

void FxmeDeEsserAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeDeEsserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeDeEsserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    deEsser.checkParameters();
    deEsser.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeDeEsserAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeDeEsserAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeDeEsserAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeDeEsserAudioProcessor::createEditor()
{
    return new FxmeDeEsserAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeDeEsserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeDeEsserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new FxmeDeEsserAudioProcessor();
}
