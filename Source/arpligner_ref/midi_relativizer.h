#pragma once

#include <JuceHeader.h>

#include <string>
#include <set>

#include "chordcat.hpp"
#include "note_map.h"

namespace gig_arranger {

class midi_relativizer {
public:
    enum note_mode_types {
        k_default,
        k_root_tells_notes_off,
    };

    void process_midi_message( juce::MidiMessage& midi_messages );
    void prepare_to_process( void );
    void note_on( chordcat::t::note note );
    void note_off( chordcat::t::note note );
    bool control_message( int8_t control, int8_t value );
    void all_notes_off( void );
    void all_sounds_off( void );
    void end_process( void );

    const std::string& get_chord_name( void );
    
    bool is_dirty( void );

private:
    bool dirty;
    note_map note_map;
    chordcat::t::chord chord;
    std::string chord_name;
    std::ostringstream oss;
    chordcat::t::note key;
};

};
