#pragma once

#include "midi_stuff.h"

namespace midi_stuff {
namespace tests {


class midi_stuff_processor_test_base {
public:
    midi_stuff::processors::midi_channel_note_input     input;
    midi_stuff::processors::midi_channel_split          split;
    midi_stuff::processors::midi_note_output            output;
    juce::MidiBuffer                                    midi_buffer;

public:
    typedef midi_stuff::processors::base_processor::binder  binder;

    midi_stuff_processor_test_base( void ) {
        output.set_output_midi_buffer( midi_buffer );
    }

    void reset_output_buffers( void ) {
        midi_buffer.clear( );
    }

    void run( void ) { output.backtrack_process( ); }
    
    template < typename container_t, typename note_collection_t >
    void assert_container_has_all( const container_t& container, const note_collection_t& notes ) {
        std::for_each( notes.begin( ), notes.end( ),
                       [ & ] ( note_t note ) {
                           jassert(
                               std::find(
                                   container.begin( ),
                                   container.end( ),
                                   note
                               ) != container.end( )
                           );
                       }
        );
    }

    template < typename container_t >
    void assert_container_has_all( const container_t& container, std::initializer_list< note_t > notes ) {
        assert_container_has_all( container, std::vector< note_t >( notes ) );
    }
};

struct test_midi_lowest_notes : public midi_stuff_processor_test_base {
    midi_stuff::processors::midi_note_dedup             dedup;
    midi_stuff::processors::midi_lowest_notes           lowest;

    test_midi_lowest_notes( void ) : midi_stuff_processor_test_base( ) {
        binder bind_it( {
            { &input,  "channel_notes", &split,   "notes" },
            { &split,  "channel_1",     &dedup,   "notes" },
            { &dedup,  "notes",         &lowest,  "notes" },
            { &lowest, "notes",         &output,  "notes" }
        } );

        lowest.number_notes_kept = 2;

        reset_output_buffers( );

        input.note_on( 1, 65, 100 ); // first
        run( );
        jassert( lowest.active_notes.size() == 1 );
        assert_container_has_all( lowest.active_notes, { 65 } );
        input.note_on( 1, 60, 100 ); // prepend
        run( );
        jassert( lowest.active_notes.size( ) == 2 );
        assert_container_has_all( lowest.active_notes, { 60, 65 } );
        input.note_on( 1, 63, 100 ); // in between, discard 65, keep 63
        run( );
        jassert( lowest.active_notes.size( ) == 2 );
        assert_container_has_all( lowest.active_notes, { 60, 63 } );
        input.note_on( 1, 67, 100 ); // dont add
        run( );
        jassert( lowest.active_notes.size( ) == 2 );
        assert_container_has_all( lowest.active_notes, { 60, 63 } );

        input.note_off( 1, 67, 100 ); // ignore
        run( );
        jassert( lowest.active_notes.size( ) == 2 );
        assert_container_has_all( lowest.active_notes, { 60, 63 } );
        input.note_off( 1, 60, 100 ); // remove first, should keep 63 and 65
        run( );
        jassert( lowest.active_notes.size( ) == 2 );
        assert_container_has_all( lowest.active_notes, { 63, 65 } );
        input.note_off( 1, 63, 100 ); // keep 65
        run( );
        jassert( lowest.active_notes.size( ) == 1 );
        assert_container_has_all( lowest.active_notes, { 65 } );
        input.note_off( 1, 65, 100 ); // mute
        run( );
        jassert( lowest.active_notes.size( ) == 0 );
    }
};

struct test_midi_highest_notes : public midi_stuff_processor_test_base {
    midi_stuff::processors::midi_note_dedup             dedup;
    midi_stuff::processors::midi_highest_notes          highest;

    test_midi_highest_notes( void ) : midi_stuff_processor_test_base( ) {
        binder bind_it( {
            { &input,   "channel_notes", &split,   "notes" },
            { &split,   "channel_1",     &dedup,   "notes" },
            { &dedup,   "notes",         &highest, "notes" },
            { &highest, "notes",         &output,  "notes" }
        } );

        highest.number_notes_kept = 2;

        reset_output_buffers( );

        input.note_on( 1, 63, 100 ); // first
        run( );
        jassert( highest.notes.size( ) == 1 );
        assert_container_has_all( highest.active_notes, { 63 } );
        input.note_on( 1, 67, 100 ); // prepend
        run( );
        jassert( highest.active_notes.size( ) == 2 );
        assert_container_has_all( highest.active_notes, { 63, 67 } );
        input.note_on( 1, 65, 100 ); // in between, discard 63, keep 65
        run( );
        jassert( highest.active_notes.size( ) == 2 );
        assert_container_has_all( highest.active_notes, { 65, 67 } );
        input.note_on( 1, 60, 100 ); // dont add
        run( );
        jassert( highest.active_notes.size( ) == 2 );
        assert_container_has_all( highest.active_notes, { 65, 67 } );

        input.note_off( 1, 60, 100 ); // ignore
        run( );
        jassert( highest.active_notes.size( ) == 2 );
        assert_container_has_all( highest.active_notes, { 65, 67 } );
        input.note_off( 1, 67, 100 ); // remove first, should keep 63 and 65
        run( );
        jassert( highest.active_notes.size( ) == 2 );
        assert_container_has_all( highest.active_notes, { 63, 65 } );
        input.note_off( 1, 63, 100 ); // keep 65
        run( );
        jassert( highest.active_notes.size( ) == 1 );
        assert_container_has_all( highest.active_notes , { 65 } );
        input.note_off( 1, 65, 100 ); // mute
        run( );
        jassert( highest.active_notes.size( ) == 0 );
    }
};

struct test_midi_note_arpeggiator : public midi_stuff_processor_test_base {
public:
    midi_stuff::processors::midi_note_dedup             dedup_1;
    midi_stuff::processors::midi_note_dedup             dedup_2;
    midi_stuff::processors::midi_note_arpeggiator       arpeggiator;
    test_midi_note_arpeggiator( void ) : midi_stuff_processor_test_base( ) {
        binder bind_it( {
            { &input,       "channel_notes", &split,       "notes"          },
            { &split,       "channel_1",     &dedup_1,     "notes"          },
            { &split,       "channel_2",     &dedup_2,     "notes"          },
            { &dedup_1,     "notes",         &arpeggiator, "chord_notes"    },
            { &dedup_2,     "notes",         &arpeggiator, "sequence_notes" },
            { &arpeggiator, "notes",         &output,      "notes"          }
        } );

        reset_output_buffers( );

        input.note_on( 1, 60, 100 );
        input.note_on( 1, 63, 100 );
        input.note_on( 1, 65, 100 );
        input.note_on( 1, 67, 100 );
        run( );
        jassert( arpeggiator.chord_notes.size( ) == 4 );
        assert_container_has_all( arpeggiator.chord_notes, { 60, 63, 65, 67 } );



        // TODO: test arp sequence
        input.note_on( 2, 0, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  0 } );
        assert_container_has_all( arpeggiator.notes,          { 60 } );

        input.note_on( 2, 1, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 2 );
        jassert(                  arpeggiator.notes.size( )          == 2 );
        assert_container_has_all( arpeggiator.sequence_notes, {  0,  1 } );
        assert_container_has_all( arpeggiator.notes,          { 60, 63 } );

        input.note_on( 2, 2, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 3 );
        jassert(                  arpeggiator.notes.size( )          == 3 );
        assert_container_has_all( arpeggiator.sequence_notes, {  0,  1,  2 } );
        assert_container_has_all( arpeggiator.notes,          { 60, 63, 65 } );
        
        input.note_on( 2, 3, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 4 );
        jassert(                  arpeggiator.notes.size( )          == 4 );
        assert_container_has_all( arpeggiator.sequence_notes, {  0,  1,  2,  3 } );
        assert_container_has_all( arpeggiator.notes,          { 60, 63, 65, 67 } );


        input.note_off( 2, 0, 100 );
        input.note_off( 2, 1, 100 );
        input.note_off( 2, 2, 100 );
        input.note_off( 2, 3, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 0 );
        jassert(                  arpeggiator.notes.size( )          == 0 );
        
        input.note_on( 2, 0, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  0 } );
        assert_container_has_all( arpeggiator.notes,          { 60 } );


        input.note_off( 2, 0, 100 );
        input.note_on(  2, 1, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  1 } );
        assert_container_has_all( arpeggiator.notes,          { 63 } );

        
        input.note_off( 2, 1, 100 );
        input.note_on(  2, 2, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  2 } );
        assert_container_has_all( arpeggiator.notes,          { 65 } );

        
        input.note_off( 2, 2, 100 );
        input.note_on(  2, 3, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  3 } );
        assert_container_has_all( arpeggiator.notes,          { 67 } );
        
        input.note_off( 2, 3, 100 );
        input.note_on(  2, 4, 100 );
        run( );
        jassert(                  arpeggiator.sequence_notes.size( ) == 1 );
        jassert(                  arpeggiator.notes.size( )          == 1 );
        assert_container_has_all( arpeggiator.sequence_notes, {  4 } );
        assert_container_has_all( arpeggiator.notes,          { 72 } ); // 60 + one octave up


        input.note_off( 2,  4, 100 );
        input.note_off( 1, 60, 100 );
        input.note_off( 1, 63, 100 );
        input.note_off( 1, 65, 100 );
        input.note_off( 1, 67, 100 );
        run( );
        jassert( arpeggiator.chord_notes.size( )    == 0 );
        jassert( arpeggiator.sequence_notes.size( ) == 0 );
        jassert( arpeggiator.notes.size( )          == 0 );
    }
};

struct test_midi_midi_chord_intervals : public midi_stuff_processor_test_base {
public:
    midi_stuff::processors::midi_note_dedup             dedup_1;
    midi_stuff::processors::midi_note_dedup             dedup_2;
    midi_stuff::processors::midi_chord_intervals        chord_intervals;

    void test_chord( note_t root, const std::vector< note_t >& intervals ) {
        input.note_on( 1, root, 100 );
        std::for_each(
            intervals.cbegin( ),
            intervals.cend( ),
            [ & ] ( const note_t& note ) {
                input.note_on( 2, note + root, 100 );
            }
        );

        run( );

        assert_container_has_all( chord_intervals.chord_notes_output, intervals );

        /*jassert( chord_intervals.chord_notes.size( ) == chord_data::chord_descriptors[ 1 ].notes.size( ) );*/
        input.note_off( 1, root, 100 );
        std::for_each(
            intervals.cbegin( ),
            intervals.cend( ),
            [ & ] ( const note_t& note ) {
                input.note_off( 2, note + root, 100 );
            }
        );

        run( );
        jassert( chord_intervals.chord_notes_output.size( ) == 0 );
    }

    test_midi_midi_chord_intervals( void ) : midi_stuff_processor_test_base( ) {
        binder bind_it( {
            { &input,           "channel_notes", &split,           "notes"          },
            { &split,           "channel_1",     &dedup_1,         "notes"          },
            { &split,           "channel_2",     &dedup_2,         "notes"          },
            { &dedup_1,         "notes",         &chord_intervals, "root_note"      },
            { &dedup_2,         "notes",         &chord_intervals, "chord_notes"    },
            { &chord_intervals, "chord_notes",   &output,          "notes"          }
        } );

        reset_output_buffers( );

        test_chord( 60, { 3, 7        } ); // minor
        test_chord( 60, { 3, 7, 9     } ); // m6
        test_chord( 60, { 3, 7, 10    } ); // m7
        test_chord( 60, { 3, 7, 10, 2 } ); // m9
        test_chord( 60, { 3, 7, 10, 5 } ); // m11
        test_chord( 60, { 3, 7, 10, 9 } ); // m13

    }
};

namespace __MAKE_THIS_CRAP_NOT_BEING_EASILY_ACCESSIBLE__ {

static std::tuple<
    midi_stuff::tests::test_midi_lowest_notes,
    midi_stuff::tests::test_midi_highest_notes,
    midi_stuff::tests::test_midi_note_arpeggiator,
    midi_stuff::tests::test_midi_midi_chord_intervals
> tests;

} // namespace __MAKE_THIS_CRAP_NOT_BEING_EASILY_ACCESSIBLE__
} // tests
} // midi_stuff