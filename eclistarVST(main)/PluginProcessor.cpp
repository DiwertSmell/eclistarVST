#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace std;

//==============================================================================
EclistarVSTAudioProcessor::EclistarVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EclistarVSTAudioProcessor::~EclistarVSTAudioProcessor()
{
}

//==============================================================================
const juce::String EclistarVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EclistarVSTAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EclistarVSTAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EclistarVSTAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EclistarVSTAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EclistarVSTAudioProcessor::getNumPrograms()
{
    return 1;   
}

int EclistarVSTAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EclistarVSTAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EclistarVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void EclistarVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EclistarVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
}

void EclistarVSTAudioProcessor::releaseResources()
{
    
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EclistarVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

    return true;
  #endif
}
#endif

void EclistarVSTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool EclistarVSTAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* EclistarVSTAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void EclistarVSTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
   
}

void EclistarVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
EclistarVSTAudioProcessor::createParameterLayout()
{
    auto choicesVal = vector<double>{ 1,1.5,2,3,4,5,7,9,10,15,20,50,100 };
    juce::StringArray strArr;
    for (auto choice : choicesVal)
    {
        strArr.add(juce::String(choice, 1));
    }

    APVTS::ParameterLayout layout;
    layout.add(make_unique<AudioParameterFloat>("Threshold",
                                                "Threshold",
                                                 NormalisableRange<float>(-60, 12, 1, 1), 0));

    auto attackReRange = NormalisableRange<float>(5, 500, 1, 1);
    layout.add(make_unique<AudioParameterFloat>("Attack",
                                                "Attack",
                                                 attackReRange, 0));
    layout.add(make_unique<AudioParameterFloat>("Release",
                                                "Release",
                                                 attackReRange, 250));
    layout.add(make_unique<AudioParameterChoice>("Ratio",
                                                 "Ratio",
                                                  strArr, 3));

    return layout;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}
