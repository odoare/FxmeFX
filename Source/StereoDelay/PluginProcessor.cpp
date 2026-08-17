/*
  ==============================================================================

    StereoDelay plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

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
// MIDI in, but only to know the last note played: the MIDI-note delay mode
// tunes the line to it. See CMakeLists.txt for the AU-side caveat.
bool   FxmeStereoDelayAudioProcessor::acceptsMidi() const                  { return true; }
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

void FxmeStereoDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
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

    // The last note of the block wins: the MIDI-note delay mode tracks whatever
    // was played most recently, and re-triggering the same note is a no-op.
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
            stereoDelay.setLastNoteHz ((float) m.getMidiNoteInHertz (m.getNoteNumber()));
    }

    stereoDelay.checkParameters();
    stereoDelay.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeStereoDelayAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeStereoDelayAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeStereoDelayAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeStereoDelayAudioProcessor::createEditor()
{
    return new FxmeStereoDelayAudioProcessorEditor (*this);
}
#endif

// State format version, written as an attribute on the saved XML. Version 1
// read both delay times as beats and had no mode parameters; version 2 reads
// them under a per-side mode (seconds / DAW sync / MIDI note). States written
// before versioning existed report 1, which is what the migration keys off.
static constexpr int kStateVersion = 2;

void FxmeStereoDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
        {
            xml->setAttribute ("stateVersion", kStateVersion);
            copyXmlToBinary (*xml, destData);
        }
}

void FxmeStereoDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            const int stateVersion = xml->getIntAttribute ("stateVersion", 1);

            // Version 1 stored the delay times as beats and knew no modes; put
            // such a state into DAW-sync mode with rescaled values so it keeps
            // the delay time it was saved with.
            if (stateVersion < 2)
                StereoDelay::migrateLegacyState (*xml, parameterPrefix);

            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeStereoDelayAudioProcessor();
}
