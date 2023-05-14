#pragma once

#include <JuceHeader.h>

using namespace juce;
using namespace dsp;
using namespace std;

//==============================================================================================
// Namespace of compressor parameters.

namespace compressor_parameters
{
    enum NamesOfParameters
    {
        // Names of the main parameters.

        ratioLowBand,
        ratioMidBand,
        ratioHighBand,

        attackLowBand,
        attackMidBand,
        attackHighBand,

        releaseLowBand,
        releaseMidBand,
        releaseHighBand,

        thresholdLowBand,
        thresholdMidBand,
        thresholdHighBand,

        // Names of additional parameters and channels.

        gainInput,
        gainOutput,

        soloLowBand,
        soloMidBand,
        soloHighBand,

        muteLowBand,
        muteMidBand,
        muteHighBand,

        bypassedLowBand,
        bypassedMidBand,
        bypassedHighBand,

        // Names of crossovers.

        lowMidCrossoverFreq,
        midHighCrossoverFreq,
    };

    // Correlation of parameters and their names when determining on the layout.

    const map<NamesOfParameters, String>& GetParameters()
    {
        static map <NamesOfParameters, String> parameters =
        {
            { ratioLowBand, "ratio low band" },
            { ratioMidBand, "ratio mid band" },
            { ratioHighBand, "Ratio high Band" },

            { attackLowBand, "attack low band" },
            { attackMidBand, "attack mid band" },
            { attackHighBand, "attack high band" },

            { releaseLowBand, "release low band" },
            { releaseMidBand, "release mid band" },
            { releaseHighBand, "release high band" },

            { thresholdLowBand, "threshold low band" },
            { thresholdMidBand, "threshold mid band" },
            { thresholdHighBand, "threshold high band" },


            { gainInput, "gain in" },
            { gainOutput, "gain out" },

            { soloLowBand, "solo low band" },
            { soloMidBand, "solo mid band" },
            { soloHighBand, "solo high band" },

            { muteLowBand, "mute low band" },
            { muteMidBand, "mute mid band" },
            { muteHighBand, "mute high band" },

            { bypassedLowBand, "bypassed low band" },
            { bypassedMidBand, "bypassed mid band" },
            { bypassedHighBand, "bypassed high band" },

            { lowMidCrossoverFreq, "low-mid crossover frequency" },
            { midHighCrossoverFreq, "mid-high crossover frequency" }
        };

        return parameters;
    }
}

//==============================================================================================
// Structure of compressor.

struct VstCompressorBand
{
private:

    Compressor<float> _compressor;

public:

    // Elements of compressor.

    AudioParameterChoice* ratio{ nullptr };

    AudioParameterBool* solo{ nullptr };
    AudioParameterBool* mute{ nullptr };
    AudioParameterBool* bypassed{ nullptr };

    AudioParameterFloat* attack{ nullptr };
    AudioParameterFloat* release{ nullptr };
    AudioParameterFloat* threshold{ nullptr };

    // Functions of the compressor itself.

    void prepare(const ProcessSpec& process_spec)
    {
        _compressor.prepare(process_spec);
    }

    void updateVstCompressorSettings()
    {
        _compressor.setAttack(attack->get());
        _compressor.setRelease(release->get());
        _compressor.setThreshold(threshold->get());
        _compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue());
    }

    void processing(AudioBuffer <float>& buffer)
    {
        auto audioBlock = AudioBlock <float>(buffer);
        auto context = ProcessContextReplacing <float>(audioBlock);

        context.isBypassed = bypassed->get();

        _compressor.process(context);
    }
};

//==============================================================================================
// Class of processor compressor.

class EclistarVSTAudioProcessor : public AudioProcessor
#if JucePlugin_Enable_ARA
    , public AudioProcessorARAExtension
#endif
{
public:

    EclistarVSTAudioProcessor();

    ~EclistarVSTAudioProcessor() override;

//==============================================================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(AudioBuffer<float>&, MidiBuffer&) override;

//==============================================================================================

    AudioProcessorEditor* createEditor() override;

    bool hasEditor() const override;

//==============================================================================================

    const String getName() const override;

    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

//==============================================================================================

    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram(int index) override;

    const String getProgramName(int index) override;

    void changeProgramName(int index, const String& newName) override;

//==============================================================================================

    void getStateInformation(MemoryBlock& destinationData) override;

    void setStateInformation(const void* data, int sizeInBytes) override;

    // Creating a tree of audio parameter values.

    using APVTS = AudioProcessorValueTreeState;

    // Linking the value tree and the layout of the parameters display.

    static APVTS::ParameterLayout createParameterLayout();
    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };


private:
//==============================================================================================
// The following are the elements of the compressor, Linkwitz filter.

    // Define compressors of three levels.

    array <VstCompressorBand, 3> _compressors;

    VstCompressorBand& _lowCompressor = _compressors[0];
    VstCompressorBand& _midCompressor = _compressors[1];
    VstCompressorBand& _highCompressor = _compressors[2];

    // Apply Linkwitz-Riley filters, which will help with the mechanization
    // of compressor activity ranges.

    using Filter = LinkwitzRileyFilter <float>;

    Filter _lowPass1;
    Filter _lowPass2;

    Filter _highPass1;
    Filter _highPass2;

    Filter _allPass2;

    // Creating gain parameters.

    Gain <float> _inGain;                            // values
    Gain <float> _outGain;      

    AudioParameterFloat* _inputGain;        // parameters    
    AudioParameterFloat* _outputGain;

    // Define crossovers and an audio buffer.

    AudioParameterFloat* _lowMidCrossover{ nullptr };
    AudioParameterFloat* _midHighCrossover{ nullptr };

    array <AudioBuffer <float>, 3> _multiFilterBuffers;

    // Apply the gain & gain context.

    template <typename B, typename G>

    void ApplyGain(B& buffer, G& gain)
    {
        auto audioBlock = AudioBlock <float>(buffer);
        auto context = ProcessContextReplacing <float>(audioBlock);

        gain.process(context);
    }

//==============================================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EclistarVSTAudioProcessor)
};