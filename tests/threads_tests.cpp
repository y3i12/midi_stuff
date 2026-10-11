#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <set>
#include <thread>
#include <vector>

namespace {

using namespace midi_stuff;
using namespace midi_stuff::processors;
using binder = base_processor::binder;

// every word holds the same counter: a torn read shows up as words that disagree
struct payload {
    std::array< uint32_t, 37 > words{ };

    explicit payload( uint32_t value = 0 ) { words.fill( value ); }

    bool consistent( void ) const {
        return std::all_of( words.begin( ), words.end( ), [ & ] ( uint32_t word ) { return word == words[ 0 ]; } );
    }
};

struct note_sink : public base_processor {
    std::set< int > held;

    note_sink( void ) : base_processor( { { "notes", k_midi_note, 0 } }, { } ) { }

    void process_event( const events::note_on&  event, parameter_port&, parameter_port& ) override { held.insert( event.note ); }
    void process_event( const events::note_off& event, parameter_port&, parameter_port& ) override { held.erase(  event.note ); }
};

} // namespace

TEST_SUITE( "threads" ) {

TEST_CASE( "seqlock_object: read returns what was written, version tracks writes" ) {
    threads::seqlock_object< payload > shared;
    payload                            snapshot( 99 );

    CHECK( shared.read( snapshot ) == 0 );
    CHECK( snapshot.words[ 0 ] == 0 );       // starts zeroed

    const auto version = shared.write( payload( 7 ) );
    CHECK( version == shared.version( ) );
    CHECK( version % 2 == 0 );

    CHECK( shared.read( snapshot ) == version );
    CHECK( snapshot.consistent( ) );
    CHECK( snapshot.words[ 0 ] == 7 );
}

TEST_CASE( "seqlock_object: readers never see a torn object" ) {
    threads::seqlock_object< payload > shared;

    constexpr uint32_t   k_writes = 200000;
    std::atomic< bool >  writing{ true };
    std::atomic< size_t > torn{ 0 };
    std::atomic< size_t > backwards{ 0 };
    std::atomic< size_t > reads{ 0 };

    auto reader = [ & ] {
        payload  snapshot;
        uint32_t last = 0;

        while ( writing ) {
            shared.read( snapshot );
            if ( !snapshot.consistent( ) )    ++torn;
            if ( snapshot.words[ 0 ] < last ) ++backwards;
            last = snapshot.words[ 0 ];
            ++reads;
        }
    };

    std::thread reader_1( reader );
    std::thread reader_2( reader );

    for ( uint32_t i = 1; i <= k_writes; ++i ) {
        shared.write( payload( i ) );
    }
    writing = false;

    reader_1.join( );
    reader_2.join( );

    payload final_state;
    shared.read( final_state );

    CHECK( reads > 0 );
    CHECK( torn == 0 );
    CHECK( backwards == 0 );
    CHECK( final_state.words[ 0 ] == k_writes );
}

TEST_CASE( "seqlock_object: two writers don't interleave" ) {
    threads::seqlock_object< payload > shared;

    constexpr uint32_t    k_writes = 100000;
    std::atomic< bool >   writing{ true };
    std::atomic< size_t > torn{ 0 };

    std::thread reader( [ & ] {
        payload snapshot;
        while ( writing ) {
            shared.read( snapshot );
            if ( !snapshot.consistent( ) ) ++torn;
        }
    } );

    auto writer = [ & ] ( uint32_t base ) {
        for ( uint32_t i = 0; i < k_writes; ++i ) shared.write( payload( base + i ) );
    };

    std::thread writer_1( writer, 0u );
    std::thread writer_2( writer, 1u << 24 );
    writer_1.join( );
    writer_2.join( );
    writing = false;
    reader.join( );

    CHECK( torn == 0 );
    CHECK( shared.version( ) == 2 * 2 * k_writes ); // every write moved the sequence by 2
}

TEST_CASE( "wormhole: notes flushed by one instance come out of another" ) {
    midi_note_wormhole sender;
    midi_note_wormhole receiver;
    note_sink          sink;

    binder bind_it( { { &receiver, "channel_5", &sink, "notes" } } );

    parameter_port& sender_input = *sender.get_input( "channel_5" );
    sender.process_event( events::note_on( 60, 100 ), sender_input, sender_input );
    sender.process_event( events::note_on( 64, 100 ), sender_input, sender_input );
    sender.process( );                                   // flush

    sink.backtrack_process( );                           // receiver reads
    CHECK( sink.held == std::set< int >{ 60, 64 } );

    sender.process_event( events::note_off( 60, 0 ), sender_input, sender_input );
    sender.process( );

    sink.backtrack_process( );
    CHECK( sink.held == std::set< int >{ 64 } );

    sender.all_notes_off( );
    sender.process( );
    sink.backtrack_process( );
    CHECK( sink.held.empty( ) );

    sink.backtrack_process( );                           // nothing new: nothing changes
    CHECK( sink.held.empty( ) );
}

} // TEST_SUITE
