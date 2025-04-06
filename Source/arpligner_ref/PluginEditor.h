/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SmarpeggiatorAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    SmarpeggiatorAudioProcessorEditor (SmarpeggiatorAudioProcessor&);
    ~SmarpeggiatorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void timerCallback( void ) override;
    void resized( void ) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SmarpeggiatorAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmarpeggiatorAudioProcessorEditor)
};
