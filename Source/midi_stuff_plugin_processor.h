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

     midi_stuff::processors::midi_note_dedup             dedup_ch_1;
     midi_stuff::processors::midi_note_dedup             dedup_ch_2;
     midi_stuff::processors::midi_note_dedup             dedup_ch_3;
     midi_stuff::processors::midi_note_dedup             dedup_ch_4;
     midi_stuff::processors::midi_note_dedup             dedup_ch_5;
     midi_stuff::processors::midi_note_dedup             dedup_ch_6;
     midi_stuff::processors::midi_note_dedup             dedup_ch_7;
     midi_stuff::processors::midi_note_dedup             dedup_ch_8;
     midi_stuff::processors::midi_note_dedup             dedup_ch_9;
     midi_stuff::processors::midi_note_dedup             dedup_ch_10;
     midi_stuff::processors::midi_note_dedup             dedup_ch_11;
     midi_stuff::processors::midi_note_dedup             dedup_ch_12;
     midi_stuff::processors::midi_note_dedup             dedup_ch_13;
     midi_stuff::processors::midi_note_dedup             dedup_ch_14;
     midi_stuff::processors::midi_note_dedup             dedup_ch_15;
     midi_stuff::processors::midi_note_dedup             dedup_ch_16;
     //midi_stuff::processors::midi_lowest_notes           lowest;
     //midi_stuff::processors::midi_highest_notes          highest;
     //midi_stuff::processors::midi_lowest_notes           lowest_2;
     midi_stuff::processors::midi_chord_intervals        chord_intervals;
     midi_stuff::processors::midi_note_dedup             dedup_arp_sequence_notes;
     midi_stuff::processors::midi_note_arpeggiator       arppegiator;

     midi_stuff::processors::midi_channel_join           join;
     midi_stuff::processors::midi_channel_note_output    output;
     
     // UI
     juce::AudioProcessorValueTreeState                  parameters;

};