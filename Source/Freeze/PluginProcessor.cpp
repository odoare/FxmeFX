/*
  ==============================================================================

    Freeze plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeFreezeAudioProcessor::FxmeFreezeAudioProcessor()
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
    freeze.assignParameters (apvts, parameterPrefix);
}

FxmeFreezeAudioProcessor::~FxmeFreezeAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeFreezeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Freeze::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeFreezeAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeFreezeAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeFreezeAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeFreezeAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeFreezeAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeFreezeAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeFreezeAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeFreezeAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeFreezeAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeFreezeAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeFreezeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    freeze.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmeFreezeAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeFreezeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeFreezeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    freeze.checkParameters();
    freeze.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeFreezeAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeFreezeAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeFreezeAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeFreezeAudioProcessor::createEditor()
{
    return new FxmeFreezeAudioProcessorEditor (*this);
}
#endif

void FxmeFreezeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeFreezeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeFreezeAudioProcessor();
}
