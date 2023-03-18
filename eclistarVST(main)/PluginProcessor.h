#pragma once

#include <JuceHeader.h>

using namespace juce;
using namespace dsp;
using namespace std;

// Namespace of compressor parameters

namespace Parameters
{
    enum NamesOfParameters
    {
        // Names of the main parameters

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

        // Names of additional parameters and channels

        bypassedLowBand,
        bypassedMidBand,
        bypassedHighBand,

        lowMidCrossoverFreq,
        midHighCrossoverFreq,
    };

    const map<NamesOfParameters, String>& GetParameters()
    {
        static map<NamesOfParameters, String> parameters =
        {
            { ratioLowBand, "Ratio Low Band" },
            { ratioMidBand, "Ratio Mid Band" },
            { ratioHighBand, "Ratio High Band" },

            { attackLowBand, "Attack Low Band" },
            { attackMidBand, "Attack Mid Band" },
            { attackHighBand, "Attack High Band" },

            { releaseLowBand, "Release Low Band" },
            { releaseMidBand, "Release Mid Band" },
            { releaseHighBand, "Release High Band" },

            { thresholdLowBand, "Threshold Low Band" },
            { thresholdMidBand, "Threshold Mid Band" },
            { thresholdHighBand, "Threshold High Band" },

            { bypassedLowBand, "Bypassed Low Band" },
            { bypassedMidBand, "Bypassed Mid Band" },
            { bypassedHighBand, "Bypassed High Band" },

            { lowMidCrossoverFreq, "Low-Mid Crossover Frequency" },
            { midHighCrossoverFreq, "Mid-High Crossover Frequency" }
        };

        return parameters;
    }
}

// Structure of compressor

struct VstCompressorBand
{
private:

    Compressor<float> __compressor;

public:

    // Elements of compressor

    AudioParameterChoice* ratio{ nullptr };

    AudioParameterBool* bypassed{ nullptr };

    AudioParameterFloat* attack{ nullptr };
    AudioParameterFloat* release{ nullptr };
    AudioParameterFloat* threshold{ nullptr };

    // Functions of the compressor itself

    void prepare(const ProcessSpec& processSpec)
    {
        __compressor.prepare(processSpec);
    }

    void updateVstCompressorSettings()
    {
        __compressor.setAttack(attack->get());
        __compressor.setRelease(release->get());
        __compressor.setThreshold(threshold->get());
        __compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue());
    }

    void processing(AudioBuffer<float>& buffer)
    {
        auto audioBlock = AudioBlock<float>(buffer);
        auto context = ProcessContextReplacing<float>(audioBlock);

        context.isBypassed = bypassed->get();

        __compressor.process(context);
    }
};

// Class of processor compressor

class EclistarVSTAudioProcessor  : public AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public AudioProcessorARAExtension
                            #endif
{
public:

    EclistarVSTAudioProcessor();
    ~EclistarVSTAudioProcessor() override;

    //==============================================================================

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();

    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };


private:
    //==============================================================================
    // The following are the elements of the compressor, Linkwitz filter

    VstCompressorBand _compressor;

    using Filter = juce::dsp::LinkwitzRileyFilter<float>;
    Filter _LP,
           _HP;

    juce::AudioParameterFloat* _lowMidCrossover{ nullptr };
    std::array<juce::AudioBuffer<float>, 2> _multiFilterBuffers;

    //==============================================================================

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EclistarVSTAudioProcessor)
};
