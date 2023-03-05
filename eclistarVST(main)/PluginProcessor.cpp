#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EclistarVSTAudioProcessor::EclistarVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    _attack = dynamic_cast<AudioParameterFloat*>(apvts.getParameter("Attack"));
    jassert(_attack != nullptr);   

    _release = dynamic_cast<AudioParameterFloat*>(apvts.getParameter("Release"));
    jassert(_release != nullptr);

    _threshold = dynamic_cast<AudioParameterFloat*>(apvts.getParameter("Threshold"));
    jassert(_threshold != nullptr);

    _ratio = dynamic_cast<AudioParameterChoice*>(apvts.getParameter("Ratio"));
    jassert(_ratio!= nullptr);
}

EclistarVSTAudioProcessor::~EclistarVSTAudioProcessor()
{
}

//==============================================================================
const String EclistarVSTAudioProcessor::getName() const
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

const String EclistarVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void EclistarVSTAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EclistarVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec processSpec;

    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();
    processSpec.sampleRate = sampleRate;

    _compressor.prepare(processSpec);
}

void EclistarVSTAudioProcessor::releaseResources()
{
    
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EclistarVSTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EclistarVSTAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    _compressor.setAttack(_attack->get());
    _compressor.setRelease(_release->get());
    _compressor.setThreshold(_threshold->get());
    _compressor.setRatio(_ratio->getCurrentChoiceName().getFloatValue());


    auto audioBlock = dsp::AudioBlock<float>(buffer);
    auto context = dsp::ProcessContextReplacing<float>(audioBlock);

    _compressor.process(context);
}

//==============================================================================
bool EclistarVSTAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* EclistarVSTAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
void EclistarVSTAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    MemoryOutputStream memOstr(destData, true);
    apvts.state.writeToStream(memOstr);
}

void EclistarVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto vTree = ValueTree::readFromData(data, sizeInBytes);
    if (vTree.isValid())
    {
        apvts.replaceState(vTree);
    }
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout
EclistarVSTAudioProcessor::createParameterLayout()
{
    auto choicesVal = vector<double>{ 1,1.5,2,3,4,5,7,9,10,15,20,50,100 };
    StringArray strArr;
    for (auto choice : choicesVal)
    {
        strArr.add(String(choice, 1));
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
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}
