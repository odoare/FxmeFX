/*
  ==============================================================================

    Phaser plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmePhaserAudioProcessor::FxmePhaserAudioProcessor()
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
    phaser.assignParameters (apvts, parameterPrefix);
}

FxmePhaserAudioProcessor::~FxmePhaserAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmePhaserAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Phaser::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmePhaserAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmePhaserAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmePhaserAudioProcessor::producesMidi() const                 { return false; }
bool   FxmePhaserAudioProcessor::isMidiEffect() const                 { return false; }
double FxmePhaserAudioProcessor::getTailLengthSeconds() const         { return 0.06; }
int    FxmePhaserAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmePhaserAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmePhaserAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmePhaserAudioProcessor::getProgramName (int)    { return {}; }
void   FxmePhaserAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmePhaserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    phaser.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmePhaserAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmePhaserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmePhaserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
            phaser.setTransport (bpm ? *bpm : phaser.getBPM(),
                                 ppq ? *ppq : 0.0,
                                 pos->getIsPlaying() && ppq.hasValue());
        }
    }

    phaser.checkParameters();
    phaser.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmePhaserAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmePhaserAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmePhaserAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmePhaserAudioProcessor::createEditor()
{
    return new FxmePhaserAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmePhaserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmePhaserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new FxmePhaserAudioProcessor();
}
