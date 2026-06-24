/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeLimiterAudioProcessor::FxmeLimiterAudioProcessor()
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
    limiter.assignParameters (apvts, parameterPrefix);
}

FxmeLimiterAudioProcessor::~FxmeLimiterAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FxmeLimiterAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Limiter::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeLimiterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FxmeLimiterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FxmeLimiterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FxmeLimiterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FxmeLimiterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FxmeLimiterAudioProcessor::getNumPrograms()
{
    return 1;
}

int FxmeLimiterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FxmeLimiterAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String FxmeLimiterAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void FxmeLimiterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void FxmeLimiterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    limiter.prepare (sampleRate, getTotalNumInputChannels());
}

void FxmeLimiterAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeLimiterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FxmeLimiterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    limiter.checkParameters();
    limiter.process (buffer);
}

bool FxmeLimiterAudioProcessor::hasEditor() const
{
   #ifdef FXME_PD_BUILD
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* FxmeLimiterAudioProcessor::createEditor()
{
   #ifdef FXME_PD_BUILD
    return nullptr;
   #else
    return new FxmeLimiterAudioProcessorEditor (*this);
   #endif
}

void FxmeLimiterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void FxmeLimiterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeLimiterAudioProcessor();
}
