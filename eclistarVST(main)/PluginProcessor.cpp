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

    auto ChoiceCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameter_name)
    {
        parameter = dynamic_cast<AudioParameterChoice*>(apvts.getParameter(parameters.at(parameter_name)));
        jassert(parameter != nullptr);
    };

    // Float cast

    auto FloatCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameter_name)
    {
        parameter = dynamic_cast<AudioParameterFloat*>(apvts.getParameter(parameters.at(parameter_name)));
        jassert(parameter != nullptr);
    };

    // Bool cast

    auto BoolCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameter_name)
    {
        parameter = dynamic_cast<AudioParameterBool*>(apvts.getParameter(parameters.at(parameter_name)));
        jassert(parameter != nullptr);
    };


    // Application of lambda functions

    FloatCastHelper(_compressor.attack, NamesOfParameters::attackLowBand);
    FloatCastHelper(_compressor.release, NamesOfParameters::releaseLowBand);
    FloatCastHelper(_low_mid_crossover, NamesOfParameters::lowMidCrossoverFreq);
    FloatCastHelper(_compressor.threshold, NamesOfParameters::thresholdLowBand);

    ChoiceCastHelper(_compressor.ratio, NamesOfParameters::ratioLowBand);

    BoolCastHelper(_compressor.bypassed, NamesOfParameters::bypassedLowBand);

    // Determining channel parameters

    _LP.setType(LinkwitzRileyFilterType::lowpass);
    _HP.setType(LinkwitzRileyFilterType::highpass);

    _AP.setType(LinkwitzRileyFilterType::allpass);
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
void EclistarVSTAudioProcessor::changeProgramName (int index, const String& new_name)
{
    // Empty...
}

//==============================================================================
// Next, in fact, are the functions of the preprocessor, that is, functions,
// the result of which entails the success or error of launching the plugin

void EclistarVSTAudioProcessor::prepareToPlay (double sample_rate, int samples_per_block)
{
    // Specification of the "music" processor
    // Preparation for processing a musical fragment (sample).

    ProcessSpec process_spec;

    process_spec.maximum_block_size = samples_per_block;
    process_spec.num_channels = getTotalNumOutputChannels();
    process_spec.sample_rate = sample_rate;

    _compressor.prepare(process_spec);

    _LP.prepare(process_spec);
    _HP.prepare(process_spec);

    _AP.prepare(process_spec);
    _all_pass_buffer.setSize(process_spec.num_channels, samples_per_block);


    for (auto& buffer : _multi_filter_buffers)
    {
        buffer.setSize(process_spec.num_channels, samples_per_block);
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

void EclistarVSTAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi_messages)
{
    // Operational scope

    ScopedNoDenormals no_denormals;

    auto total_num_input_channels = getTotalNumInputChannels();
    auto total_num_output_channels = getTotalNumOutputChannels();

    // Clearing the old channel

    for (auto i = total_num_input_channels; i < total_num_output_channels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    for (auto& filter_buffer : _multi_filter_buffers)
    {
        filter_buffer = buffer;
    }

    // Audio cutoff for channels

    auto cutoff = _low_mid_crossover->get();

    _LP.setCutoffFrequency(cutoff);
    _HP.setCutoffFrequency(cutoff);
    _AP.setCutoffFrequency(cutoff);

    // Creating a buffer for samples, allocating memory

    auto filter_buf_0_block = AudioBlock<float>(_multi_filter_buffers[0]);
    auto filter_buf_1_block = AudioBlock<float>(_multi_filter_buffers[1]);

    // ProcessContextReplacing contains contextual information
    // that is passed to the processing method of the algorithm

    auto filter_buf_0_context = ProcessContextReplacing<float>(filter_buf_0_block);
    auto filter_buf_1_context = ProcessContextReplacing<float>(filter_buf_1_block);

    auto num_samples = buffer.getNumSamples();
    auto num_channels = buffer.getNumChannels();


    _all_pass_buffer = buffer;

    auto all_pass_block = AudioBlock<float>(_all_pass_buffer);
    auto all_pass_context = ProcessContextReplacing<float>(all_pass_block);
    

    _LP.process(filter_buf_0_context);
    _HP.process(filter_buf_1_context);
    _AP.process(all_pass_context);


    buffer.clear();

    // Buffer exchange with DSP, sound processing

    auto AddFilterBand = [numC = num_channels, numS = num_samples](auto& input_buffer,
                                                                 const auto& source)
    {
        for (auto i = 0; i < numC; ++i)
        {
            input_buffer.addFrom(i, 0, source, i, 0, numS);
        }
    };

    // When the compressor is in bypass mode,
    // it is necessary to analyze all passing signals

    AddFilterBand(buffer, _multi_filter_buffers[0]);
    AddFilterBand(buffer, _multi_filter_buffers[1]);

    if (_compressor.bypassed->get())
    {
        for (auto channel = 0; channel < num_channels; channel++)
        {
            FloatVectorOperations::multiply(_all_pass_buffer.getWritePointer(channel),
                                            -1.f, num_samples);
        }
        AddFilterBand(buffer, _all_pass_buffer);
    }
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

void EclistarVSTAudioProcessor::getStateInformation (MemoryBlock& destination_data)
{
    // The sound went to the right place

    MemoryOutputStream mem_out_stream(destination_data, true);
    apvts.state.writeToStream(mem_out_stream);
}

void EclistarVSTAudioProcessor::setStateInformation (const void* data, int size_in_bytes)
{
    // A universal structure for storing and interacting data with each other

    auto value_tree = ValueTree::readFromData(data, size_in_bytes);
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
