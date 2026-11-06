/*
  ==============================================================================

    Crusher plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

FxmeCrusherAudioProcessor::FxmeCrusherAudioProcessor()
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
    crusher.assignParameters (apvts, parameterPrefix);
   #ifndef FXME_PD_BUILD
    // FxmeNet: the waveform view's two traces.
    net.telemetry().addTap ("in",  crusher.getInputTap());
    net.telemetry().addTap ("out", crusher.getOutputTap());
   #endif
}

FxmeCrusherAudioProcessor::~FxmeCrusherAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeCrusherAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    Crusher::addParameters (params, parameterPrefix);
    return { params.begin(), params.end() };
}

const juce::String FxmeCrusherAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeCrusherAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeCrusherAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeCrusherAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeCrusherAudioProcessor::getTailLengthSeconds() const         { return 0.0; }
int    FxmeCrusherAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeCrusherAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeCrusherAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeCrusherAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeCrusherAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeCrusherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    crusher.checkParameters();
    crusher.prepare (sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void FxmeCrusherAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeCrusherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeCrusherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    crusher.checkParameters();
    crusher.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeCrusherAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeCrusherAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeCrusherAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeCrusherAudioProcessor::createEditor()
{
    return new FxmeCrusherAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Bump it
// whenever this plugin's state layout changes and branch on it in
// setStateInformation to migrate older sessions — a version cannot be added
// retroactively to states that are already out there.
static constexpr int kStateVersion = 1;

void FxmeCrusherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeCrusherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new FxmeCrusherAudioProcessor();
}
