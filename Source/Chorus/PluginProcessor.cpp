/*
  ==============================================================================

    Chorus plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeChorusAudioProcessor::FxmeChorusAudioProcessor()
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
    chorus.assignParameters (apvts, parameterPrefix);
}

FxmeChorusAudioProcessor::~FxmeChorusAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeChorusAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Chorus::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeChorusAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeChorusAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeChorusAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeChorusAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeChorusAudioProcessor::getTailLengthSeconds() const         { return 0.06; }
int    FxmeChorusAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeChorusAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeChorusAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeChorusAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeChorusAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeChorusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chorus.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmeChorusAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeChorusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeChorusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
            chorus.setTransport (bpm ? *bpm : chorus.getBPM(),
                                 ppq ? *ppq : 0.0,
                                 pos->getIsPlaying() && ppq.hasValue());
        }
    }

    chorus.checkParameters();
    chorus.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeChorusAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeChorusAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeChorusAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeChorusAudioProcessor::createEditor()
{
    return new FxmeChorusAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeChorusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeChorusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            // States written before versioning report 1. Nothing to migrate yet.
            const int stateVersion = xml->getIntAttribute ("stateVersion", 1);
            juce::ignoreUnused (stateVersion);

            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeChorusAudioProcessor();
}
