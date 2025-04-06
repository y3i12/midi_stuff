#pragma once

#include "chordcat_types.hpp"

#include <array>
#include <sstream>
#include <ostream>
#include <string>

namespace chordcat {


static const std::array< std::string, 12 > degrees        = { "root",   "b2", "2", "b3",  "3",  "4",  "b5",  "5", "b6",  "6",  "b7", "7"  };
static const std::array< std::string, 12 > compound_tones = { "octave", "b9", "9", "b10", "10", "11", "#11", "5", "b13", "13", "B7", "7"  };
static const std::array< std::string, 12 > note_names     = { "C",      "C#", "D", "Eb",  "E",  "F",  "F#",  "G", "Ab",  "A",  "Bb", "B" };


inline std::string key_number_to_note_name( const size_t index ) {
    return note_names[ index % note_names.size() ];
}

namespace t {
struct chord {
    t::note                         root;
    std::string                     base_name;
    size_t                          num_accidentals;
    std::vector< t::note >          tones;
    std::vector< t::note >          extra_tones;
    std::vector< t::note >          omitted_tones;
    const chordcat::chord_db_entry* db_entry;

    friend bool operator<( const chordcat::t::chord& x, const chordcat::t::chord& y ) {
        return x.num_accidentals < y.num_accidentals;
    }

    std::string to_string( void ) {
        static std::ostringstream oss;
        oss.str( "" );

        oss << key_number_to_note_name( root ) << base_name;

        size_t accidentals_count = num_accidentals;
        if ( accidentals_count > 0 ) {
            oss << "(";
        }

        for ( auto tone : omitted_tones ) {
            oss << "no" << degrees[ tone % 12 ] << ( --accidentals_count == 0 ? ")" : "," );
        }
        
        for ( auto tone : extra_tones ) {
            oss << compound_tones[tone % 12] << "[" << (int)tone << "]" << ( --accidentals_count == 0 ? ")" : "," );
        }

        return oss.str();
    }
};
}
}