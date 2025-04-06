/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "plugin_processor.h"

//==============================================================================
/**
*/
class plugin_editor  : public juce::AudioProcessorEditor
{
public:
    plugin_editor (plugin_processor&);
    ~plugin_editor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    plugin_processor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (plugin_editor)
};
