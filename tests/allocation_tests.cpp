#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <atomic>
#include <cstdlib>
#include <new>

// The processing path must never allocate ( audio thread, Teensy ). operator new is replaced for the whole test
// program, but only counts while an allocation_counter is alive - setting up a graph may allocate, playing it may not.

namespace {

std::atomic< bool >   counting_allocations{ false };
std::atomic< size_t > allocation_count{ 0 };

struct allocation_counter {
    allocation_counter( void )  { allocation_count = 0; counting_allocations = true;  }
    ~allocation_counter( void ) {                       counting_allocations = false; }

    size_t count( void ) const { return allocation_count; }
};

} // namespace

void* operator new( std::size_t size ) {
    if ( counting_allocations ) ++allocation_count;

    if ( void* memory = std::malloc( size ? size : 1 ) ) return memory;

#if defined( __cpp_exceptions )
    throw std::bad_alloc( );
#else
    std::abort( );
#endif
}

void operator delete( void* memory ) noexcept              { std::free( memory ); }
void operator delete( void* memory, std::size_t ) noexcept { std::free( memory ); }

namespace {

using namespace midi_stuff;
using namespace midi_stuff::processors;
using binder = base_processor::binder;

// end of the graph: counts, without allocating
struct counting_sink : public base_processor {
    size_t ons  = 0;
    size_t offs = 0;

    counting_sink( void ) : base_processor( { { "notes", k_midi_channel_note, 0 } }, { } ) { }

    void process_event( const events::channel_note_on&,  parameter_port&, parameter_port& ) override { ++ons;  }
    void process_event( const events::channel_note_off&, parameter_port&, parameter_port& ) override { ++offs; }
};

// the same graph as midi_stuff_processor ( Source/midi_stuff_processor.h ), without JUCE
struct plugin_graph {
    midi_channel_note_input input;
    midi_channel_split      split;
    midi_note_dedup         dedup_ch_1;
    midi_note_dedup         dedup_ch_2;
    midi_note_dedup         dedup_ch_3;
    midi_note_dedup         dedup_ch_4;
    midi_chord_intervals    chord_intervals;
    midi_note_arpeggiator   arpeggiator;
    midi_note_wormhole      wormhole;
    midi_channel_join       join;
    counting_sink           sink;

    plugin_graph( void ) {
        binder bind_it( {
            { &input,           "channel_notes",   &split,           "notes"          },
            { &split,           "channel_1",       &dedup_ch_1,      "notes"          },
            { &split,           "channel_2",       &dedup_ch_2,      "notes"          },
            { &split,           "channel_3",       &dedup_ch_3,      "notes"          },
            { &split,           "channel_4",       &dedup_ch_4,      "notes"          },
            { &dedup_ch_1,      "notes",           &chord_intervals, "root_note"      },
            { &dedup_ch_2,      "notes",           &chord_intervals, "chord_notes"    },
            { &dedup_ch_3,      "notes",           &chord_intervals, "pattern_notes"  },
            { &dedup_ch_4,      "notes",           &arpeggiator,     "sequence_notes" },
            { &chord_intervals, "chord_notes",     &arpeggiator,     "chord_notes"    },
            { &chord_intervals, "chord_notes",     &wormhole,        "channel_1"      },
            { &chord_intervals, "pattern_degrees", &wormhole,        "channel_2"      },
            { &arpeggiator,     "notes",           &wormhole,        "channel_3"      },
            { &wormhole,        "channel_1",       &join,            "channel_1"      },
            { &wormhole,        "channel_2",       &join,            "channel_2"      },
            { &wormhole,        "channel_3",       &join,            "channel_3"      },
            { &join,            "channel_notes",   &sink,            "notes"          }
        } );
    }

    void play( void ) {
        input.note_on( 1, 48, 100 );                                                  // root C
        for ( int note : { 60, 64, 67, 71 } ) input.note_on( 2, note, 100 );          // Cmaj7
        for ( int note : { 62, 65, 69 } )     input.note_on( 3, note, 100 );          // pattern
        for ( int note : { 0, 1, 2, 3, 5 } )  input.note_on( 4, note, 100 );          // arp sequence
        sink.backtrack_process( );

        for ( int note : { 64, 67 } ) input.note_off( 2, note, 100 );                 // chord change
        for ( int note : { 63, 67 } ) input.note_on(  2, note, 100 );                 // Cm7 ( maj7 kept )
        sink.backtrack_process( );

        input.note_off( 1, 48, 100 );
        for ( int note : { 60, 63, 67, 71 } ) input.note_off( 2, note, 100 );
        for ( int note : { 62, 65, 69 } )     input.note_off( 3, note, 100 );
        for ( int note : { 0, 1, 2, 3, 5 } )  input.note_off( 4, note, 100 );
        sink.backtrack_process( );
    }
};

} // namespace

TEST_SUITE( "allocations" ) {

TEST_CASE( "the counter sees allocations" ) {
    allocation_counter counter;
    auto* leak_check = new int( 42 );
    delete leak_check;
    CHECK( counter.count( ) == 1 );
}

TEST_CASE( "playing the plugin graph never allocates" ) {
    plugin_graph graph; // wiring may allocate

    size_t allocations = 0;
    {
        allocation_counter counter;
        graph.play( );
        allocations = counter.count( );
    }

    CHECK( graph.sink.ons > 0 );
    CHECK( graph.sink.ons == graph.sink.offs ); // everything that went on, went off
    CHECK( allocations == 0 );
}

} // TEST_SUITE
