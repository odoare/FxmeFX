/*
  ==============================================================================

    Cab plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void FxmeCabAudioProcessor::getBuiltInIRList (juce::StringArray& names, juce::StringArray& resources)
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

int FxmeCabAudioProcessor::getNumBuiltInIRs()
{
    juce::StringArray n, r;
    getBuiltInIRList (n, r);
    return n.size();
}

FxmeCabAudioProcessor::FxmeCabAudioProcessor()
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
    cab.assignParameters (apvts, parameterPrefix);

    juce::StringArray names, resources;
    getBuiltInIRList (names, resources);
    cab.setImpulseList (names, resources);
}

FxmeCabAudioProcessor::~FxmeCabAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeCabAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Cab::addParameters (params, parameterPrefix, getNumBuiltInIRs());
    return { params.begin(), params.end() };
}

const juce::String FxmeCabAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeCabAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeCabAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeCabAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeCabAudioProcessor::getTailLengthSeconds() const         { return 0.5; }
int    FxmeCabAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeCabAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeCabAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeCabAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeCabAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeCabAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    cab.prepare (sampleRate, samplesPerBlock);
}

void FxmeCabAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeCabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeCabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    cab.process (buffer);
}

bool FxmeCabAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeCabAudioProcessor::createEditor()
{
    return new FxmeCabAudioProcessorEditor (*this);
}

void FxmeCabAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeCabAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeCabAudioProcessor();
}
