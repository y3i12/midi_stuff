#pragma once

#include "midi_stuff.h"

namespace midi_stuff {
namespace tests {

struct midi_channel_note_index {
    midi_channel_note_index( void ) {
        midi_stuff::midi_channel_note_index channel_notes;

        // basic insertions in not ordered sequence of notes
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 62, 100 ) ) == true ); // first note
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 1 );
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 60, 100 ) ) == true ); // push front
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 2 );
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 63, 100 ) ) == true ); // push back
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 3 );
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 61, 100 ) ) == true ); // insert
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 4 );

        // assert ordering
        auto end = channel_notes.get_channel_notes( 1 )->end( );
        auto current = channel_notes.get_channel_notes( 1 )->begin( );
        auto next = current;

        while ( ( next = std::next( next ) ) != end ) {
            jassert( current->note < next->note );
            current = next;
        }

        // off all notes in different order
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 61, 100 ) ) == true ); // middle
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 3 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 60, 100 ) ) == true ); // beginning
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 2 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 63, 100 ) ) == true ); // end
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 1 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 62, 100 ) ) == true ); // single
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 0 );

        // on again
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 62, 100 ) ) == true ); // first note
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 60, 100 ) ) == true ); // push front
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 63, 100 ) ) == true ); // push back
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 1, 61, 100 ) ) == true ); // insert
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 4 );

        // on in a different channel
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 62, 100 ) ) == true ); // first note
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 60, 100 ) ) == true ); // push front
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 63, 100 ) ) == true ); // push back
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 61, 100 ) ) == true ); // insert
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 4 );

        // with some double note on
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 61, 100 ) ) == false );
        jassert( channel_notes.note_on( midi_note_event( midi_note_event::k_note_on, 2, 62, 100 ) ) == false );
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 4 );

        // assert ordering
        end = channel_notes.get_channel_notes( 2 )->end( );
        current = channel_notes.get_channel_notes( 2 )->begin( );
        next = current;

        while ( ( next = std::next( next ) ) != end ) {
            jassert( current->note < next->note );
            current = next;
        }

        // off all notes in channel 1
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 61, 100 ) ) == true ); // middle
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 3 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 60, 100 ) ) == true ); // beginning
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 2 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 63, 100 ) ) == true ); // end
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 1 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 1, 62, 100 ) ) == true ); // single
        jassert( channel_notes.get_channel_notes( 1 )->size( ) == 0 );

        // off all notes in channel 2
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 61, 100 ) ) == false ); // double reference - doesn't remove
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 4 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 60, 100 ) ) == true );
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 3 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 63, 100 ) ) == true );
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 2 );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 62, 100 ) ) == false );  // double reference - doesn't remove
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 2 );

        // remove double on
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 61, 100 ) ) == true );
        jassert( channel_notes.note_off( midi_note_event( midi_note_event::k_note_off, 2, 62, 100 ) ) == true );
        jassert( channel_notes.get_channel_notes( 2 )->size( ) == 0 );
    }
};


class test_consumer : public midi_stuff::note_processor::midi_note_processor {
public:
    struct juce_mocks {
        static int noteOn(   int, int, int ) { return 1; }
        static int noteOff(       int, int ) { return 0; }
        void       addEvent(      int, int ) {           }
    };

public:
    int counter;
    midi_note::note_t last_note;
    juce_mocks mocks;

public:
    test_consumer( void ) : 
        midi_stuff::note_processor::midi_note_processor( ),
        counter( 0 ),
        last_note( -1 ) {
    }
    // end of mocks for juce

    virtual void process_note( const midi_note_event& event ) override {
        jassert( event.event_type >= midi_note_event::event_type_t::k_invalid &&
                 event.event_type <= midi_note_event::event_type_t::k_last    );

        jassert( event.channel >=                                                      1 &&
                 event.channel <= midi_stuff::midi_channel_note_index::k_number_channels );

        jassert( event.note >=   0 &&
                 event.note <= 127 );

        jassert( event.velocity >= 0 &&
                 event.velocity <= 127 );

        midi_note_event new_event( event );
        jassert( new_event == event && event == new_event );
        new_event = event;
        jassert( new_event == event && event == new_event );

        ++counter;
        last_note = event.note;
    }

    virtual void process( void ) override {
        jassert( provided.size( ) == providers.size( ) );
        if ( counter ) {
            jassert( last_note != -1 );
            last_note = -1;
            counter   =  0;
        }
    }

};

class midi_stuff_test_base_processor {
public:
    midi_stuff::note_processor::midi_note_input     input;
    midi_stuff::note_processor::midi_note_keeper    keeper;
    midi_stuff::note_processor::midi_lowest_notes   lowest;
    midi_stuff::note_processor::midi_highest_notes  highest;
    midi_stuff::note_processor::midi_note_output_tmpl< midi_stuff::tests::test_consumer::juce_mocks >    output;
    midi_stuff::tests::test_consumer                observer;

public:
    typedef midi_stuff::note_processor::midi_note_keeper::note_map_t note_map_t;
    typedef midi_stuff::note_processor::midi_note_processor::binder  binder;

    midi_stuff_test_base_processor( void ) {
        binder::binder( {
            { input,   observer },
            { keeper,  observer },
            { lowest,  observer },
            { input,   observer },
            { highest, observer },
            { output,  observer }
        } );
    }

    midi_stuff_test_base_processor( binder::bind_list_t list ) : midi_stuff_test_base_processor( ) {
        binder::binder( list );
    }

    void assert_notemap_has_all( const note_map_t& notes, std::initializer_list< midi_stuff::midi_note::note_t > list ) {
        for ( auto value : list ) {
            jassert( std::find_if( notes.begin( ), notes.end( ), [ & ] ( auto note ) { return note.note == value; } ) != notes.end( ) );
        }
    }
    void reset_output_buffers( void ) {
        output.set_output_midi_buffer( observer.mocks );
    }

    void run( void ) { observer.backtrack_process( ); }
};

struct midi_lowest_notes : public midi_stuff_test_base_processor {
    midi_lowest_notes( void ) : midi_stuff_test_base_processor( {
            { input, keeper  },
            { keeper, lowest },
            { lowest, output }
        } ) {

        lowest.number_notes_kept = 2;

        reset_output_buffers( );

        input.note_on( 1, 65, 100 ); // first
        run( );
        jassert( lowest.get_channel_notes( 1 )->size() == 1 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 65 } );
        input.note_on( 1, 60, 100 ); // prepend
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 60, 65 } );
        input.note_on( 1, 63, 100 ); // in between, discard 65, keep 63
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 60, 63 } );
        input.note_on( 1, 67, 100 ); // dont add
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 60, 63 } );

        input.note_off( 1, 67, 100 ); // ignore
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 60, 63 } );
        input.note_off( 1, 60, 100 ); // remove first, should keep 63 and 65
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 63, 65 } );
        input.note_off( 1, 63, 100 ); // keep 65
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 1 );
        assert_notemap_has_all( *lowest.get_channel_notes( 1 ), { 65 } );
        input.note_off( 1, 65, 100 ); // mute
        run( );
        jassert( lowest.get_channel_notes( 1 )->size( ) == 0 );
    }
};

struct midi_highest_notes : public midi_stuff_test_base_processor {
    midi_highest_notes( void ) : midi_stuff_test_base_processor( {
            { input, keeper  },
            { keeper, highest },
            { highest, output }
                                                                } ) {

        highest.number_notes_kept = 2;

        reset_output_buffers( );

        input.note_on( 1, 63, 100 ); // first
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 1 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 63 } );
        input.note_on( 1, 67, 100 ); // prepend
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 63, 67 } );
        input.note_on( 1, 65, 100 ); // in between, discard 63, keep 65
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 65, 67 } );
        input.note_on( 1, 60, 100 ); // dont add
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 65, 67 } );

        input.note_off( 1, 60, 100 ); // ignore
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 65, 67 } );
        input.note_off( 1, 67, 100 ); // remove first, should keep 63 and 65
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 2 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 63, 65 } );
        input.note_off( 1, 63, 100 ); // keep 65
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 1 );
        assert_notemap_has_all( *highest.get_channel_notes( 1 ), { 65 } );
        input.note_off( 1, 65, 100 ); // mute
        run( );
        jassert( highest.get_channel_notes( 1 )->size( ) == 0 );
    }
};

namespace __MAKE_THIS_CRAP_NOT_BEING_EASILY_ACCESSIBLE__ {
static midi_stuff::tests::midi_channel_note_index __test__midi_channel_note_index;
static midi_stuff::tests::midi_lowest_notes       __test__midi_lowest_notes;
static midi_stuff::tests::midi_highest_notes      __test__midi_highest_notes;
}
}
}