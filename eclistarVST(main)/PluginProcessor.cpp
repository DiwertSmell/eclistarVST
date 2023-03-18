#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Parameters;

//==============================================================================
// The main system elements of the compressor

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
    // Next, parameter casting is implemented using simplified lambda functions

    const auto& parameters = GetParameters();

    // Float cast

    auto Fhelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterFloat*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Choice cast

    auto Chelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterChoice*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Bool cast

    auto Bhelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterBool*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Application of lambda functions

    Fhelper(_compressor.attack, NamesOfParameters::attackLowBand);
    Fhelper(_compressor.release, NamesOfParameters::releaseLowBand);
    Fhelper(_compressor.threshold, NamesOfParameters::thresholdLowBand);

    Chelper(_compressor.ratio, NamesOfParameters::ratioLowBand);

    Bhelper(_compressor.bypassed, NamesOfParameters::bypassedLowBand);

    Fhelper(_lowMidCrossover, NamesOfParameters::lowMidCrossoverFreq);

    // Determining channel parameters

    _LP.setType(LinkwitzRileyFilterType::lowpass);
    _HP.setType(LinkwitzRileyFilterType::highpass);
}

EclistarVSTAudioProcessor::~EclistarVSTAudioProcessor()
{
}

//==============================================================================
// This block contains system functions that currently do not contain
// and do not require to contain anything

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

const String EclistarVSTAudioProcessor::getName() const
{
    return JucePlugin_Name;
}
const String EclistarVSTAudioProcessor::getProgramName (int index)
{
    return {};
}

void EclistarVSTAudioProcessor::setCurrentProgram(int index)
{
    // Empty...
}
void EclistarVSTAudioProcessor::changeProgramName (int index, const String& newName)
{
    // Empty...
}

//==============================================================================
// Next, in fact, are the functions of the preprocessor, that is, functions,
// the result of which entails the success or error of launching the plugin

void EclistarVSTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Specification of the "music" processor
    // Preparation for processing a musical fragment (sample).

    ProcessSpec processSpec;

    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();
    processSpec.sampleRate = sampleRate;

    _compressor.prepare(processSpec);
    _LP.prepare(processSpec);
    _HP.prepare(processSpec);

    for (auto& buffer : _multiFilterBuffers)
    {
        buffer.setSize(processSpec.numChannels, samplesPerBlock);
    }
}

void EclistarVSTAudioProcessor::releaseResources()
{
    // Empty...
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
    // Operational scope

    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    for (auto& filterBuffer : _multiFilterBuffers)
    {
        filterBuffer = buffer;
    }

    // Sound synthesis

    auto cutoff = _lowMidCrossover->get();
    _LP.setCutoffFrequency(cutoff);
    _HP.setCutoffFrequency(cutoff);

    auto fb0Block = juce::dsp::AudioBlock<float>(_multiFilterBuffers[0]);
    auto fb1Block = juce::dsp::AudioBlock<float>(_multiFilterBuffers[1]);

    auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
    auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);

    _LP.process(fb0Ctx);
    _HP.process(fb1Ctx);

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    buffer.clear();

    // Buffer exchange with DSP

    auto addFilterBand = [nc = numChannels, ns = numSamples](auto& inputBuffer, const auto& source)
    {
        for (auto i = 0; i < nc; ++i)
        {
            inputBuffer.addFrom(i, 0, source, i, 0, ns);
        }
    };

    addFilterBand(buffer, _multiFilterBuffers[0]);
    addFilterBand(buffer, _multiFilterBuffers[1]);

}

//==============================================================================
// Functions of editor

bool EclistarVSTAudioProcessor::hasEditor() const
{
    return true; 
}

AudioProcessorEditor* EclistarVSTAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor (*this);
}

//==============================================================================
// Data state status

void EclistarVSTAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // The sound went to the right place

    MemoryOutputStream memOstr(destData, true);
    apvts.state.writeToStream(memOstr);
}

void EclistarVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // A universal structure for storing and interacting data with each other

    auto vTree = ValueTree::readFromData(data, sizeInBytes);
    if (vTree.isValid())
    {
        apvts.replaceState(vTree);
    }
}

//==============================================================================
// Layout, application appearance

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

    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioLowBand),
                                                 parameters.at(NamesOfParameters::ratioLowBand),
                                                 strArr, 3));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdLowBand),
                                                parameters.at(NamesOfParameters::thresholdLowBand),
                                                NormalisableRange<float>(-60, 12, 1, 1), 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackLowBand),
                                                parameters.at(NamesOfParameters::attackLowBand),
                                                attackReRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseLowBand),
                                                parameters.at(NamesOfParameters::releaseLowBand),
                                                attackReRange, 250));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedLowBand),
                                               parameters.at(NamesOfParameters::bypassedLowBand),
                                               false));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::lowMidCrossoverFreq),
        parameters.at(NamesOfParameters::lowMidCrossoverFreq),
        NormalisableRange<float>(20, 20000, 1, 1),
        500));

    return layout;
}

//==============================================================================
// Creating plugin filters and crossovers

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}
