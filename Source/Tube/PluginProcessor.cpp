/*
  ==============================================================================

    Tube plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeTubeAudioProcessor::FxmeTubeAudioProcessor()
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
    tube.assignParameters (apvts, parameterPrefix);
}

FxmeTubeAudioProcessor::~FxmeTubeAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeTubeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Tube::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeTubeAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeTubeAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeTubeAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeTubeAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeTubeAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeTubeAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeTubeAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeTubeAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeTubeAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeTubeAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeTubeAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    tube.prepare (sampleRate);
}

void FxmeTubeAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeTubeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeTubeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    tube.checkParameters();
    tube.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeTubeAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeTubeAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeTubeAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeTubeAudioProcessor::createEditor()
{
    return new FxmeTubeAudioProcessorEditor (*this);
}
#endif

void FxmeTubeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeTubeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeTubeAudioProcessor();
}
