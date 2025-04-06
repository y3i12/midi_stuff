#pragma once

#include "plugin_processor.h"
#include "midi_stuff/midi_stuff.h"

class midi_stuff_plugin_processor : public plugin_processor {
public:
    midi_stuff_plugin_processor( void );
    ~midi_stuff_plugin_processor( void ) override;


    void process_midi( juce::MidiBuffer& midi_buffer ) override;

 protected:
     midi_stuff::note_processor::midi_note_input    input;
     midi_stuff::note_processor::midi_note_keeper   keeper;
     midi_stuff::note_processor::midi_lowest_notes  lowest;
     midi_stuff::note_processor::midi_highest_notes highest;
     midi_stuff::note_processor::midi_lowest_notes  lowest_2;
     midi_stuff::note_processor::midi_note_output   output;

};