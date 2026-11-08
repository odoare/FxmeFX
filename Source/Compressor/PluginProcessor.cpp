/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeCompressorAudioProcessor::FxmeCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #ifndef FXME_PD_BUILD
                       // The key for the Ext SC switch: off until the host routes something to it.
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                     #endif
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#else
     : apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    compressor.assignParameters (apvts, parameterPrefix);
   #ifndef FXME_PD_BUILD
    net.telemetry().addMeter ("gr", compressor.getGrMeter());   // FxmeNet: the gain reduction meter
   #endif
}

FxmeCompressorAudioProcessor::~FxmeCompressorAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FxmeCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
   #ifdef FXME_PD_BUILD
    Compressor::addParameters (params, parameterPrefix);          // no aux input in Pd: no Ext SC switch
   #else
    Compressor::addParameters (params, parameterPrefix, true);
   #endif
    return { params.begin(), params.end() };
}

const juce::String FxmeCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FxmeCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FxmeCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FxmeCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FxmeCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FxmeCompressorAudioProcessor::getNumPrograms()
{
    return 1;
}

int FxmeCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FxmeCompressorAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String FxmeCompressorAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void FxmeCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void FxmeCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    compressor.prepare (sampleRate, getMainBusNumInputChannels());
}

void FxmeCompressorAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

    // The side-chain input is optional: absent, off, mono or stereo.
    if (layouts.inputBuses.size() > 1)
    {
        const auto key = layouts.getChannelSet (true, 1);
        if (! key.isDisabled() && key != juce::AudioChannelSet::mono() && key != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
  #endif
}
#endif

void FxmeCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    compressor.checkParameters();
   #ifdef FXME_PD_BUILD
    compressor.process (buffer);
   #else
    // The buffer holds the main channels and, after them, the side-chain's:
    // the effect sees the main ones, and the key apart. No allocation: both
    // only refer to the host's channels.
    auto mainBus = getBusBuffer (buffer, true, 0);
    auto* keyBus = getBus (true, 1);
    if (keyBus != nullptr && keyBus->isEnabled() && keyBus->getNumberOfChannels() > 0)
    {
        const auto key = getBusBuffer (buffer, true, 1);
        compressor.process (mainBus, &key);
    }
    else
    {
        compressor.process (mainBus);
    }
   #endif
}

bool FxmeCompressorAudioProcessor::hasEditor() const
{
   #ifdef FXME_PD_BUILD
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* FxmeCompressorAudioProcessor::createEditor()
{
   #ifdef FXME_PD_BUILD
    return nullptr;
   #else
    return new FxmeCompressorAudioProcessorEditor (*this);
   #endif
}

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new FxmeCompressorAudioProcessor();
}
