/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "plugin_processor.h"
#include "plugin_editor.h"

//==============================================================================
plugin_editor::plugin_editor (plugin_processor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

plugin_editor::~plugin_editor()
{
}

//==============================================================================
void plugin_editor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void plugin_editor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
