#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace compressor_parameters;

//==============================================================================================
// The main system elements of the compressor.

EclistarVSTAudioProcessor::EclistarVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    //---------------------------------------------------------------------
    // Next, parameter casting is implemented
    // using simplified lambda functions.

    const auto& parameters = GetParameters();

    // Choice cast.

    auto ChoiceCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast <AudioParameterChoice*> (apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Float cast.

    auto FloatCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast <AudioParameterFloat*> (apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    // Bool cast.

    auto BoolCastHelper = [&apvts = this->apvts, &parameters](auto& parameter, const auto& parameterName)
    {
        parameter = dynamic_cast <AudioParameterBool*> (apvts.getParameter(parameters.at(parameterName)));
        jassert(parameter != nullptr);
    };

    //---------------------------------------------------------------------
    // Application of lambda functions:

    // Gain;

    FloatCastHelper(_inputGain, NamesOfParameters::gainInput);
    FloatCastHelper(_outputGain, NamesOfParameters::gainOutput);

    // Low compressor;

    ChoiceCastHelper(_lowCompressor.ratio, NamesOfParameters::ratioLowBand);

    FloatCastHelper(_lowCompressor.attack, NamesOfParameters::attackLowBand);
    FloatCastHelper(_lowCompressor.release, NamesOfParameters::releaseLowBand);
    FloatCastHelper(_lowCompressor.threshold, NamesOfParameters::thresholdLowBand);

    BoolCastHelper(_lowCompressor.solo, NamesOfParameters::soloLowBand);
    BoolCastHelper(_lowCompressor.mute, NamesOfParameters::muteLowBand);
    BoolCastHelper(_lowCompressor.bypassed, NamesOfParameters::bypassedLowBand);

    // Middle compressor;

    ChoiceCastHelper(_midCompressor.ratio, NamesOfParameters::ratioMidBand);

    FloatCastHelper(_midCompressor.attack, NamesOfParameters::attackMidBand);
    FloatCastHelper(_midCompressor.release, NamesOfParameters::releaseMidBand);
    FloatCastHelper(_midCompressor.threshold, NamesOfParameters::thresholdMidBand);

    BoolCastHelper(_midCompressor.solo, NamesOfParameters::soloMidBand);
    BoolCastHelper(_midCompressor.mute, NamesOfParameters::muteMidBand);
    BoolCastHelper(_midCompressor.bypassed, NamesOfParameters::bypassedMidBand);

    // High Compressor;

    ChoiceCastHelper(_highCompressor.ratio, NamesOfParameters::ratioHighBand);

    FloatCastHelper(_highCompressor.attack, NamesOfParameters::attackHighBand);
    FloatCastHelper(_highCompressor.release, NamesOfParameters::releaseHighBand);
    FloatCastHelper(_highCompressor.threshold, NamesOfParameters::thresholdHighBand);

    BoolCastHelper(_highCompressor.solo, NamesOfParameters::soloHighBand);
    BoolCastHelper(_highCompressor.mute, NamesOfParameters::muteHighBand);
    BoolCastHelper(_highCompressor.bypassed, NamesOfParameters::bypassedHighBand);

    // Crossovers.

    FloatCastHelper(_lowMidCrossover, NamesOfParameters::lowMidCrossoverFreq);
    FloatCastHelper(_midHighCrossover, NamesOfParameters::midHighCrossoverFreq);

    // Determining channel parameters.

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

//==============================================================================================
// This block contains system functions that currently do not contain
// and do not require to contain anything.

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

const String EclistarVSTAudioProcessor::getProgramName(int index)
{
    return {};
}

void EclistarVSTAudioProcessor::setCurrentProgram(int index)
{
    // Empty...
}

void EclistarVSTAudioProcessor::changeProgramName(int index, const String& newName)
{
    // Empty...
}

//==============================================================================================
// Next, in fact, are the functions of the preprocessor, that is, functions,
// the result of which entails the success or error of launching the plugin.

void EclistarVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Specification of the "music" processor
    // Preparation for processing a musical fragment (sample).

    ProcessSpec processSpec;

    processSpec.maximumBlockSize = samplesPerBlock;
    processSpec.numChannels = getTotalNumOutputChannels();
    processSpec.sampleRate = sampleRate;

    // Preparing several compressors.

    for (auto& compressor : _compressors)
    {
        compressor.prepare(processSpec);
    }

    // Preparing levels of compressor.

    _lowPass1.prepare(processSpec);
    _highPass1.prepare(processSpec);

    _lowPass2.prepare(processSpec);
    _highPass2.prepare(processSpec);

    _allPass2.prepare(processSpec);

    // Preparing gain.

    _inGain.prepare(processSpec);
    _outGain.prepare(processSpec);

    _inGain.setRampDurationSeconds(0.05);
    _outGain.setRampDurationSeconds(0.05);

    // Setting the size of buffers for transmitting sounds.

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
bool EclistarVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    ignoreUnused(layouts);
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
    //---------------------------------------------------------------------
    // Operational scope.

    ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clearing the old channel.

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update the parameters of all compressors.

    for (auto& compressor : _compressors)
    {
        compressor.updateVstCompressorSettings();
    }

    // Set gain parameters.

    _inGain.setGainDecibels(_inputGain->get());
    _outGain.setGainDecibels(_outputGain->get());

    // In gain used before applying filters.

    ApplyGain(buffer, _inGain);

    // Set the buffer size.

    for (auto& filterBuffer : _multiFilterBuffers)
    {
        filterBuffer = buffer;
    }

    // Audio cutoff for channels.

    auto lowMidCutoff = _lowMidCrossover->get();
    auto midHighCutoff = _midHighCrossover->get();

    _lowPass1.setCutoffFrequency(lowMidCutoff);
    _highPass1.setCutoffFrequency(lowMidCutoff);

    _lowPass2.setCutoffFrequency(midHighCutoff);
    _highPass2.setCutoffFrequency(midHighCutoff);

    _allPass2.setCutoffFrequency(midHighCutoff);

    // Creating a buffer for samples, allocating static memory.

    auto filterBuf0Block = AudioBlock <float>(_multiFilterBuffers[0]);
    auto filterBuf1Block = AudioBlock <float>(_multiFilterBuffers[1]);
    auto filterBuf2Block = AudioBlock <float>(_multiFilterBuffers[2]);

    // ProcessContextReplacing contains contextual information
    // that is passed to the processing method of the algorithm.

    auto filterBuf0Context = ProcessContextReplacing <float>(filterBuf0Block);
    auto filterBuf1Context = ProcessContextReplacing <float>(filterBuf1Block);
    auto filterBuf2Context = ProcessContextReplacing <float>(filterBuf2Block);

    // Processing levels.

    _lowPass1.process(filterBuf0Context);
    _allPass2.process(filterBuf0Context);

    _highPass1.process(filterBuf1Context);
    _multiFilterBuffers[2] = _multiFilterBuffers[1];

    _lowPass2.process(filterBuf1Context);
    _highPass2.process(filterBuf2Context);

    // Determine the size of the data to work with each sub-compressor.

    for (size_t i = 0; i < _multiFilterBuffers.size(); ++i)
    {
        _compressors[i].processing(_multiFilterBuffers[i]);
    }

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    buffer.clear();

    //---------------------------------------------------------------------
    // Buffer exchange with DSP, sound processing.

    auto soledBands = false;

    // Lambda function for applying a filter to the sound

    auto AddFilterBand = [numC = numChannels, numS = numSamples](auto& inputBuffer,
        const auto& source)
    {
        for (auto i = 0; i < numC; ++i)
        {
            inputBuffer.addFrom(i, 0, source, i, 0, numS);
        }
    };

    // Checking for superimposed effects : solo.

    for (auto& compressor : _compressors)
    {
        if (compressor.solo->get())
        {
            soledBands = true;
            break;
        }
    }

    // If there is an imposed effect, we proceed from
    // other states of the compressor elements.

    if (soledBands)
    {
        for (size_t i = 0; i < _compressors.size(); ++i)
        {
            auto& compressor = _compressors[i];
            if (compressor.solo->get())
            {
                AddFilterBand(buffer, _multiFilterBuffers[i]);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < _compressors.size(); ++i)
        {
            auto& compressor = _compressors[i];
            if (!compressor.mute->get())
            {
                AddFilterBand(buffer, _multiFilterBuffers[i]);
            }
        }
    }

    // Output gain used after applying filters.

    ApplyGain(buffer, _outGain);
}

//==============================================================================================
// Functions of editor.

bool EclistarVSTAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* EclistarVSTAudioProcessor::createEditor()
{
    return new GenericAudioProcessorEditor(*this);
}

//==============================================================================================
// Data state status.

void EclistarVSTAudioProcessor::getStateInformation(MemoryBlock& destinationData)
{
    // The sound went to the right place.

    MemoryOutputStream memOutStream(destinationData, true);
    apvts.state.writeToStream(memOutStream);
}

void EclistarVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // A universal structure for storing and interacting data with each other.

    auto valueTree = ValueTree::readFromData(data, sizeInBytes);
    if (valueTree.isValid())
    {
        apvts.replaceState(valueTree);
    }
}

//==============================================================================================
// Layout, application appearance.

AudioProcessorValueTreeState::ParameterLayout
EclistarVSTAudioProcessor::createParameterLayout()
{
    auto attackRange = NormalisableRange <float>(5, 500, 1, 1);
    auto choicesValues = vector <double>{ 1,1.5,2,3,4,5,7,9,10,15,20,50,100 };

    auto gainRange = NormalisableRange <float>(-24.f, 24.f, 0.5f, 1.f);


    StringArray strArray;
    for (auto choice : choicesValues)
    {
        strArray.add(String(choice, 1));
    }

    APVTS::ParameterLayout layout;
    const auto& parameters = GetParameters();

    // Testing sliders:

    // Gain;

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::gainInput),
        parameters.at(NamesOfParameters::gainInput),
        gainRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::gainOutput),
        parameters.at(NamesOfParameters::gainOutput),
        gainRange, 0));

    // Low compressor;

    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioLowBand),
        parameters.at(NamesOfParameters::ratioLowBand),
        strArray, 3));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdLowBand),
        parameters.at(NamesOfParameters::thresholdLowBand),
        NormalisableRange<float>(-60, 12, 1, 1), 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackLowBand),
        parameters.at(NamesOfParameters::attackLowBand),
        attackRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseLowBand),
        parameters.at(NamesOfParameters::releaseLowBand),
        attackRange, 250));

    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::soloLowBand),
        parameters.at(NamesOfParameters::soloLowBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::muteLowBand),
        parameters.at(NamesOfParameters::muteLowBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedLowBand),
        parameters.at(NamesOfParameters::bypassedLowBand),
        false));

    // Middle compressor;

    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioMidBand),
        parameters.at(NamesOfParameters::ratioMidBand),
        strArray, 3));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdMidBand),
        parameters.at(NamesOfParameters::thresholdMidBand),
        NormalisableRange<float>(-60, 12, 1, 1), 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackMidBand),
        parameters.at(NamesOfParameters::attackMidBand),
        attackRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseMidBand),
        parameters.at(NamesOfParameters::releaseMidBand),
        attackRange, 250));

    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::soloMidBand),
        parameters.at(NamesOfParameters::soloMidBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::muteMidBand),
        parameters.at(NamesOfParameters::muteMidBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedMidBand),
        parameters.at(NamesOfParameters::bypassedMidBand),
        false));

    // High compressor;

    layout.add(make_unique<AudioParameterChoice>(parameters.at(NamesOfParameters::ratioHighBand),
        parameters.at(NamesOfParameters::ratioHighBand),
        strArray, 3));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::thresholdHighBand),
        parameters.at(NamesOfParameters::thresholdHighBand),
        NormalisableRange<float>(-60, 12, 1, 1), 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::attackHighBand),
        parameters.at(NamesOfParameters::attackHighBand),
        attackRange, 0));
    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::releaseHighBand),
        parameters.at(NamesOfParameters::releaseHighBand),
        attackRange, 250));

    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::soloHighBand),
        parameters.at(NamesOfParameters::soloHighBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::muteHighBand),
        parameters.at(NamesOfParameters::muteHighBand),
        false));
    layout.add(make_unique<AudioParameterBool>(parameters.at(NamesOfParameters::bypassedHighBand),
        parameters.at(NamesOfParameters::bypassedHighBand),
        false));

    // Crossovers.

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::lowMidCrossoverFreq),
        parameters.at(NamesOfParameters::lowMidCrossoverFreq),
        NormalisableRange<float>(20, 999, 1, 1), 400));

    layout.add(make_unique<AudioParameterFloat>(parameters.at(NamesOfParameters::midHighCrossoverFreq),
        parameters.at(NamesOfParameters::midHighCrossoverFreq),
        NormalisableRange<float>(1000, 20000, 1, 1), 2000));


    return layout;
}

//==============================================================================================
// Creating plugin filters and crossovers.

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EclistarVSTAudioProcessor();
}

