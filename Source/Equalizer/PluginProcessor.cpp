/*
  ==============================================================================

    Equalizer plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeEqualizerAudioProcessor::FxmeEqualizerAudioProcessor()
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
    equalizer.assignParameters (apvts, parameterPrefix);
}

FxmeEqualizerAudioProcessor::~FxmeEqualizerAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeEqualizerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Equalizer::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeEqualizerAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeEqualizerAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeEqualizerAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeEqualizerAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeEqualizerAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeEqualizerAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeEqualizerAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeEqualizerAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeEqualizerAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeEqualizerAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeEqualizerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    equalizer.prepare (sampleRate, getTotalNumInputChannels());
}

void FxmeEqualizerAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeEqualizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeEqualizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    equalizer.checkParameters();
    equalizer.process (buffer);
}

bool FxmeEqualizerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeEqualizerAudioProcessor::createEditor()
{
    return new FxmeEqualizerAudioProcessorEditor (*this);
}

void FxmeEqualizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeEqualizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeEqualizerAudioProcessor();
}
