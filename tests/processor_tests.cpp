#include <doctest/doctest.h>

#include "midi_stuff/midi_stuff.h"

#include <set>
#include <utility>
#include <vector>

namespace {

using namespace midi_stuff;
using namespace midi_stuff::processors;
using binder = base_processor::binder;

// end of a graph: records what reaches it
struct note_sink : public base_processor {
    std::set< int >                     held;
    std::vector< std::pair< bool, int > > log; // ( is_note_on, note )

    explicit note_sink( parameter_type type = k_midi_note ) : base_processor( { { "notes", type, 0 } }, { } ) { }

    void process_event( const events::note_on&          event, parameter_port&, parameter_port& ) override { on(  event.note ); }
    void process_event( const events::note_off&         event, parameter_port&, parameter_port& ) override { off( event.note ); }
    void process_event( const events::channel_note_on&  event, parameter_port&, parameter_port& ) override { on(  event.note ); }
    void process_event( const events::channel_note_off& event, parameter_port&, parameter_port& ) override { off( event.note ); }

private:
    void on(  int note ) { held.insert( note ); log.emplace_back( true,  note ); }
    void off( int note ) { held.erase(  note ); log.emplace_back( false, note ); }
};

struct counting_dedup : public midi_note_dedup {
    int processed = 0;
    void process( void ) override { ++processed; }
};

std::set< int > notes_of( const containers::ascending_midi_note_list& list ) {
    std::set< int > result;
    for ( const auto& note : list ) {
        result.insert( note.note );
    }
    return result;
}

// channel 1: root, channel 2: chord, channel 3: pattern -> chord_intervals -> sink( pattern_degrees )
struct chord_intervals_graph {
    midi_channel_note_input input;
    midi_channel_split      split;
    midi_note_dedup         dedup_root;
    midi_note_dedup         dedup_chord;
    midi_note_dedup         dedup_pattern;
    midi_chord_intervals    chord_intervals;
    note_sink               sink;

    chord_intervals_graph( void ) {
        binder bind_it( {
            { &input,           "channel_notes",   &split,           "notes"         },
            { &split,           "channel_1",       &dedup_root,      "notes"         },
            { &split,           "channel_2",       &dedup_chord,     "notes"         },
            { &split,           "channel_3",       &dedup_pattern,   "notes"         },
            { &dedup_root,      "notes",           &chord_intervals, "root_note"     },
            { &dedup_chord,     "notes",           &chord_intervals, "chord_notes"   },
            { &dedup_pattern,   "notes",           &chord_intervals, "pattern_notes" },
            { &chord_intervals, "pattern_degrees", &sink,            "notes"         }
        } );
    }

    void chord( std::initializer_list< int > notes, bool on = true ) {
        for ( int note : notes ) on ? input.note_on( 2, note, 100 ) : input.note_off( 2, note, 100 );
    }

    void run( void ) { sink.backtrack_process( ); }
};

} // namespace

TEST_SUITE( "midi_chord_intervals" ) {

TEST_CASE( "without a root note, the lowest played note is the root" ) {
    chord_intervals_graph graph;

    graph.chord( { 55, 59, 62 } ); // G3 B3 D4
    graph.run( );

    REQUIRE( graph.chord_intervals.chosen_chord != nullptr );
    CHECK( graph.chord_intervals.chosen_chord->name == "major" );
    CHECK( notes_of( graph.chord_intervals.chord_notes_output ) == std::set< int >{ 7, 11, 14 } ); // root_shift + intervals
}

TEST_CASE( "an explicit root note wins over the lowest note" ) {
    chord_intervals_graph graph;

    graph.input.note_on( 1, 50, 100 ); // D
    graph.chord( { 54, 57, 62 } );     // F# A D
    graph.run( );

    REQUIRE( graph.chord_intervals.chosen_chord != nullptr );
    CHECK( graph.chord_intervals.chosen_chord->name == "major" );
    CHECK( notes_of( graph.chord_intervals.chord_notes_output ) == std::set< int >{ 2, 6, 9 } );
}

TEST_CASE( "pattern degrees are transposition invariant" ) {
    chord_intervals_graph in_c;
    in_c.chord( { 60, 64, 67 } );           // C E G
    in_c.input.note_on( 3, 64, 100 );       // E: the third
    in_c.run( );

    chord_intervals_graph in_g;
    in_g.chord( { 55, 59, 62 } );           // G B D
    in_g.input.note_on( 3, 71, 100 );       // B: the third, same octave relative to the root
    in_g.run( );

    CHECK( in_c.sink.held.size( ) == 1 );
    CHECK( in_c.sink.held == in_g.sink.held );
}

TEST_CASE( "releasing a pattern note sends its note off" ) {
    chord_intervals_graph graph;
    graph.chord( { 60, 64, 67 } );
    graph.input.note_on( 3, 64, 100 );
    graph.run( );
    REQUIRE( graph.sink.held.size( ) == 1 );

    graph.input.note_off( 3, 64, 100 );
    graph.run( );

    CHECK( graph.sink.held.empty( ) );
    CHECK( graph.chord_intervals.pattern_degrees_output.empty( ) );
}

} // TEST_SUITE

TEST_SUITE( "base_processor" ) {

TEST_CASE( "a node feeding two branches is processed once per pass" ) {
    //            +-> branch_a -+
    // input -> split -> shared              join -> sink
    //            +-> branch_b -+
    midi_channel_note_input input;
    midi_channel_split      split;
    counting_dedup          shared;
    midi_note_dedup         branch_a;
    midi_note_dedup         branch_b;
    midi_channel_join       join;
    note_sink               sink( k_midi_channel_note );

    binder bind_it( {
        { &input,    "channel_notes", &split,    "notes"     },
        { &split,    "channel_1",     &shared,   "notes"     },
        { &shared,   "notes",         &branch_a, "notes"     },
        { &shared,   "notes",         &branch_b, "notes"     },
        { &branch_a, "notes",         &join,     "channel_1" },
        { &branch_b, "notes",         &join,     "channel_2" },
        { &join,     "channel_notes", &sink,     "notes"     }
    } );

    sink.backtrack_process( );
    CHECK( shared.processed == 1 );

    sink.backtrack_process( ); // and the bookkeeping is reset between passes
    CHECK( shared.processed == 2 );
}

} // TEST_SUITE

TEST_SUITE( "midi_channel_filter" ) {

TEST_CASE( "channel_mute toggles the 1-based channel it receives" ) {
    midi_channel_filter filter;
    parameter_port&     mute_port = *filter.get_input( "channel_mute" );

    events::value_change mute_2( events::value_change_event_t( static_cast< int32_t >( 2 ) ) );

    filter.process_event( mute_2, mute_port, mute_port );
    CHECK_FALSE( filter.channel_mute[ 0 ] );
    CHECK(       filter.channel_mute[ 1 ] );

    filter.process_event( mute_2, mute_port, mute_port );
    CHECK_FALSE( filter.channel_mute[ 1 ] );
}

} // TEST_SUITE
