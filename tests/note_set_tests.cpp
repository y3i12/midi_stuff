#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <utility>
#include <vector>

namespace {

using midi_stuff::midi_note;
using midi_stuff::containers::note_set;

std::vector< int > ascending( const note_set& set ) {
    std::vector< int > result;
    for ( const midi_note& note : set ) result.push_back( note.note );
    return result;
}

std::vector< int > descending( const note_set& set ) {
    std::vector< int > result;
    for ( const midi_note& note : set.descending( ) ) result.push_back( note.note );
    return result;
}

note_set make( std::initializer_list< std::pair< int, int > > notes ) {
    note_set set;
    for ( auto [ note, velocity ] : notes ) set.insert( midi_note( static_cast< int8_t >( note ), static_cast< uint8_t >( velocity ) ) );
    return set;
}

} // namespace

TEST_SUITE( "note_set" ) {

TEST_CASE( "insert, erase and contains" ) {
    note_set set;
    CHECK( set.empty( ) );

    CHECK(       set.insert( midi_note( 60, 90 ) ) );
    CHECK_FALSE( set.insert( midi_note( 60, 10 ) ) ); // already there: velocity kept
    CHECK( set.contains( 60 ) );
    CHECK( set.velocity( 60 ) == 90 );
    CHECK( set.size( ) == 1 );

    CHECK(       set.erase( midi_note( 60 ) ) );
    CHECK_FALSE( set.erase( midi_note( 60 ) ) );
    CHECK( set.empty( ) );
    CHECK( set.velocity( 60 ) == 0 );
}

TEST_CASE( "iterates in order across the word boundary" ) {
    const note_set set = make( { { 127, 1 }, { 0, 1 }, { 64, 1 }, { 63, 1 }, { 65, 1 } } );

    CHECK( ascending(  set ) == std::vector< int >{ 0, 63, 64, 65, 127 } );
    CHECK( descending( set ) == std::vector< int >{ 127, 65, 64, 63, 0 } );
    CHECK( set.front( ).note == 0   );
    CHECK( set.back(  ).note == 127 );
}

TEST_CASE( "iterating an empty set does nothing" ) {
    const note_set set;
    CHECK( ascending(  set ).empty( ) );
    CHECK( descending( set ).empty( ) );
    CHECK( set.begin( ) == set.end( ) );
}

TEST_CASE( "diff gives note offs then note ons, with the right velocities" ) {
    const note_set from = make( { { 60, 10 }, { 64, 20 }, { 100, 30 } } );
    const note_set to   = make( { { 60, 99 }, { 67, 40 }, { 70,  50 } } );

    std::vector< std::pair< int, int > > offs, ons;
    note_set::diff( from, to,
        [ & ] ( const midi_note& note ) { offs.emplace_back( note.note, note.velocity ); },
        [ & ] ( const midi_note& note ) { ons.emplace_back(  note.note, note.velocity ); }
    );

    CHECK( offs == std::vector< std::pair< int, int > >{ { 64, 20 }, { 100, 30 } } );
    CHECK( ons  == std::vector< std::pair< int, int > >{ { 67, 40 }, { 70,  50 } } );
}

TEST_CASE( "equality includes velocities, erased notes leave no trace" ) {
    note_set a = make( { { 60, 10 } } );
    note_set b = make( { { 60, 10 } } );
    CHECK( a == b );

    b.erase( midi_note( 60 ) );
    b.insert( midi_note( 60, 11 ) );
    CHECK_FALSE( a == b );

    a.insert( midi_note( 61 ) );
    a.erase(  midi_note( 61 ) );
    b.erase(  midi_note( 60 ) );
    b.insert( midi_note( 60, 10 ) );
    CHECK( a == b );
}

TEST_CASE( "is trivially copyable" ) {
    CHECK( std::is_trivially_copyable_v< note_set  > );
    CHECK( std::is_trivially_copyable_v< midi_note > );
}

} // TEST_SUITE
