#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================================

EclistarVSTAudioProcessorEditor::EclistarVSTAudioProcessorEditor(EclistarVSTAudioProcessor& processor)
    : AudioProcessorEditor(&processor), audio_processor(processor)
{
    setSize(600, 500);
}

EclistarVSTAudioProcessorEditor::~EclistarVSTAudioProcessorEditor()
{
}

//==============================================================================================

void EclistarVSTAudioProcessorEditor::paint(Graphics& graphics)
{
    graphics.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    graphics.setColour(Colours::white);
    graphics.setFont(15.0f);
    graphics.drawFittedText("Empty", getLocalBounds(), Justification::centred, 1);
}

void EclistarVSTAudioProcessorEditor::resized()
{
}
