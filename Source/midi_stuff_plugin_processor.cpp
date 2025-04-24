#include "midi_stuff_plugin_processor.h"
#include "BinaryData.h"

midi_stuff_plugin_processor::midi_stuff_plugin_processor( void ) : 
    plugin_processor( ), 
    parameters( *this, nullptr, juce::Identifier( "midi_stuff" ), {
        std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "lowest",   1 ), "Lowest",   0, 12, 12 ),
        std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "highest",  1 ), "Highest",  0, 12, 12 ),
        std::make_unique<juce::AudioParameterInt>( juce::ParameterID( "lowest_2", 1 ), "Lowest 2", 0, 12, 12 )
    } ) {
    // midi_stuff
    using binder = midi_stuff::processors::base_processor::binder;

    // input -> split -> (ch1) dedup_lhl.notes -> lowest.notes -> highest.notes -> lowest_2.notes -> join.channel_1 -> output
    //            |
    //            +----> (ch2) dedup_arp_chord_notes ----> arp.chord_notes ----\
    //            |                                                             +-> arp.notes -> join.channel_2 -> output
    //            +----> (ch3) dedup_arp_sequence_notes -> arp.sequence_notes -/
    binder bind_it( {
            { &input,                    "channel_notes", &split,                    "notes"          },
            { &split,                    "channel_1",     &dedup_lhl,                "notes"          },
            { &dedup_lhl,                "notes",         &lowest,                   "notes"          },
            { &lowest,                   "notes",         &highest,                  "notes"          },
            { &highest,                  "notes",         &lowest_2,                 "notes"          },
            { &lowest_2,                 "notes",         &join,                     "channel_1"      },
            { &split,                    "channel_2",     &dedup_arp_chord_notes,    "notes"          },
            { &split,                    "channel_3",     &dedup_arp_sequence_notes, "notes"          },
            { &dedup_arp_chord_notes,    "notes",         &arppegiator,              "chord_notes"    },
            { &dedup_arp_sequence_notes, "notes",         &arppegiator,              "sequence_notes" },
            { &arppegiator,              "notes",         &join,                     "channel_2"      },
            { &join,                     "channel_notes", &output,                   "notes"          }
    } );

    // ui
    magicState.setGuiValueTree( BinaryData::midi_stuff_xml, BinaryData::midi_stuff_xmlSize );

    param_lowest_n   = parameters.getRawParameterValue( "lowest"   );
    param_highest_n  = parameters.getRawParameterValue( "highest"  );
    param_lowest_2_n = parameters.getRawParameterValue( "lowest_2" );

}

midi_stuff_plugin_processor::~midi_stuff_plugin_processor( void ) {

}

void midi_stuff_plugin_processor::process_midi( juce::MidiBuffer& midi_buffer ) {
    if ( input.generate_events( midi_buffer, true ) ) {
        lowest.number_notes_kept   = static_cast< uint8_t >( *param_lowest_n   );
        highest.number_notes_kept  = static_cast< uint8_t >( *param_highest_n  );
        lowest_2.number_notes_kept = static_cast< uint8_t >( *param_lowest_2_n );
    
        output.set_output_midi_buffer( midi_buffer );
        output.backtrack_process( );
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter( ) {
    return new midi_stuff_plugin_processor( );
}
