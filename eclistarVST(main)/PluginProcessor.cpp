#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Parameters;

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
    const auto& parameters = GetParameters();

    auto Fhelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterFloat*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    auto Chelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterChoice*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    auto Bhelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterBool*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };


    Fhelper(compressor.attack, NamesOfParameters::attackLowBand);
    Fhelper(compressor.release, NamesOfParameters::releaseLowBand);
    Fhelper(compressor.threshold, NamesOfParameters::thresholdLowBand);

    Chelper(compressor.ratio, NamesOfParameters::ratioLowBand);

    Bhelper(compressor.bypassed, NamesOfParameters::bypassedLowBand);
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

void EclistarVSTAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================

void EclistarVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp::ProcessSpec processSpec;

    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();
    processSpec.sampleRate = sampleRate;

    compressor.prepare(processSpec);
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


    compressor.updateVstCompressorSettings();
    compressor.processing(buffer);
}

//==============================================================================

bool EclistarVSTAudioProcessor::hasEditor() const
{
    return true; 
}

AudioProcessorEditor* EclistarVSTAudioProcessor::createEditor()
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
    auto attackReRange = NormalisableRange<float>(5, 500, 1, 1);
    auto choicesVal = vector<double>{ 1,1.5,2,3,4,5,7,9,10,15,20,50,100 };

    StringArray strArr;
    for (auto choice : choicesVal)
    {
        strArr.add(String(choice, 1));
    }


    APVTS::ParameterLayout layout;
    const auto& parameters = GetParameters();


    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdLowBand),
                                                parameters.at(NamesOfParameters::thresholdLowBand),
                                                NormalisableRange<float>(-60, 12, 1, 1), 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackLowBand),
                                                parameters.at(NamesOfParameters::attackLowBand),
                                                attackReRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseLowBand),
                                                parameters.at(NamesOfParameters::releaseLowBand),
                                                attackReRange, 250));
    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioLowBand),
                                                 parameters.at(NamesOfParameters::ratioLowBand),
                                                 strArr, 3));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedLowBand),
                                               parameters.at(NamesOfParameters::bypassedLowBand),
                                               false));

    return layout;
}

//==============================================================================

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}
