/*
  ==============================================================================

    Flanger plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeFlangerAudioProcessor::FxmeFlangerAudioProcessor()
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
    flanger.assignParameters (apvts, parameterPrefix);
}

FxmeFlangerAudioProcessor::~FxmeFlangerAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeFlangerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Flanger::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeFlangerAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeFlangerAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeFlangerAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeFlangerAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeFlangerAudioProcessor::getTailLengthSeconds() const         { return 0.06; }
int    FxmeFlangerAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeFlangerAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeFlangerAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeFlangerAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeFlangerAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    flanger.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmeFlangerAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeFlangerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
        {
            const auto bpm = pos->getBpm();
            const auto ppq = pos->getPpqPosition();
            flanger.setTransport (bpm ? *bpm : flanger.getBPM(),
                                 ppq ? *ppq : 0.0,
                                 pos->getIsPlaying() && ppq.hasValue());
        }
    }

    flanger.checkParameters();
    flanger.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeFlangerAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeFlangerAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeFlangerAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeFlangerAudioProcessor::createEditor()
{
    return new FxmeFlangerAudioProcessorEditor (*this);
}
#endif

void FxmeFlangerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeFlangerAudioProcessor();
}
