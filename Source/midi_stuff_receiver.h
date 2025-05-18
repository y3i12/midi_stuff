#pragma once

#include "midi_stuff/midi_stuff.h"

class midi_stuff_receiver {
public:
    midi_stuff_receiver( void ) {

        using binder = midi_stuff::processors::base_processor::binder;


        binder bind_it(
            {
                { &wormhole,           "channel_1",                 &join,                     "channel_1"       },
                { &wormhole,           "channel_2",                 &join,                     "channel_2"       },
                { &wormhole,           "channel_3",                 &join,                     "channel_3"       },

                { &join,               "channel_notes",             &output,                   "notes"           }
            }
        );
    }


    void process_midi( juce::MidiBuffer& midi_buffer ) {
        output.set_output_midi_buffer( midi_buffer );
        midi_buffer.clear( );
        output.backtrack_process( );
    }

protected:
    midi_stuff::processors::midi_note_wormhole          wormhole;
    midi_stuff::processors::midi_channel_join           join;
    midi_stuff::processors::midi_channel_note_output    output;
};