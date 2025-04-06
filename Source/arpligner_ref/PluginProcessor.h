/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "midi_relativizer.h"
#include "midi_stuff/midi_note_input.h"
#include "midi_stuff/sorted_note_keeper.h"

//==============================================================================
/**
*/
class SmarpeggiatorAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SmarpeggiatorAudioProcessor();
    ~SmarpeggiatorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

   
    void process_midi( juce::MidiBuffer& midi_buffer, bool read_only );
    void processBlock( juce::AudioBuffer<float>&, juce::MidiBuffer& midi_buffer ) override;
    void processBlockBypassed( juce::AudioBuffer<float>&, juce::MidiBuffer& midi_buffer ) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const std::string& get_chord( void ) { return midi_relativizers[0].get_chord_name( ); }

public:
    std::atomic< bool > chord_changed;

protected:
    gig_arranger::midi_relativizer              midi_relativizers[16];
    midi_stuff::midi_note_input                 note_input_dispatcher;
    midi_stuff::sorted_note_keeper< 128 >       note_keeper;

    midi_stuff::types::listener_handle_t        dispatcher_handle;
    juce::MidiBuffer*                           current_midi_buffer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SmarpeggiatorAudioProcessor)

};
