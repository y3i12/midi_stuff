#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <string>
#include <vector>

// Pin known bugs with `* doctest::should_fail( )`: the suite stays green while they're broken,
// and the test flips to "unexpectedly passed" as soon as the bug is fixed - then drop the decorator.

namespace {

using namespace midi_stuff;

std::string guess( std::initializer_list< int > notes, int root ) {
    containers::ascending_midi_note_list list;
    for ( int note : notes ) {
        list.insert( midi_note( static_cast< int8_t >( note ) ) );
    }

    return chord_data::midi_stuff_chord_guesser::get_best_fitting_named_noteset( list, static_cast< note_t >( root ) ).name;
}

} // namespace

TEST_SUITE( "chord_data" ) {

TEST_CASE( "set_scale_code is relative to the root and skips it" ) {
    chord_data::named_noteset::scale_code_t code;
    std::vector< midi_note > notes = { midi_note( 67 ), midi_note( 71 ), midi_note( 74 ) }; // G B D

    chord_data::named_noteset::set_scale_code( notes, code, 7 );

    CHECK( code.test( 4 ) );         // B = major third of G
    CHECK( code.test( 7 ) );         // D = fifth of G
    CHECK_FALSE( code.test( 0 ) );   // root is never encoded
    CHECK( code.count( ) == 2 );
}

TEST_CASE( "triads are identified" ) {
    CHECK( guess( { 0, 4, 7 }, 0 ) == "major" );
    CHECK( guess( { 0, 3, 7 }, 0 ) == "minor" );
    CHECK( guess( { 0, 3, 6 }, 0 ) == "dim"   );
    CHECK( guess( { 0, 4, 8 }, 0 ) == "aug"   );
}

TEST_CASE( "sevenths and extensions are identified" ) {
    CHECK( guess( { 0, 4, 7, 10    }, 0 ) == "7"    );
    CHECK( guess( { 0, 4, 7, 11    }, 0 ) == "maj7" );
    CHECK( guess( { 0, 3, 7, 10    }, 0 ) == "m7"   );
    CHECK( guess( { 0, 4, 7, 11, 2 }, 0 ) == "maj9" );
}

TEST_CASE( "identification is transposition invariant" ) {
    CHECK( guess( { 7, 11, 2 }, 7 ) == "major" ); // G major
    CHECK( guess( { 9, 0,  4 }, 9 ) == "minor" ); // A minor
}

} // TEST_SUITE

TEST_SUITE( "midi_note" ) {

TEST_CASE( "uint16_t packing round-trips" ) {
    midi_note original( 60, 100 );
    uint16_t  packed = original;

    midi_note unpacked;
    unpacked = packed;

    CHECK( unpacked.note     == 60  );
    CHECK( unpacked.velocity == 100 );
}

} // TEST_SUITE
