#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;


class EclistarVSTAudioProcessorEditor : public AudioProcessorEditor
{
public:

    EclistarVSTAudioProcessorEditor(EclistarVSTAudioProcessor&);
    ~EclistarVSTAudioProcessorEditor() override;

//==============================================================================================

    void paint(Graphics&) override;
    void resized() override;

private:

    EclistarVSTAudioProcessor& audio_processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EclistarVSTAudioProcessorEditor)
};
