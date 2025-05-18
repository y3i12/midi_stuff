#pragma once

#pragma once

#include "midi_stuff/midi_stuff.h"

class midi_stuff_processor {
public:
    midi_stuff_processor( void ) {

        using binder = midi_stuff::processors::base_processor::binder;


        binder bind_it(
            {
                { &input,              "channel_notes",             &split,                    "notes"           },

                { &split,              "channel_1",                 &dedup_ch_1,               "notes"           },
                { &split,              "channel_2",                 &dedup_ch_2,               "notes"           },
                { &split,              "channel_3",                 &dedup_ch_3,               "notes"           },
                { &split,              "channel_4",                 &dedup_ch_4,               "notes"           },
                //{ &split,              "channel_5",                 &dedup_ch_5,               "notes"           },
                //{ &split,              "channel_6",                 &dedup_ch_6,               "notes"           },
                //{ &split,              "channel_7",                 &dedup_ch_7,               "notes"           },
                //{ &split,              "channel_8",                 &dedup_ch_8,               "notes"           },
                //{ &split,              "channel_9",                 &dedup_ch_9,               "notes"           },
                //{ &split,              "channel_10",                &dedup_ch_10,              "notes"           },
                //{ &split,              "channel_11",                &dedup_ch_11,              "notes"           },
                //{ &split,              "channel_12",                &dedup_ch_12,              "notes"           },
                //{ &split,              "channel_13",                &dedup_ch_13,              "notes"           },
                //{ &split,              "channel_14",                &dedup_ch_14,              "notes"           },
                //{ &split,              "channel_15",                &dedup_ch_15,              "notes"           },
                //{ &split,              "channel_16",                &dedup_ch_16,              "notes"           },

                { &dedup_ch_1,         "notes",                     &chord_intervals,          "root_note"       },
                { &dedup_ch_2,         "notes",                     &chord_intervals,          "chord_notes"     },
                { &dedup_ch_3,         "notes",                     &chord_intervals,          "pattern_notes"   },
                { &dedup_ch_4,         "notes",                     &arppegiator,              "sequence_notes"  },

                { &chord_intervals,    "chord_notes",               &arppegiator,              "chord_notes"     },

                { &chord_intervals,    "chord_notes",               &wormhole,                 "channel_1"       },
                { &chord_intervals,    "pattern_degrees",           &wormhole,                 "channel_2"       },
                { &arppegiator,        "notes",                     &wormhole,                 "channel_3"       },


                { &wormhole,           "channel_1",                 &join,                     "channel_1"       },
                { &wormhole,           "channel_2",                 &join,                     "channel_2"       },
                { &wormhole,           "channel_3",                 &join,                     "channel_3"       },

                { &join,               "channel_notes",             &output,                   "notes"           }
            }
        );
    }


    void process_midi( juce::MidiBuffer& midi_buffer ) {
        if ( input.generate_events( midi_buffer, true ) ) {
            output.set_output_midi_buffer( midi_buffer );
            output.backtrack_process( );
        }
    }

protected:
   // midi_stuff
    midi_stuff::processors::midi_channel_note_input     input;
    midi_stuff::processors::midi_channel_split          split;

    midi_stuff::processors::midi_note_dedup             dedup_ch_1;
    midi_stuff::processors::midi_note_dedup             dedup_ch_2;
    midi_stuff::processors::midi_note_dedup             dedup_ch_3;
    midi_stuff::processors::midi_note_dedup             dedup_ch_4;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_5;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_6;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_7;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_8;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_9;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_10;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_11;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_12;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_13;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_14;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_15;
    //midi_stuff::processors::midi_note_dedup             dedup_ch_16;

    midi_stuff::processors::midi_chord_intervals        chord_intervals;
    midi_stuff::processors::midi_note_dedup             dedup_arp_sequence_notes;
    midi_stuff::processors::midi_note_arpeggiator       arppegiator;

    midi_stuff::processors::midi_note_wormhole          wormhole;
    midi_stuff::processors::midi_channel_join           join;
    midi_stuff::processors::midi_channel_note_output    output;
};