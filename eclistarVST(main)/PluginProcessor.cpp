#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace compressor_parameters;

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


    // Choice cast

    auto ChoiceCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterChoice*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Float cast

    auto FloatCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterFloat*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Bool cast

    auto BoolCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast<AudioParameterBool*>(apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };


    // Application of lambda functions

    FloatCastHelper(_compressor.attack, NamesOfParameters::attackLowBand);
    FloatCastHelper(_compressor.release, NamesOfParameters::releaseLowBand);
    FloatCastHelper(_lowMidCrossover, NamesOfParameters::lowMidCrossoverFreq);
    FloatCastHelper(_compressor.threshold, NamesOfParameters::thresholdLowBand);
    FloatCastHelper(_midHighCrossover, NamesOfParameters::midHighCrossoverFreq);

    ChoiceCastHelper(_compressor.ratio, NamesOfParameters::ratioLowBand);

    BoolCastHelper(_compressor.bypassed, NamesOfParameters::bypassedLowBand);

    // Determining channel parameters

    _lowPass1.setType(LinkwitzRileyFilterType::lowpass);
    _highPass1.setType(LinkwitzRileyFilterType::highpass);

    _lowPass2.setType(LinkwitzRileyFilterType::lowpass);
    _highPass2.setType(LinkwitzRileyFilterType::highpass);

    _allPass2.setType(LinkwitzRileyFilterType::allpass);
}

EclistarVSTAudioProcessor::~EclistarVSTAudioProcessor()
{
    // Empty...
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

    _lowPass1.prepare(processSpec);
    _highPass1.prepare(processSpec);

    _lowPass2.prepare(processSpec);
    _highPass2.prepare(processSpec);

    _allPass2.prepare(processSpec);


    _allPassBuffer.setSize(processSpec.numChannels, samplesPerBlock);


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

    // Clearing the old channel

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    for (auto& filter_buffer : _multiFilterBuffers)
    {
        filter_buffer = buffer;
    }

    // Audio cutoff for channels

    auto lowMidCutoff = _lowMidCrossover->get();
    auto midHighCutoff = _midHighCrossover->get();

    _lowPass1.setCutoffFrequency(lowMidCutoff);
    _highPass1.setCutoffFrequency(lowMidCutoff);

    _lowPass2.setCutoffFrequency(midHighCutoff);
    _highPass2.setCutoffFrequency(midHighCutoff);

    _allPass2.setCutoffFrequency(midHighCutoff);

    // Creating a buffer for samples, allocating memory

    auto filterBuf0Block = AudioBlock<float>(_multiFilterBuffers[0]);
    auto filterBuf1Block = AudioBlock<float>(_multiFilterBuffers[1]);
    auto filterBuf2Block = AudioBlock<float>(_multiFilterBuffers[2]);

    // ProcessContextReplacing contains contextual information
    // that is passed to the processing method of the algorithm

    auto filterBuf0Context = ProcessContextReplacing<float>(filterBuf0Block);
    auto filterBuf1Context = ProcessContextReplacing<float>(filterBuf1Block);
    auto filterBuf2Context = ProcessContextReplacing<float>(filterBuf2Block);

    _lowPass1.process(filterBuf0Context);
    _allPass2.process(filterBuf0Context);

    _highPass1.process(filterBuf1Context);
    _multiFilterBuffers[2] = _multiFilterBuffers[1];

    _lowPass2.process(filterBuf1Context);
    _highPass2.process(filterBuf2Context);

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();


    buffer.clear();

    // Buffer exchange with DSP, sound processing

    auto AddFilterBand = [numC = numChannels, numS = numSamples](auto& input_buffer,
                                                                 const auto& source)
    {
        for (auto i = 0; i < numC; ++i)
        {
            input_buffer.addFrom(i, 0, source, i, 0, numS);
        }
    };

    // When the compressor is in bypass mode,
    // it is necessary to analyze all passing signals

    AddFilterBand(buffer, _multiFilterBuffers[0]);
    AddFilterBand(buffer, _multiFilterBuffers[1]);
    AddFilterBand(buffer, _multiFilterBuffers[2]);

    /*if (_compressor.bypassed->get())
    {
        for (auto channel = 0; channel < numChannels; channel++)
        {
            FloatVectorOperations::multiply(_allPassBuffer.getWritePointer(channel),
                                            -1.f, numSamples);
        }
        AddFilterBand(buffer, _allPassBuffer);
    }
    */
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

void EclistarVSTAudioProcessor::getStateInformation (MemoryBlock& destinationData)
{
    // The sound went to the right place

    MemoryOutputStream mem_out_stream(destinationData, true);
    apvts.state.writeToStream(mem_out_stream);
}

void EclistarVSTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // A universal structure for storing and interacting data with each other

    auto value_tree = ValueTree::readFromData(data, sizeInBytes);
    if (value_tree.isValid())
    {
        apvts.replaceState(value_tree);
    }
}

//==============================================================================
// Layout, application appearance

AudioProcessorValueTreeState::ParameterLayout
EclistarVSTAudioProcessor::createParameterLayout()
{
    auto attack_re_range = NormalisableRange<float>(5, 500, 1, 1);
    auto choices_val = vector<double>{ 1,1.5,2,3,4,5,7,9,10,15,20,50,100 };

    StringArray str_arr;
    for (auto choice : choices_val)
    {
        str_arr.add(String(choice, 1));
    }


    APVTS::ParameterLayout layout;
    const auto& parameters = GetParameters();

    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioLowBand),
                                                 parameters.at(NamesOfParameters::ratioLowBand),
                                                 str_arr, 3));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdLowBand),
                                                parameters.at(NamesOfParameters::thresholdLowBand),
                                                NormalisableRange<float>(-60, 12, 1, 1), 0));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackLowBand),
                                                parameters.at(NamesOfParameters::attackLowBand),
                                                attack_re_range, 0));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseLowBand),
                                                parameters.at(NamesOfParameters::releaseLowBand),
                                                attack_re_range, 250));

    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedLowBand),
                                               parameters.at(NamesOfParameters::bypassedLowBand),
                                               false));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::lowMidCrossoverFreq),
                                                parameters.at(NamesOfParameters::lowMidCrossoverFreq),
                                                NormalisableRange<float>(20, 999, 1, 1),
                                                400));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::midHighCrossoverFreq),
                                                parameters.at(NamesOfParameters::midHighCrossoverFreq),
                                                NormalisableRange<float>(1000, 20000, 1, 1),
                                                2000));

    return layout;
}

//==============================================================================
// Creating plugin filters and crossovers

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}
