/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SmarpeggiatorAudioProcessorEditor::SmarpeggiatorAudioProcessorEditor (SmarpeggiatorAudioProcessor& p)
    : AudioProcessorEditor (&p), Timer(), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize( 400, 300 );
    startTimerHz( 30 );
}

SmarpeggiatorAudioProcessorEditor::~SmarpeggiatorAudioProcessorEditor()
{
    stopTimer( );
}

//==============================================================================
void SmarpeggiatorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (juce::FontOptions (15.0f));

     // fill the whole window white
    g.fillAll( juce::Colours::white );

    // set the current drawing colour to black
    g.setColour( juce::Colours::black );

    // set the font size and draw text to the screen
    g.setFont( 15.0f );

    g.drawFittedText( audioProcessor.get_chord().c_str(), getLocalBounds( ), juce::Justification::centred, 10 );
    // g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SmarpeggiatorAudioProcessorEditor::timerCallback( void ) {
    if ( audioProcessor.chord_changed ) {
        repaint();

        audioProcessor.chord_changed = false;
    }
}

void SmarpeggiatorAudioProcessorEditor::resized( void ) {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
