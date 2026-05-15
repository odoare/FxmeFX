/*
  ==============================================================================

    ConvolReverb plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#ifndef FXME_PD_BUILD
 #include "PluginEditor.h"
#endif

namespace
{
    // Display name (.wav filename) → BinaryData resource symbol.
    // Symbols are produced by juce_add_binary_data: non-alphanumeric chars
    // become underscores, so "Council Chamber.wav" → "Council_Chamber_wav".
    struct IRMapping { const char* display; const char* resource; };

    static const IRMapping kBuiltInIRs[] =
    {
        { "Council Chamber.wav",         "Council_Chamber_wav"        },
        { "Forest short.wav",            "Forest_short_wav"           },
        { "Forest long.wav",             "Forest_long_wav"            },
        { "Rectangular room small.wav",  "Rectangular_room_small_wav" },
        { "Rectangular room medium.wav", "Rectangular_room_medium_wav"},
        { "Rectangular room large.wav",  "Rectangular_room_large_wav" },
    };
}

int FxmeConvolReverbAudioProcessor::getNumBuiltInIRs()
{
    return (int) (sizeof (kBuiltInIRs) / sizeof (kBuiltInIRs[0]));
}

void FxmeConvolReverbAudioProcessor::getBuiltInIRList (juce::StringArray& names, juce::StringArray& resources)
{
    names.clear();
    resources.clear();
    for (const auto& ir : kBuiltInIRs)
    {
        names.add (ir.display);
        resources.add (ir.resource);
    }
}

FxmeConvolReverbAudioProcessor::FxmeConvolReverbAudioProcessor()
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
    reverb.assignParameters (apvts, parameterPrefix);

    juce::StringArray names, resources;
    getBuiltInIRList (names, resources);
    reverb.setImpulseList (names, resources);
}

FxmeConvolReverbAudioProcessor::~FxmeConvolReverbAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout FxmeConvolReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    ConvolReverb::addParameters (params, parameterPrefix, getNumBuiltInIRs());
    return { params.begin(), params.end() };
}

const juce::String FxmeConvolReverbAudioProcessor::getName() const          { return JucePlugin_Name; }
bool   FxmeConvolReverbAudioProcessor::acceptsMidi() const                  { return false; }
bool   FxmeConvolReverbAudioProcessor::producesMidi() const                 { return false; }
bool   FxmeConvolReverbAudioProcessor::isMidiEffect() const                 { return false; }
double FxmeConvolReverbAudioProcessor::getTailLengthSeconds() const         { return 8.0; }
int    FxmeConvolReverbAudioProcessor::getNumPrograms()                     { return 1; }
int    FxmeConvolReverbAudioProcessor::getCurrentProgram()                  { return 0; }
void   FxmeConvolReverbAudioProcessor::setCurrentProgram (int)              {}
const  juce::String FxmeConvolReverbAudioProcessor::getProgramName (int)    { return {}; }
void   FxmeConvolReverbAudioProcessor::changeProgramName (int, const juce::String&) {}

void FxmeConvolReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    reverb.prepare (sampleRate, samplesPerBlock);
}

void FxmeConvolReverbAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FxmeConvolReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
#endif

void FxmeConvolReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalIn  = getTotalNumInputChannels();
    auto totalOut = getTotalNumOutputChannels();
    for (auto i = totalIn; i < totalOut; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    reverb.process (buffer);
}

#ifdef FXME_PD_BUILD
bool FxmeConvolReverbAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* FxmeConvolReverbAudioProcessor::createEditor() { return nullptr; }
#else
bool FxmeConvolReverbAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FxmeConvolReverbAudioProcessor::createEditor()
{
    return new FxmeConvolReverbAudioProcessorEditor (*this);
}
#endif

void FxmeConvolReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
}

void FxmeConvolReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FxmeConvolReverbAudioProcessor();
}
