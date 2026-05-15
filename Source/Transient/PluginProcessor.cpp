/*
  ==============================================================================

    Transient plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeTransientAudioProcessor::FxmeTransientAudioProcessor()
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
    transientFx.assignParameters (apvts, parameterPrefix);
}

FxmeTransientAudioProcessor::~FxmeTransientAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeTransientAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Transient::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeTransientAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeTransientAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeTransientAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeTransientAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeTransientAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeTransientAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeTransientAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeTransientAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeTransientAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeTransientAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeTransientAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    transientFx.prepare (sampleRate, getTotalNumInputChannels());
}

void FxmeTransientAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeTransientAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeTransientAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    transientFx.checkParameters();
    transientFx.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeTransientAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeTransientAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeTransientAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeTransientAudioProcessor::createEditor()
{
    return new FxmeTransientAudioProcessorEditor (*this);
}
#endif

void FxmeTransientAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeTransientAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeTransientAudioProcessor();
}
