#pragma once

#include <JuceHeader.h>

using namespace juce;
using namespace std;


struct VstCompressorBand
{
private:

    dsp::Compressor<float> _compressor;

public:
    
    AudioParameterChoice* ratio{ nullptr };

    AudioParameterBool* bypassed{ nullptr };

    AudioParameterFloat* attack{ nullptr };
    AudioParameterFloat* release{ nullptr };
    AudioParameterFloat* threshold{ nullptr };

    //==============================================================================

    void prepare(const dsp::ProcessSpec& processSpec)
    {
        _compressor.prepare(processSpec);
    }

    void updateVstCompressorSettings()
    {
        _compressor.setAttack(attack->get());
        _compressor.setRelease(release->get());
        _compressor.setThreshold(threshold->get());
        _compressor.setRatio(ratio->getCurrentChoiceName().getFloatValue());
    }

    void processing(AudioBuffer<float>& buffer)
    {
        auto audioBlock = dsp::AudioBlock<float>(buffer);
        auto context = dsp::ProcessContextReplacing<float>(audioBlock);

        context.isBypassed = bypassed->get();

        _compressor.process(context);
    }
};


class EclistarVSTAudioProcessor  : public AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
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
    /*
    dsp::Compressor<float> _compressor;


    AudioParameterFloat* _attack{ nullptr };
    AudioParameterFloat* _release{ nullptr };
    AudioParameterFloat* _threshold{ nullptr };

    AudioParameterChoice* _ratio{ nullptr };

    AudioParameterBool* _bypassed{ nullptr };

    */

    VstCompressorBand compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EclistarVSTAudioProcessor)
};
