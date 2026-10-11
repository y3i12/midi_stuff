#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <cstdint>

namespace {

using namespace midi_stuff;
using namespace midi_stuff::processors;

} // namespace

TEST_SUITE( "value_change" ) {

TEST_CASE( "integers in, any arithmetic type out" ) {
    events::value_change value( events::value_change_event_t( static_cast< uint16_t >( 0x0891 ) ) ); // a scale code

    CHECK( value.is_integer( ) );
    CHECK( value.get< int32_t  >( ) == 0x0891 );
    CHECK( value.get< uint16_t >( ) == 0x0891 );
    CHECK( value.get< float    >( ) == doctest::Approx( 2193.0f ) );
}

TEST_CASE( "reals in, any arithmetic type out" ) {
    events::value_change value( events::value_change_event_t( 0.75 ) );

    CHECK_FALSE( value.is_integer( ) );
    CHECK( value.get< float   >( ) == doctest::Approx( 0.75f ) );
    CHECK( value.get< int32_t >( ) == 0 );
}

TEST_CASE( "set replaces the value and its kind" ) {
    events::value_change value;
    CHECK( value.is_integer( ) );
    CHECK( value.get< int32_t >( ) == 0 );

    value.set( 2.5f );
    CHECK_FALSE( value.is_integer( ) );

    value.set( 7u );
    CHECK( value.is_integer( ) );
    CHECK( value.get< uint32_t >( ) == 7u );
}

TEST_CASE( "a value read as another type doesn't throw: lowest notes takes its count from an int32_t" ) {
    midi_lowest_notes lowest;
    parameter_port&   number_notes = *lowest.get_input( "number_notes" );

    lowest.process_event( events::value_change( events::value_change_event_t( static_cast< int32_t >( 3 ) ) ), number_notes, number_notes );

    CHECK( lowest.number_notes_kept == 3u );
}

} // TEST_SUITE
