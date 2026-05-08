/*
  ==============================================================================

    Cabinet plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void FxmeCabinetAudioProcessor::getBuiltInIRList (juce::StringArray& names, juce::StringArray& resources)
{
    names.clear();
    resources.clear();

    // Walk JUCE's BinaryData tables and pick every .wav resource. The
    // generated symbol name is what we hand to BinaryData::getNamedResource;
    // the original filename is what we show in the UI.
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String resource (BinaryData::namedResourceList[i]);
        const juce::String original (BinaryData::originalFilenames[i]);
        if (original.endsWithIgnoreCase (".wav"))
        {
            names.add (original);
            resources.add (resource);
        }
    }
}

int FxmeCabinetAudioProcessor::getNumBuiltInIRs()
{
    juce::StringArray n, r;
    getBuiltInIRList (n, r);
    return n.size();
}

FxmeCabinetAudioProcessor::FxmeCabinetAudioProcessor()
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
    cabinet.assignParameters (apvts, parameterPrefix);

    juce::StringArray names, resources;
    getBuiltInIRList (names, resources);
    cabinet.setImpulseList (names, resources);
}

FxmeCabinetAudioProcessor::~FxmeCabinetAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeCabinetAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Cabinet::addParameters (params, parameterPrefix, getNumBuiltInIRs());
    return { params.begin(), params.end() };
}

const juce::String FxmeCabinetAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeCabinetAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeCabinetAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeCabinetAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeCabinetAudioProcessor::getTailLengthSeconds() const         { return 0.5; }
int    FxmeCabinetAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeCabinetAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeCabinetAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeCabinetAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeCabinetAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeCabinetAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    cabinet.prepare (sampleRate, samplesPerBlock);
}

void FxmeCabinetAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeCabinetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeCabinetAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    cabinet.process (buffer);
}

bool FxmeCabinetAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeCabinetAudioProcessor::createEditor()
{
    return new FxmeCabinetAudioProcessorEditor (*this);
}

void FxmeCabinetAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeCabinetAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeCabinetAudioProcessor();
}
