#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


class EclistarVSTAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EclistarVSTAudioProcessorEditor (EclistarVSTAudioProcessor&);
    ~EclistarVSTAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    
    EclistarVSTAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EclistarVSTAudioProcessorEditor)
};
