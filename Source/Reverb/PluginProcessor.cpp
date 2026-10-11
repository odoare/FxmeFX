/*
  ==============================================================================

    Reverb plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeReverbAudioProcessor::FxmeReverbAudioProcessor()
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
    reverb.assignParameters (apvts, parameterPrefix);
}

FxmeReverbAudioProcessor::~FxmeReverbAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    ReverbEffect::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeReverbAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeReverbAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeReverbAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeReverbAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeReverbAudioProcessor::getTailLengthSeconds() const         { return 8.0; }
int    FxmeReverbAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeReverbAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeReverbAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeReverbAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeReverbAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    reverb.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmeReverbAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    reverb.checkParameters();
    reverb.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeReverbAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeReverbAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeReverbAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeReverbAudioProcessor::createEditor()
{
    return new FxmeReverbAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new FxmeReverbAudioProcessor();
}
