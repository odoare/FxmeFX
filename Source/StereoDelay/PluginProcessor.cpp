/*
  ==============================================================================

    StereoDelay plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

FxmeStereoDelayAudioProcessor::FxmeStereoDelayAudioProcessor()
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
    stereoDelay.assignParameters (apvts, parameterPrefix);
}

FxmeStereoDelayAudioProcessor::~FxmeStereoDelayAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeStereoDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    StereoDelay::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeStereoDelayAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeStereoDelayAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeStereoDelayAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeStereoDelayAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeStereoDelayAudioProcessor::getTailLengthSeconds() const         { return 4.0; }
int    FxmeStereoDelayAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeStereoDelayAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeStereoDelayAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeStereoDelayAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeStereoDelayAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeStereoDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    stereoDelay.prepare (sampleRate, samplesPerBlock);
}

void FxmeStereoDelayAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeStereoDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeStereoDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (auto* playHead = getPlayHead())
    {
        if (auto pos = playHead->getPosition())
            if (auto bpm = pos->getBpm())
                stereoDelay.setBPM (*bpm);
    }

    stereoDelay.checkParameters();
    stereoDelay.process (buffer);
}

bool FxmeStereoDelayAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeStereoDelayAudioProcessor::createEditor()
{
    return new FxmeStereoDelayAudioProcessorEditor (*this);
}

void FxmeStereoDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeStereoDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeStereoDelayAudioProcessor();
}
