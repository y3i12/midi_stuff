#pragma once

#include "chord.hpp"
#include "chord_db.hpp"

#include <algorithm>
#include <ranges>
#include <set>
#include <string>
#include <vector>

namespace chordcat {
namespace utils {

inline std::vector< std::string > key_numbers_to_note_names( const std::vector< size_t >& indices ) {
    std::vector< std::string > result = { };

    for ( auto index : indices ) {
        result.push_back( note_names[ index % note_names.size() ] );
    }

    return result;
}



inline t::note get_note_distance( t::note root, t::note other ) {
    if ( root >= other ) {
        return 12 + ( other - root ) % 12;
    }

    return other - root;
}


inline t::chord& make_chord_of( t::chord&                       chord,
                                t::note                         root,
                                const chordcat::chord_db_entry& entry,
                                const t::note_set&              input_intervals
                                ) {
    chord.root      = root;
    chord.base_name = entry.name;
    chord.db_entry  = &entry;

    const t::note_set& chord_intervals = entry.notes;
    auto ii = input_intervals.begin();
    auto ci = chord_intervals.begin();
    
    while ( ii != input_intervals.end( ) && ci != chord_intervals.end( ) ) {
        if ( *ii == *ci ) { // in both
            chord.tones.push_back( *ii );
            ++ii;++ci;
        } else if ( *ii < *ci ) { // only in input intervals
            chord.extra_tones.push_back( *ii );
            ++ii;
        } else if ( *ii > *ci ) { // only in chord intervals
            chord.omitted_tones.push_back( *ci );
            ++ci;
        }
    }

    for ( ; ii != input_intervals.end( ); ++ii ) { chord.extra_tones.push_back(   *ii ); }
    for ( ; ci != chord_intervals.end( ); ++ci ) { chord.omitted_tones.push_back( *ci ); }

    chord.num_accidentals = chord.extra_tones.size( ) + chord.omitted_tones.size( );

    return chord;
}

inline void insert_chords( t::note                  root,
                           const t::note_set& intervals,
                           std::multiset< t::chord >& res ) {
    std::set< t::chord > temp;
    t::chord chord = {};
    std::ranges::for_each( chord_db, [&]( auto& entry ) {
        chord = {};
        temp.insert( make_chord_of( chord, root, entry, intervals ) );
    } );
    if ( !temp.empty( ) ) {
        res.insert( *temp.begin( ) );
    }
}


inline void set_chord( t::note             root,
                       const t::note_set&  intervals,
                       t::chord&           chord ) {
    std::set< t::chord > temp;

    std::ranges::for_each( chord_db, [&]( auto& entry ) {
        chord = {};
        temp.insert( make_chord_of( chord, root, entry, intervals ) );
    } );

    if ( !temp.empty( ) ) {
        chord = *temp.begin( );
    }
}



inline std::multiset< t::chord > name_that_chord( const std::vector< size_t >& indices ) {
    t::note_set notes = {};

    for ( auto index : indices ) {
        notes.insert( index % 12 );
    }

    std::multiset< t::chord > result = {};

    for ( auto root : notes ) {
        t::note_set intervals = {};
        for (auto other : notes) {
            if (other == root) {
                continue;
            }

            intervals.insert( get_note_distance( root, other ) );
        }

        insert_chords( root, intervals, result );
    }

    return result;
}

// optimization overload - avoids copying stuff around
template < typename T >
void name_that_chord( const T& notes, std::multiset< t::chord >& chords ) {
    chords.clear( );

    // this is N^2 and most likely can be improved
    for ( auto root : notes ) {
        t::note_set intervals = {};
        for ( auto other : notes ) {
            if ( other == root ) {
                continue;
            }

            intervals.insert( get_note_distance( root.first, other.first ) );
        }

        insert_chords( root.first, intervals, chords );
    }
}

// optimization overload - avoids copying stuff around
template < typename T >
void name_that_chord( const t::note root, const T& notes, t::chord& chord ) {

    t::note_set intervals = {};
    for ( auto other : notes ) {
        if ( other.first == root ) {
            continue;
        }

        intervals.insert( get_note_distance( root, other.first ) );
    }

    set_chord( root, intervals, chord );
}

} // namespace utils
} // namespace chordcat