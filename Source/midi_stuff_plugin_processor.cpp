#include "midi_stuff_plugin_processor.h"




midi_stuff_plugin_processor::midi_stuff_plugin_processor( void ) : plugin_processor( ) {

    midi_stuff::note_processor::midi_note_processor::binder::binder( {
        { input,  keeper },
        { keeper, lowest  },
        { lowest,  highest   },
        { highest,   lowest_2  },
        { lowest_2,  output }
    } );

    lowest.number_notes_kept   = 3;
    highest.number_notes_kept  = 2;
    lowest_2.number_notes_kept = 1;
}

midi_stuff_plugin_processor::~midi_stuff_plugin_processor( void ) {

}

void midi_stuff_plugin_processor::process_midi( juce::MidiBuffer& midi_buffer ) {
    std::for_each(
        midi_buffer.begin( ),
        midi_buffer.end( ),
        [ & ]( juce::MidiBufferIterator::reference buffer_itr ) {
            auto midi_message = buffer_itr.getMessage( );
            if ( midi_message.isNoteOn( ) ) {
                input.note_on(
                    midi_message.getChannel( ),
                    midi_message.getNoteNumber( ),
                    midi_message.getVelocity( )
                );
            } else if ( midi_message.isNoteOff( ) ) {
                input.note_off(
                    midi_message.getChannel( ),
                    midi_message.getNoteNumber( ),
                    midi_message.getVelocity( )
                );
            }
        }
    );
    //*/
    midi_buffer.clear( );
    output.set_output_midi_buffer( midi_buffer );
    output.backtrack_process( );
    //*/
}





//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter( ) {
    return new midi_stuff_plugin_processor( );
}
