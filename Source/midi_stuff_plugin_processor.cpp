#include "midi_stuff_plugin_processor.h"
#include "BinaryData.h"

midi_stuff_plugin_processor::midi_stuff_plugin_processor( void ) : 
    plugin_processor( ), 
    parameters( *this, nullptr, juce::Identifier( "midi_stuff" ), {
        /*std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "lowest",   1 ), "Lowest",   0, 12, 12 ),
        std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "highest",  1 ), "Highest",  0, 12, 12 ),
        std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "lowest_2", 1 ), "Lowest 2", 0, 12, 12 )*/
    } ) {
    // midi_stuff
    using binder = midi_stuff::processors::base_processor::binder;


    binder bind_it( {
            { &input,              "channel_notes",             &split,                    "notes"           },

            { &split,              "channel_1",                 &dedup_ch_1,               "notes"           },
            { &split,              "channel_2",                 &dedup_ch_2,               "notes"           },
            { &split,              "channel_3",                 &dedup_ch_3,               "notes"           },
            { &split,              "channel_4",                 &dedup_ch_4,               "notes"           },
            { &split,              "channel_5",                 &dedup_ch_5,               "notes"           },
            { &split,              "channel_6",                 &dedup_ch_6,               "notes"           },
            { &split,              "channel_7",                 &dedup_ch_7,               "notes"           },
            { &split,              "channel_8",                 &dedup_ch_8,               "notes"           },
            { &split,              "channel_9",                 &dedup_ch_9,               "notes"           },
            { &split,              "channel_10",                &dedup_ch_10,              "notes"           },
            { &split,              "channel_11",                &dedup_ch_11,              "notes"           },
            { &split,              "channel_12",                &dedup_ch_12,              "notes"           },
            { &split,              "channel_13",                &dedup_ch_13,              "notes"           },
            { &split,              "channel_14",                &dedup_ch_14,              "notes"           },
            { &split,              "channel_15",                &dedup_ch_15,              "notes"           },
            { &split,              "channel_16",                &dedup_ch_16,              "notes"           },

            { &dedup_ch_1,         "notes",                     &chord_intervals,          "root_note"       },
            { &dedup_ch_2,         "notes",                     &chord_intervals,          "chord_notes"     },
            { &dedup_ch_3,         "notes",                     &chord_intervals,          "pattern_notes"   },
            { &dedup_ch_4,         "notes",                     &arppegiator,              "sequence_notes"  },

            { &chord_intervals,    "chord_notes",               &arppegiator,              "chord_notes"     },
            
            /*
            { &chord_intervals,    "chord_notes",               &join,                     "channel_1"       },
            { &chord_intervals,    "pattern_degrees",           &join,                     "channel_2"       },
            { &arppegiator,        "notes",                     &join,                     "channel_3"       },
            */
            { &arppegiator,        "notes",                     &join,                     "channel_1"       },
            { &join,               "channel_notes",             &output,                   "notes"           }
    } );

    // ui
    // magicState.setGuiValueTree( BinaryData::midi_stuff_xml, BinaryData::midi_stuff_xmlSize );
}

midi_stuff_plugin_processor::~midi_stuff_plugin_processor( void ) {

}

void midi_stuff_plugin_processor::process_midi( juce::MidiBuffer& midi_buffer ) {
    if ( input.generate_events( midi_buffer, true ) ) {
        output.set_output_midi_buffer( midi_buffer );
        output.backtrack_process( );
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter( ) {
    return new midi_stuff_plugin_processor( );
}
