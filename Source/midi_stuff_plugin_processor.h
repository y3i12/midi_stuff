#pragma once

#include "plugin_processor.h"
#include "midi_stuff/midi_stuff.h"

class midi_stuff_plugin_processor : public plugin_processor {
public:
    midi_stuff_plugin_processor( void );
    ~midi_stuff_plugin_processor( void ) override;


    void process_midi( juce::MidiBuffer& midi_buffer ) override;

 protected:
    // midi_stuff
     midi_stuff::processors::midi_channel_note_input     input;
     midi_stuff::processors::midi_channel_split          split;

     midi_stuff::processors::midi_note_dedup             dedup_lhl; // lowest highest lowest
     midi_stuff::processors::midi_lowest_notes           lowest;
     midi_stuff::processors::midi_highest_notes          highest;
     midi_stuff::processors::midi_lowest_notes           lowest_2;

     midi_stuff::processors::midi_note_dedup             dedup_arp_chord_notes;
     midi_stuff::processors::midi_note_dedup             dedup_arp_sequence_notes;
     midi_stuff::processors::midi_note_arpeggiator       arppegiator;

     midi_stuff::processors::midi_channel_join           join;
     midi_stuff::processors::midi_channel_note_output    output;
     
     // UI
     juce::AudioProcessorValueTreeState                  parameters;

     std::atomic< float >*                               param_lowest_n   = nullptr;
     std::atomic< float >*                               param_highest_n  = nullptr;
     std::atomic< float >*                               param_lowest_2_n = nullptr;

};