#include "midi_relativizer.h"


namespace gig_arranger {

void midi_relativizer::prepare_to_process( void ) {

}

void midi_relativizer::note_on( chordcat::t::note note ) {
    // auto note = static_cast<chordcat::t::note>( midi_message.getNoteNumber( ) );
    dirty |= note_map += note;

    if ( note < 12 ) { // forcing root
        chord.root = note;

        //*/ // for debugging purposes only
        if ( key == 0xFF ) {
            key = note;
        }
        //*/ // end of for debugging purposes only
    }
}

void midi_relativizer::note_off( chordcat::t::note note ) {
    // only update chord notes on key on?
    dirty |= note_map -= note;
}

bool midi_relativizer::control_message( int8_t control, int8_t value ) {
    if ( control == 102 ) { // set key
        key = static_cast< chordcat::t::note >( value );
        return true;
    } else if ( control == 103 ) { // note_mode
        // TODO!!  = value ;
        return true;
    }
    return false;
}

void midi_relativizer::all_notes_off( void ) {
    note_map.clear( );
    dirty = true;
}

void midi_relativizer::all_sounds_off( void ) {
    note_map.clear( );
    dirty = true;
}


void midi_relativizer::process_midi_message( juce::MidiMessage& midi_message ) {
    

    if ( midi_message.isController( ) ) {
        if ( midi_message.getControllerNumber( ) == 102 ) { // set key
            key = static_cast<chordcat::t::note>( midi_message.getControllerValue( ) );
        } else if ( midi_message.getControllerNumber( ) == 103 ) { // set key
            key = static_cast<chordcat::t::note>( midi_message.getControllerValue( ) );
        }
    } else if ( midi_message.isNoteOn( ) ) {
        auto note = static_cast<chordcat::t::note>( midi_message.getNoteNumber( ) );
        dirty |= note_map += note;
        if ( note < 12 ) { // forcing root
            chord.root = note;

            //*/ // for debugging purposes only
            if ( key == 0xFF ) {
                key = note;
            }
            //*/ // end of for debugging purposes only
        }
    } else if ( midi_message.isNoteOff( ) ) {
        note_map -= static_cast<note_map::note_t>( midi_message.getNoteNumber( ) );
    }
}


void midi_relativizer::end_process( void ) {
    if ( dirty ) {
        chordcat::utils::name_that_chord( chord.root, note_map.getCounters( ), chord );
        oss.str( "" );
        oss.clear( );

        if ( note_map.hasNotes( ) ) {
            oss << chordcat::key_number_to_note_name( chord.root ) << " " << chord.base_name << " " << chord.to_string( );
        }

        chord_name = oss.str( );
    }
}

bool  midi_relativizer::is_dirty( void ) {
    return dirty;
}

const std::string& midi_relativizer::get_chord_name( void ) {
    return chord_name;
}

};