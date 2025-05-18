#pragma once

#include "plugin_processor.h"
#include "midi_stuff_processor.h"
#include "midi_stuff_receiver.h"

class midi_stuff_plugin_processor : public plugin_processor {
public:
    midi_stuff_plugin_processor( void );
    ~midi_stuff_plugin_processor( void ) override;


    void                        process_midi( juce::MidiBuffer& midi_buffer )            override;

    juce::AudioProcessorEditor* createEditor( void )                                     override;
    void                        getStateInformation( juce::MemoryBlock& destData )       override;
    void                        setStateInformation( const void* data, int sizeInBytes ) override;

 protected:
     midi_stuff_processor                               processor;
     midi_stuff_receiver                                receiver;

     // UI
     juce::AudioProcessorValueTreeState                 parameters;
     juce::AudioParameterFloat*                         is_receiver = nullptr;

};


//==============================================================================
/**
*/
class midi_stuff_plugin_editor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    midi_stuff_plugin_editor( midi_stuff_plugin_processor& );
    ~midi_stuff_plugin_editor( ) override;

    //==============================================================================
    void paint( juce::Graphics& ) override;
    void timerCallback( void ) override;
    void resized( void ) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    midi_stuff_plugin_processor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( midi_stuff_plugin_editor )
};