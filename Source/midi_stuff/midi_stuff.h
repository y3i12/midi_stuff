#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <unordered_set>

#include "midi_stuff/tinyevents/tinyevents.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace midi_stuff {

typedef tinyevents::Dispatcher dispatcher_t;

//---------------------------------------------------------------------------------------------------------------------------
// midi note event
//---------------------------------------------------------------------------------------------------------------------------

struct midi_note {
    typedef int8_t       channel_t;
    typedef int8_t       note_t;
    typedef uint8_t      velocity_t;


    struct channel_note_velocity_less_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return std::tie( lhs.channel, lhs.note, lhs.velocity ) < std::tie( rhs.channel, rhs.note, rhs.velocity );
        }
    };

    struct channel_note_less_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return std::tie( lhs.channel, lhs.note ) < std::tie( rhs.channel, rhs.note );
        }
    };

    struct note_less_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return std::tie( lhs.note ) < std::tie( rhs.note );
        }
    };

    struct note_eq_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return std::tie( lhs.note ) == std::tie( rhs.note );
        }
    };

    midi_note( void ) :
        channel( 0 ),
        note( 0 ),
        velocity( 0 ) {
    }

    midi_note( const midi_note& other ) :
        channel( other.channel ),
        note( other.note ),
        velocity( other.velocity ) {
    }
    
    template < class T, class U, class V > 
    midi_note( T channel, U note, V velocity ) :
        channel(  static_cast< channel_t  >( channel  ) ),
        note(     static_cast< note_t     >( note     ) ),
        velocity( static_cast< velocity_t >( velocity ) ) {
    }

    bool operator==( const midi_note& other ) {
        return note_eq_t( )( *this, other );
    }

    channel_t    channel;
    note_t       note;
    velocity_t   velocity;
};

struct midi_note_event : public midi_note {
    using midi_note::channel_t;
    using midi_note::note_t;
    using midi_note::velocity_t;

    enum event_type_t {
        k_invalid = 0,
        k_note_on = 1,
        k_note_off = 2,


        k_last,
    };

    midi_note_event( void ) :
        midi_note( ),
        event_type( k_invalid ),
        reference_num( 0 ),
        extra_data( nullptr ) {
    }

    midi_note_event( event_type_t event_type, const midi_note_event& other ) :
        midi_note( other ),
        event_type( event_type ),
        reference_num( 0 ),
        extra_data( other.extra_data ) {
    }

    midi_note_event( const midi_note_event& other, bool add_ref = false ) :
        midi_note( other ),
        event_type( other.event_type ),
        reference_num( add_ref ? 1 : other.reference_num ),
        extra_data( other.extra_data ) {
    }

    template < class T, class U, class V >
    midi_note_event( event_type_t event_type, T channel, U note, V velocity, void* extra_data = nullptr ) :
        midi_note( channel, note, velocity ),
        event_type( event_type ),
        reference_num( 0 ),
        extra_data( extra_data ) {
    }

    event_type_t event_type;
    int8_t       reference_num;
    void*        extra_data;
};

//---------------------------------------------------------------------------------------------------------------------------
// midi channel note index
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_note_index {
public:
    static constexpr int k_number_channels = 16;

    typedef std::list< midi_note_event >                                            note_map_t;
    typedef note_map_t::value_type                                                  counted_note_t;
    typedef note_map_t::iterator                                                    note_iterator_t;
    typedef note_map_t::reverse_iterator                                            note_reverse_iterator_t;
    typedef std::pair< note_iterator_t, note_iterator_t >                           note_range_t;
    typedef std::vector< note_map_t >                                               channel_note_map_t;

protected:
    channel_note_map_t channel_notes;

public:
    midi_channel_note_index( void ) :
        channel_notes( k_number_channels ) {
    }

    note_map_t* get_channel_notes( int8_t channel ) {
        if ( channel < 1 || channel > k_number_channels ) {
            return nullptr;
        }

        return &channel_notes[ channel - 1 ];
    }

    bool note_on( const midi_note_event& event ) {
        if ( event.channel               < 1                 ||
             event.channel               > 16                ||
             midi_note_event::k_note_on != event.event_type ) {
            return false;
        }

        auto& notes        = *get_channel_notes( event.channel );
        auto note_iterator = std::find_if( notes.begin( ), notes.end( ), [ & ] ( auto note ) { return note.note >= event.note; } );

        if ( ( note_iterator != notes.end( ) ) && ( *note_iterator == event ) ) {
            ++note_iterator->reference_num;
            return false;
        }

        notes.insert( note_iterator, midi_note_event( event, true ) );
        return true;
    };

    bool note_off( const midi_note_event& event ) {
        if ( event.channel               < 1                 ||
             event.channel               > 16                ||
             midi_note_event::k_note_off != event.event_type ) {
            return false;
        }

        auto& notes        = *get_channel_notes( event.channel );
        auto note_iterator = std::find_if( notes.begin( ), notes.end( ), [ & ]( auto note ) { return note.note == event.note; } );

        if ( ( note_iterator != notes.end( ) ) && ( 0 == --note_iterator->reference_num ) ) {
            notes.erase( note_iterator );
            return true;
        }

        return false;
    };

    note_range_t get_channel_note_range( int8_t channel ) {
        if ( channel < 1 || channel > 16 ) {
            auto& notes = *get_channel_notes( 1 );
            return note_range_t( notes.end( ), notes.end( ) );
        }

        auto& notes = *get_channel_notes( channel );
        return note_range_t( notes.begin(), notes.end() );
    }

    template < typename T >
    void channel_notes_into( int8_t channel, T& output ) {
        auto note_range = get_channel_note_range( channel );
        std::transform( note_range.first, note_range.second, std::back_inserter( output ), [ ] ( auto& kv ) { return kv.first; } );
    }

    template < typename T >
    void channel_note_ptrs_into( int8_t channel, T& output ) {
        auto note_range = get_channel_note_range( channel );
        std::transform( note_range.first, note_range.second, std::back_inserter( output ), [ ] ( auto& kv ) { return &kv.first; } );
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// note processor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace note_processor {

//---------------------------------------------------------------------------------------------------------------------------
// midi note processor - base class and binding helpers
//---------------------------------------------------------------------------------------------------------------------------
class midi_note_processor {
protected:
    dispatcher_t                                 dispatcher;
    std::unordered_set< uint64_t >               dispatcher_handles;
    std::unordered_set< midi_note_processor* >   consumers;
    std::unordered_set< midi_note_processor* >   providers;
    std::unordered_set< midi_note_processor* >   provided;
    bool                                         has_processed;

public:
    struct binder {
        typedef std::pair<
            midi_stuff::note_processor::midi_note_processor&,
            midi_stuff::note_processor::midi_note_processor&
        > bind_t;

        typedef std::initializer_list< bind_t > bind_list_t;

        binder( bind_list_t list ) {
            std::for_each( list.begin( ), list.end( ), [ & ] ( auto& to_bind ) { to_bind.first.add_consumer( to_bind.second ); } );
        }
    };

public:
    midi_note_processor( void ) : has_processed( false ) { }

    void add_consumer( midi_note_processor& the_midi_note_processor ) {
        auto insertion = consumers.insert( &the_midi_note_processor );

        if ( insertion.second ) { 
            dispatcher_handles.insert(
                dispatcher.listen< midi_note_event >( 
                    [ & ] ( const midi_note_event& event ) {
                        the_midi_note_processor.process_note( event );
                    }
                )
            );

            the_midi_note_processor.providers.insert( this );
        }
    }

    virtual void process_note( const midi_note_event& event ) = 0;
    virtual void process( void ) = 0;

    void set_provided( midi_note_processor* the_midi_note_processor ) {
        provided.insert( the_midi_note_processor );
    }

    bool try_process( void ) {
        if ( provided.size( ) == providers.size( ) ) {
            process( );
            provided.clear( );
            has_processed = true;
            return true;
        }
        has_processed = false;
        return false;
    }

    template < class T, class U, class V >
    void enqueue_note( midi_note_event::event_type_t event_type, T channel, U note, V velocity ) {
        dispatcher.queue(
            midi_note_event(
                event_type,
                static_cast< int8_t >( channel ),
                static_cast< int8_t >( note ),
                static_cast< uint8_t >( velocity )
            )
        );
    }

    void enqueue_note( const midi_note_event& the_note ) {
        dispatcher.queue( the_note );
    }

    void dispatch_enqueued_notes( void ) {
        dispatcher.process( );
    }

    void process_tree_execute( void ) {
        dispatch_enqueued_notes( );
        if ( try_process( ) ) {
            for ( auto consumer : consumers ) {
                consumer->set_provided( this );
                consumer->process_tree_execute( );
            }
        }
    }

    void process_tree_clear( void ) {
        for ( auto consumer : consumers ) {
            consumer->provided.erase( this );
            consumer->process_tree_clear( );
        }
    }

    void process_tree( void ) {
        process_tree_execute( );
        process_tree_clear( );
    }

    void backtrack_process_execute( void ) {
        for ( auto provider : providers ) {
            if ( provided.find( provider ) == provided.end( ) ) {
                provider->backtrack_process_execute( );
            }
        }

        dispatch_enqueued_notes( );
        process( );

        for ( auto consumer : consumers ) {
            consumer->set_provided( this );
        }
    }

    void backtrack_process_clear( void ) {
        for ( auto provider : providers ) {
            if ( provided.find( provider ) != provided.end( ) ) {
                provider->backtrack_process_clear( );
            }
        }

        for ( auto consumer : consumers ) {
            consumer->provided.erase( this );
        }
    }

    void backtrack_process( void ) {
        backtrack_process_execute( );
        backtrack_process_clear( );
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi note input 
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_input : public midi_note_processor {
public:
    midi_note_input ( void ) : midi_note_processor( ) { }

    // only creates notes, no input
    virtual void process_note( const midi_note_event& ) override { };
    virtual void process( void ) override { };

    template < class T, class U, class V >
    void note_on( T channel, U note, V velocity ) {
        enqueue_note( midi_note_event::k_note_on, channel, note, velocity );
    }

    template < class T, class U, class V >
    void note_off( T channel, U note, V velocity ) {
        enqueue_note( midi_note_event::k_note_off, channel, note, velocity );
    }
};



//---------------------------------------------------------------------------------------------------------------------------
// midi note keeper 
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_keeper : public midi_note_processor {
public:
    typedef midi_channel_note_index::note_map_t                 note_map_t;
    typedef midi_channel_note_index::counted_note_t             counted_note_t;
    typedef midi_channel_note_index::note_iterator_t            note_iterator_t;
    typedef midi_channel_note_index::note_reverse_iterator_t    note_reverse_iterator_t;
    typedef midi_channel_note_index::note_range_t               note_range_t;
    typedef midi_channel_note_index::channel_note_map_t         channel_note_map_t;

protected:
    midi_channel_note_index notes;

public:
    midi_note_keeper( void ) : midi_note_processor( ) { }

    void forward_event( const midi_note_event& event ) {
        enqueue_note( event );
    }

    virtual void process_note( const midi_note_event& event ) override {
        if ( midi_note_event::k_note_on == event.event_type ) {
            note_on( event );
        } else if ( midi_note_event::k_note_off == event.event_type ) {
            note_off( event );
        }
    };

    virtual void note_on( const midi_note_event& event ) {
        if ( notes.note_on( event ) ) {
            forward_event( event );
        }
    }

    virtual void note_off( const midi_note_event& event ) {
        if ( notes.note_off( event ) ) {
            forward_event( event );
        }
    }

    virtual void process( void ) override { };

    note_map_t* get_channel_notes( int8_t channel ) {
        return notes.get_channel_notes( channel );
    }

    note_range_t get_channel_note_range( int8_t channel ) {
        return notes.get_channel_note_range( channel );
    }

    template < typename T >
    void channel_notes_into( int8_t channel, T& output ) {
        notes.channel_notes_into( channel, output );
    }


    template < typename T >
    void channel_note_ptrs_into( int8_t channel, T& output ) {
        notes.channel_note_ptrs_into( channel, output );
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi lowest notes
//---------------------------------------------------------------------------------------------------------------------------

class midi_lowest_notes : public midi_note_keeper {
public:
    using   midi_note_keeper::note_map_t;
    using   midi_note_keeper::counted_note_t;
    using   midi_note_keeper::note_iterator_t;
    using   midi_note_keeper::note_range_t;
    using   midi_note_keeper::note_reverse_iterator_t;
    using   midi_note_keeper::channel_note_map_t;

    typedef midi_note::note_less_t  note_less_t;
    typedef midi_note::note_eq_t    note_eq_t;

public:
    uint8_t                 number_notes_kept;
    std::vector< bool >     dirty_channels;


public:
    midi_lowest_notes( void ) :
        midi_note_keeper( ),
        number_notes_kept( 0 ),
        dirty_channels( midi_channel_note_index::k_number_channels ) {
    }

    // delayed note forwarding
    virtual void note_on(  const midi_note_event& event ) {
        dirty_channels[ event.channel - 1 ] = true;
    }
    virtual void note_off( const midi_note_event& event ) {
        dirty_channels[ event.channel - 1 ] = true;
    }

    virtual void process( ) override {
        for ( int8_t i = 0; i < midi_channel_note_index::k_number_channels; ++i ) {
            if ( !dirty_channels[ i ] ) {
                continue;
            }

            dirty_channels[ i ] = 0;

            auto& channel_notes           = *get_channel_notes( i + 1 );
            auto  current_notes_itr       = channel_notes.begin( );

            // TODO: This is not nice, find a way to remove the static cast and ensure that providers has only one input
            midi_note_keeper* midi_note_source = static_cast< midi_note_keeper* >( *providers.begin( ) );
            auto& other_channel_notes     = *( midi_note_source->get_channel_notes( i + 1 ) );
            auto  other_current_notes_itr = other_channel_notes.begin( );

            uint8_t current_note_num      = 0;

            // merge the n lowest elements of the two ordered lists of notes present in this->notes and midi_note_source->notes
            // into this->notes. What is present in this->notes and not in midi_note_source->notes is removed, and what 
            // is in midi_note_source->notes and not in this->notes is added.
            // 
            // Move two iterators from the start comparing elements, random example for number_notes_kept=4:
            //
            //                      r  k  a  r  k   a  i
            //       current notes 60 63    67 70
            // other current notes    63 65    70  73 75 
            //           note kept     1  2     3   4
            // 
            // where r: remove
            //       k: keep
            //       a: add
            //       i: ignore
            while ( current_note_num         <  number_notes_kept &&
                    current_notes_itr       != channel_notes.end( ) &&
                    other_current_notes_itr != other_channel_notes.end( ) ) {

                if ( note_less_t( )( *current_notes_itr, *other_current_notes_itr ) ) {
                    // A < B -> the lowest note in this is lower than in midi_note_source,
                    // then it is not present on the new notes: note_off
                    forward_event( midi_note_event( midi_note_event::k_note_off, *current_notes_itr ) );
                    current_notes_itr = channel_notes.erase( current_notes_itr );

                } else if ( note_less_t( )( *other_current_notes_itr, *current_notes_itr ) ) {
                    // B < A -> the lowest note in midi_note_source is lower than in this,
                    // then it is a new note to be added
                    channel_notes.insert( current_notes_itr, *other_current_notes_itr );
                    forward_event( midi_note_event( midi_note_event::k_note_on, *other_current_notes_itr ) );
                    ++other_current_notes_itr;
                    ++current_note_num;

                } else {
                    // equals - they're in both. it is cool, nothing to be done
                    ++current_notes_itr;
                    ++other_current_notes_itr;
                    ++current_note_num;
                }
            }

            // Additional checks for completing and removing the tail of current notes
            if ( current_notes_itr == channel_notes.end( ) ) { // complete
                // If all pre-existing notes were iterated through, it means that there might be additional tail notes to note_on
                // Tries to fill the current notes with the notes from midi_note_source till number_notes_kept notes are on
                while ( current_note_num         < number_notes_kept          && 
                        other_current_notes_itr != other_channel_notes.end( ) ) {
                    channel_notes.push_back( *other_current_notes_itr );
                    forward_event( midi_note_event( midi_note_event::k_note_on, *other_current_notes_itr ) );
                    ++current_note_num;
                    ++other_current_notes_itr;
                }
            } else if ( ( current_note_num == number_notes_kept ) || ( other_current_notes_itr == other_channel_notes.end( ) ) ) { // remove tail
                // if the iterator isn't pointing at the end and the desired number of notes were collected, note_off the tail of the list
                while ( current_notes_itr != channel_notes.end( ) ) {
                    forward_event( midi_note_event( midi_note_event::k_note_off, *current_notes_itr ) );
                    current_notes_itr = channel_notes.erase( current_notes_itr );
                }
            }
        }
    }

};

//---------------------------------------------------------------------------------------------------------------------------
// midi highest notes
//---------------------------------------------------------------------------------------------------------------------------

class midi_highest_notes : public midi_lowest_notes {
public:
    midi_highest_notes( void ) :
        midi_lowest_notes( ) {
    }

    virtual void process( ) override {
        for ( int8_t i = 0; i < midi_channel_note_index::k_number_channels; ++i ) {
            if ( !dirty_channels[ i ] ) {
                continue;
            }

            dirty_channels[ i ] = 0;

            auto&                        channel_notes     = *get_channel_notes( i + 1 );
            note_map_t::reverse_iterator current_notes_itr =  channel_notes.rbegin(    );
            note_map_t::reverse_iterator current_notes_end =  channel_notes.rend(      );

            // TODO: This is not nice, find a way to remove the static cast and ensure that providers has only one input
            midi_note_keeper*            midi_note_source        = static_cast< midi_note_keeper* >( *providers.begin( ) );
            auto&                        other_channel_notes     = *( midi_note_source->get_channel_notes( i + 1 ) );
            note_map_t::reverse_iterator other_current_notes_itr = other_channel_notes.rbegin( );
            note_map_t::reverse_iterator other_current_notes_end = other_channel_notes.rend(    );

            uint8_t current_note_num      = 0;

            // merge the n lowest elements of the two ordered lists of notes present in this->notes and midi_note_source->notes
            // into this->notes. What is present in this->notes and not in midi_note_source->notes is removed, and what 
            // is in midi_note_source->notes and not in this->notes is added.
            // 
            // Move two iterators from the start comparing elements, random example for number_notes_kept=4:
            //
            //                      r  k  a  r  k   a  i
            //       current notes 60 63    67 70
            // other current notes    63 65    70  73 75 
            //           note kept     1  2     3   4
            // 
            // where r: remove
            //       k: keep
            //       a: add
            //       i: ignore
            while ( current_note_num         <  number_notes_kept &&
                    current_notes_itr       != current_notes_end &&
                    other_current_notes_itr != other_current_notes_end ) {

                if ( note_less_t( )( *other_current_notes_itr, *current_notes_itr ) ) {
                    // B < A -> the highest note in this is higher than in midi_note_source,
                    // then it is not present on the new notes: note_off
                    forward_event( midi_note_event( midi_note_event::k_note_off, *current_notes_itr ) );
                    current_notes_itr = note_map_t::reverse_iterator( channel_notes.erase( std::next( current_notes_itr ).base( ) ) );

                } else if ( note_less_t( )( *current_notes_itr , *other_current_notes_itr ) ) {
                    // A < B -> the highest note in midi_note_source is higher than in this,
                    // then it is a new note to be added
                    channel_notes.insert( current_notes_itr.base( ), *other_current_notes_itr );
                    forward_event( midi_note_event( midi_note_event::k_note_on, *other_current_notes_itr ) );
                    ++other_current_notes_itr;
                    ++current_notes_itr;
                    ++current_note_num;

                } else {
                    // equals - they're in both. it is cool, nothing to be done
                    ++current_notes_itr;
                    ++other_current_notes_itr;
                    ++current_note_num;
                }
            }

            // Additional checks for completing and removing the tail of current notes
            if ( current_notes_itr == current_notes_end ) { // complete
                // If all pre-existing notes were iterated through, it means that there might be additional tail notes to note_on
                // Tries to fill the current notes with the notes from midi_note_source till number_notes_kept notes are on
                while ( current_note_num         < number_notes_kept          && 
                        other_current_notes_itr != other_current_notes_end ) {
                    channel_notes.push_front( *other_current_notes_itr );
                    forward_event( midi_note_event( midi_note_event::k_note_on, *other_current_notes_itr ) );
                    ++current_note_num;
                    ++other_current_notes_itr;
                }
            } else if ( ( current_note_num == number_notes_kept ) || ( other_current_notes_itr == other_current_notes_end ) ) { // remove tail
                // if the iterator isn't pointing at the end and the desired number of notes were collected, note_off the tail of the list
                while ( current_notes_itr != current_notes_end ) {
                    forward_event( midi_note_event( midi_note_event::k_note_off, *current_notes_itr ) );
                    ++current_notes_itr;
                    channel_notes.erase( current_notes_itr.base( ) );
                }
            }
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi note output
//---------------------------------------------------------------------------------------------------------------------------

template < typename T, typename U = T >
class midi_note_output_tmpl : public midi_note_processor {
protected:
    T* output_midi_buffer;

public:
    midi_note_output_tmpl( void ) : midi_note_processor( ), output_midi_buffer( nullptr ) { }

    // only creates notes, no input
    virtual void process_note( const midi_note_event& event ) override {
        if ( nullptr == output_midi_buffer ) {
            return;
        }

        if ( midi_note_event::k_note_on == event.event_type ) {
            this->output_midi_buffer->addEvent(
                U::noteOn(
                    event.channel,
                    event.note,
                    event.velocity
                ),
                0
            );
        } else if ( midi_note_event::k_note_off == event.event_type ) {
            this->output_midi_buffer->addEvent(
                U::noteOff(
                    event.channel,
                    event.note
                ),
                0
            );
        }
    };

    virtual void process( void ) override { };

    void set_output_midi_buffer( T& the_midi_buffer ) {
        output_midi_buffer = &the_midi_buffer;
    }
};

typedef midi_note_output_tmpl< juce::MidiBuffer, juce::MidiMessage > midi_note_output;

} // namespace note_processor

} // namespace midi_stuff

#include "midi_stuff/tests.hpp"