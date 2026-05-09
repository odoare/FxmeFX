/*
  ==============================================================================

    Oct plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeOctAudioProcessor::FxmeOctAudioProcessor()
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
    oct.assignParameters (apvts, parameterPrefix);
}

FxmeOctAudioProcessor::~FxmeOctAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeOctAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Oct::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeOctAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeOctAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeOctAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeOctAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeOctAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeOctAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeOctAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeOctAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeOctAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeOctAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeOctAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    oct.prepare (sampleRate, getTotalNumInputChannels());
}

void FxmeOctAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeOctAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeOctAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    oct.checkParameters();
    oct.process (buffer);
}

bool FxmeOctAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeOctAudioProcessor::createEditor()
{
    return new FxmeOctAudioProcessorEditor (*this);
}

void FxmeOctAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeOctAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeOctAudioProcessor();
}
