#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EclistarVSTAudioProcessorEditor::EclistarVSTAudioProcessorEditor (EclistarVSTAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

EclistarVSTAudioProcessorEditor::~EclistarVSTAudioProcessorEditor()
{
}

//==============================================================================
void EclistarVSTAudioProcessorEditor::paint (Graphics& g)
{
    
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
}

void EclistarVSTAudioProcessorEditor::resized()
{
    
}
