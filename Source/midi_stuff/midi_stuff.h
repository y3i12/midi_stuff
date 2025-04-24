#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <optional>
#include <type_traits>
#include <unordered_set>

#include "midi_stuff/tinyevents/tinyevents.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace midi_stuff {

//---------------------------------------------------------------------------------------------------------------------------
// midi note
//---------------------------------------------------------------------------------------------------------------------------

struct midi_note {
    typedef int8_t       note_t;
    typedef uint8_t      velocity_t;

    note_t               note;
    velocity_t           velocity;

    struct lt_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return lhs.note < rhs.note;
        }
    };

    struct le_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return lhs.note <= rhs.note;
        }
    };

    struct gt_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return lhs.note > rhs.note;
        }
    };

    struct ge_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return lhs.note >= rhs.note;
        }
    };

    struct eq_t {
        bool operator()( const midi_note& lhs, const midi_note& rhs ) const {
            return lhs.note == rhs.note;
        }
    };

    bool operator==( const midi_note& other ) const {
        return eq_t( )( *this, other );
    }

    bool operator==( note_t other ) const {
        return this->note == other;
    }

    midi_note( void ) :
        note(     0 ),
        velocity( 0 ) {
    }
    
    midi_note( const midi_note& other) :
        note(     other.note     ),
        velocity( other.velocity ) {
    }

    midi_note( int8_t the_note, uint8_t the_velocity ) :
        note(     the_note     ),
        velocity( the_velocity ) {
    }

    template < typename T >
    midi_note( T, int8_t the_note, uint8_t the_velocity ) :
        note(     the_note     ),
        velocity( the_velocity ) { }

};

//---------------------------------------------------------------------------------------------------------------------------
// midi channel note
//---------------------------------------------------------------------------------------------------------------------------

struct midi_channel_note : public midi_note {
    typedef int8_t       channel_t;

    channel_t            channel;

    midi_channel_note( void ) :
        midi_note( ),
        channel( 0 ) {
    }

    midi_channel_note( const midi_channel_note& other ) :
        midi_note( other.note, other.velocity ),
        channel( other.channel ) {
    }
    
    template < class T, class U, class V > 
    midi_channel_note( T channel, U note, V velocity ) :
        midi_note( static_cast< note_t     >( note     ), 
                   static_cast< velocity_t >( velocity ) ),
        channel(   static_cast< channel_t  >( channel  ) ) {
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: containers
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace containers {

//---------------------------------------------------------------------------------------------------------------------------
// sorted list
//---------------------------------------------------------------------------------------------------------------------------

template < typename type_t, typename comparator_t, typename container_t = std::list< typename type_t > >
class sorted_list {
public:
    using type                     = type_t;
    using comparator_type          = comparator_t;
    using container                = container_t;
    using iterator                 = container_t::iterator;
    using const_iterator           = container_t::const_iterator;
    using class_type               = sorted_list< type, comparator_type, container >;

    container                      notes;

    sorted_list( void ) { }

    inline size_t size( void ) const {
        return notes.size( );
    }

          container& operator  * ( void )       { return  notes; }
    const container& operator  * ( void ) const { return  notes; }
          container* operator -> ( void )       { return &notes; }
    const container* operator -> ( void ) const { return &notes; }

    std::pair< iterator, bool > find_if_compare( const midi_note& note ) {
        auto itr = std::find_if(
            notes.begin( ),
            notes.end( ),
            [ & ] ( auto existing_note ) {
                return !comparator_type( )( existing_note, note );
            }
        );

        return std::pair< iterator, bool >(
            itr,
            ( itr != notes.end( ) ) && !comparator_type( )( note, *itr )
        );
    }

    bool insert( const type_t& note ) {
        auto iterator_and_is_equal = find_if_compare( note );

        if ( !iterator_and_is_equal.second ) { // note wasn't found
            notes.insert( iterator_and_is_equal.first, note );
        }

        return !iterator_and_is_equal.second;
    };

    bool erase( const type_t& note ) {
        auto iterator_and_is_equal = find_if_compare( note );

        if ( iterator_and_is_equal.second ) { // note was found
            notes.erase( iterator_and_is_equal.first );
        }

        return iterator_and_is_equal.second;
    };



    // diff range first-last from a with first-last from b - expects both to be ordered in the same
    // criteria and that container has bool container::comparator_type( )( type& a, type& b )
    // implemented the results are inserted in in_a_not_in_b, in_b_not_in_a, in_both
    // and in_undefined ( which has tail that have shroedinger compare results )
    static void symmetric_difference_apply( const class_type& a, const class_type& b,
               std::optional < std::function< void( const midi_note& ) > > if_in_a_not_in_b,
               std::optional < std::function< void( const midi_note& ) > > if_in_b_not_in_a,
               std::optional < std::function< void( const midi_note& ) > > if_in_both      = std::nullopt,
               std::optional < std::function< void( const midi_note& ) > > if_in_undefined = std::nullopt ) {

        const_iterator first_a = a.notes.begin( );
        const_iterator last_a  = a.notes.end( );
        const_iterator first_b = b.notes.begin( );
        const_iterator last_b  = b.notes.end( );

        jassert( if_in_a_not_in_b || if_in_both || if_in_b_not_in_a );

        while ( first_a != last_a && first_b != last_b ) {
            if ( comparator_t( )( *first_a, *first_b ) ) {
                if ( auto callback = if_in_a_not_in_b ) ( *callback )( *first_a );
                ++first_a;

            } else if ( comparator_t( )( *first_b, *first_a ) ) {
                if ( auto callback = if_in_b_not_in_a ) ( *callback )( *first_b );
                ++first_b;

            } else {
                if ( auto callback = if_in_both ) ( *callback )( *first_a );
                ++first_a;
                ++first_b;
            }
        }

        // Additional checks for completing and removing the tail of the iterables
        // if the output for in_b_not_in_a is passed as param and first_a is at its final stage
        // copy range b into in_b_not_in_a
        if ( first_a == last_a ) {
            if ( auto callback = if_in_b_not_in_a ) std::for_each( first_b, last_b, *callback );

        // repeat the other side: if the output for in_a_not_in_b is passed as param and first_b is at its final stage
        // copy range a into in_a_not_in_b
        } else if ( first_b == last_b) {
            if ( auto callback = if_in_a_not_in_b ) std::for_each( first_a, last_a, *callback );

        // if execution reaches this point, there are two tails to be copied, which doesn't makes sense,
        // regardless the rest is copied into in_undefined, if provided. first a, then b
        } else if ( auto callback = if_in_undefined ) {
            std::for_each( first_a, last_a, *callback );
            std::for_each( first_b, last_b, *callback );
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// basic list types
//---------------------------------------------------------------------------------------------------------------------------

typedef sorted_list< midi_note, midi_note::lt_t > ascending_midi_note_list;
typedef sorted_list< midi_note, midi_note::gt_t > descending_midi_note_list;

}; // namespace containers


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: events
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace events {

//---------------------------------------------------------------------------------------------------------------------------
// event enumerations
//---------------------------------------------------------------------------------------------------------------------------

enum event_type_enum {
    k_note_on = 0,
    k_note_off = 1
};

//---------------------------------------------------------------------------------------------------------------------------
// event types
//---------------------------------------------------------------------------------------------------------------------------

struct note_on_event_t  { typedef note_on_event_t      type; static constexpr event_type_enum k_event_type = k_note_on;  };
struct note_off_event_t { typedef note_off_event_t     type; static constexpr event_type_enum k_event_type = k_note_off; };

//---------------------------------------------------------------------------------------------------------------------------
// base event wrapper
//---------------------------------------------------------------------------------------------------------------------------

template < typename event_type_t, typename base_t >
struct midi_note_event_base : public base_t {
    typedef          event_type_t                               event_type;
    typedef          base_t                                     base_type;
    typedef          midi_note_event_base< event_type, base_t > type;
    static constexpr event_type_enum k_event_type = event_type::k_event_type;

    midi_note_event_base( void ) :
        base_type( ) { }

    midi_note_event_base( const type& other ) :
        base_type( other ) { }

    midi_note_event_base( const base_type& other ) :
        base_type( other ) { }

    template < class T, class U, class V >
    midi_note_event_base( T channel, U note, V velocity )
#ifdef JUCE_VERSION
        requires ( !std::is_same_v< base_t, juce::MidiMessage > )
#endif
        : base_type( channel, note, velocity ) { }

    template < class U, class V >
    midi_note_event_base( U note, V velocity )
#ifdef JUCE_VERSION
            requires ( !std::is_same_v< base_t, juce::MidiMessage > )
#endif
        : base_type( note, velocity ) { }

#ifdef JUCE_VERSION
    template < class T, class U, class V >
    midi_note_event_base( T channel, U note, V velocity ) requires ( std::is_same_v< base_t, juce::MidiMessage > ) :
        base_type(
            k_event_type == k_note_on ?
            juce::MidiMessage::noteOn(  channel, note, velocity ) :
            juce::MidiMessage::noteOff( channel, note, velocity )
        ) { 
    };
#endif

};

//---------------------------------------------------------------------------------------------------------------------------
// event definition
//---------------------------------------------------------------------------------------------------------------------------

typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_on_event_t,  midi_channel_note > channel_note_on;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_off_event_t, midi_channel_note > channel_note_off;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_on_event_t,  midi_note >         note_on;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_off_event_t, midi_note >         note_off;

}; // namespace events


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// processors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace processors {

using dispatcher        = tinyevents::Dispatcher;
using dispatcher_handle = dispatcher::ListenerHandle;

enum parameter_type {
    k_midi_note,
    k_midi_channel_note,
    k_invalid,
};

class base_processor;

//---------------------------------------------------------------------------------------------------------------------------
// parameter port structure (both for input and output)
//---------------------------------------------------------------------------------------------------------------------------

class base_processor;

struct parameter_port {
    using extra_t = std::variant< void*, int > ;

    parameter_port( base_processor* the_owner, const std::string& the_name, parameter_type the_type, extra_t the_extra = 0 ) :
        owner( the_owner ), 
        name( the_name ), 
        dispatcher( nullptr ),
        provider_port( nullptr ),
        name_hash( std::hash< std::string >{}( name ) ),
        type( the_type ),
        extra( the_extra ) {
    }

    parameter_port( base_processor* the_owner, const char* the_name, parameter_type the_type, extra_t the_extra = 0 ) :
        parameter_port( the_owner, std::string( the_name ), the_type, the_extra ) {
    }

    bool operator  < ( const parameter_port& rhs ) { return name  < rhs.name; }
    bool operator == ( const parameter_port& rhs ) { return name == rhs.name; }
    
    struct hash {
        std::size_t operator()( const midi_stuff::processors::parameter_port* p ) const noexcept {
            return std::hash< std::string >{}( p->name );
        }
    };

    base_processor*       owner;
    dispatcher*           dispatcher; // allocated always in the output owner
    parameter_port*       provider_port; // for inputs, it points to the output port

    const std::string     name;
    const size_t          name_hash;
    parameter_type        type;

    extra_t               extra;
};

//---------------------------------------------------------------------------------------------------------------------------
// base_processor
//---------------------------------------------------------------------------------------------------------------------------

class base_processor {
protected:
    std::vector< parameter_port >                               input_ports;
    std::unordered_map< std::string, parameter_port* >          named_input_ports;

    std::vector< parameter_port >                               output_ports;
    std::unordered_map< std::string, parameter_port* >          named_output_ports;
    std::vector< dispatcher >                                   otput_dispatchers;
    std::unordered_set< parameter_port* >                       output_consumers;

    std::unordered_set< parameter_port*, parameter_port::hash > provided_inputs;
    bool                                                        has_processed;
public:
    using parameter_initializer_list =  std::initializer_list< std::tuple< std::string, parameter_type, parameter_port::extra_t > >;

public:
    template < typename T >
    struct create_listener {

    };

    struct binder {
        struct bond {
            struct parameter {
                parameter( base_processor* the_processor, std::string the_name ) : processor( the_processor ), name( the_name ) { }
                base_processor*   processor;
                std::string       name;
            };

            bond( const parameter& _from, const parameter& _to ) : from( _from ), to( _to ) { }
            bond(       parameter  _from,       parameter  _to ) : from( _from ), to( _to ) { }
            bond( base_processor* source_processor,
                  std::string source_parameter,
                  base_processor* destination_processor,
                  std::string destination_parameter ) :
                  from( source_processor, source_parameter ),
                  to( destination_processor, destination_parameter ) {
            }
            parameter from;
            parameter to;
        };

        binder( void ) { }

        binder( bond to_bind ) {
            auto* output_port = to_bind.from.processor->get_output( to_bind.from.name );
            auto* input_port =    to_bind.to.processor->get_input( to_bind.to.name    );

            // TODO: improve errors telling I/O
            jassert( nullptr != output_port );
            jassert( nullptr != output_port->dispatcher );
            jassert( nullptr != input_port );
           //  jassert( nullptr != input_port->dispatcher  );

            to_bind.from.processor->bind_output( *output_port, *input_port );
        }

        binder( std::initializer_list< bond > to_bind_list ) {
            std::for_each(
                to_bind_list.begin(),
                to_bind_list.end(),
                [ & ] ( bond to_bind ) {
                    binder bind_it( to_bind );
                }
            );
        }
    };

public:
    base_processor( parameter_initializer_list inputs,
                    parameter_initializer_list outputs ) : has_processed( false ) {

        input_ports.reserve( inputs.size( ) );
        for ( auto port_decl : inputs ) {
            auto [ name, type, extra ] = port_decl;
            input_ports.push_back( parameter_port( this, name, type, extra ) );
            auto& port_added = input_ports[ input_ports.size( ) - 1 ];
            named_input_ports.insert( std::pair< std::string, parameter_port* >( port_added.name, &port_added ) );
        }

        output_ports.reserve( outputs.size( ) );
        otput_dispatchers.reserve( outputs.size( ) );
        for ( auto port_decl : outputs ) {
            auto [ name, type, extra ] = port_decl;
            output_ports.push_back( parameter_port( this, name, type, extra ) );
            auto& port_added = output_ports[ output_ports.size( ) - 1 ];
            named_output_ports.insert( std::pair< std::string, parameter_port* >( port_added.name, &port_added ) );

            otput_dispatchers.push_back( dispatcher() );
            port_added.dispatcher = &otput_dispatchers[ otput_dispatchers.size( ) - 1 ];
        }
    }

    void bind_output( parameter_port& from, parameter_port& to ) {
        jassert( from.type == to.type );
        jassert( nullptr   == to.dispatcher );

        to.dispatcher    = from.dispatcher;
        to.provider_port = &from;
        output_consumers.insert( &to );

        // TODO: check for double listens and overriden bindings
        // TODO: store handles and make things unbindable
        switch ( to.type ) {
            case k_midi_note:
                from.dispatcher->listen< events::note_on >(
                    [ & ] ( const events::note_on& event ) {
                        to.owner->process_event( event, from, to );
                    }
                );
                from.dispatcher->listen< events::note_off >(
                    [ & ] ( const events::note_off& event ) {
                        to.owner->process_event( event, from, to );
                    }
                );
                break;
            case k_midi_channel_note:
                from.dispatcher->listen< events::channel_note_on >(
                    [ & ] ( const events::channel_note_on& event ) {
                        to.owner->process_event( event, from, to );
                    }
                );
                from.dispatcher->listen< events::channel_note_off >(
                    [ & ] ( const events::channel_note_off& event ) {
                        to.owner->process_event( event, from, to );
                    }
                );
                break;
            case k_invalid:
            default:
            jassert( false );
        }
    }

    parameter_port* get_output( const std::string& port_name ) {
        auto port_itr = named_output_ports.find( port_name );
        return port_itr == named_output_ports.end( ) ? nullptr : port_itr->second;
    }

    parameter_port* get_input( const std::string& port_name ) {
        auto port_itr = named_input_ports.find( port_name );
        return port_itr == named_input_ports.end( ) ? nullptr : port_itr->second;
    }

    virtual void process_event( const int&,                      parameter_port& , parameter_port& ) { };
    virtual void process_event( const float&,                    parameter_port& , parameter_port& ) { };
    virtual void process_event( const events::note_on&,          parameter_port& , parameter_port& ) { };
    virtual void process_event( const events::note_off&,         parameter_port& , parameter_port& ) { };
    virtual void process_event( const events::channel_note_on&,  parameter_port& , parameter_port& ) { };
    virtual void process_event( const events::channel_note_off&, parameter_port& , parameter_port& ) { };

    template < class T, class U, class V >
    bool enqueue_event( parameter_port& output_port, events::event_type_enum event_type, T channel, U note, V velocity ) {
        jassert( nullptr != output_port.dispatcher );
        jassert( named_output_ports.find( output_port.name ) != named_output_ports.end( ) );
        jassert( named_output_ports.find( output_port.name )->second->dispatcher == output_port.dispatcher );

        if ( events::k_note_on == event_type ){
            enqueue_event(
                output_port,
                events::channel_note_on(
                    static_cast< int8_t >( channel ),
                    static_cast< int8_t >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;

        } else if ( events::k_note_off == event_type ) {
            enqueue_event(
                output_port,
                events::channel_note_off(
                    static_cast< int8_t >( channel ),
                    static_cast< int8_t >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;
        }

        return false;
    }

    template < class U, class V >
    bool enqueue_event( parameter_port& output_port, events::event_type_enum event_type, U note, V velocity ) {
        jassert( nullptr != output_port.dispatcher );
        jassert( named_output_ports.find( output_port.name ) != named_output_ports.end( ) );
        jassert( named_output_ports.find( output_port.name )->second->dispatcher == output_port.dispatcher );

        if ( events::k_note_on == event_type ) {
            enqueue_event(
                output_port,
                events::note_on(
                    static_cast< int8_t  >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;

        } else if ( events::k_note_off == event_type ) {
            enqueue_event(
                output_port,
                events::note_off(
                    static_cast< int8_t  >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );

            return true;
        }

        return false;
    }

#ifdef JUCE_VERSION
    bool enqueue_event( parameter_port& output_port, const juce::MidiMessage& event ) {
        if ( event.isNoteOn( ) ) {
            enqueue_event(
                output_port,
                events::channel_note_on( event.getChannel( ), event.getNoteNumber( ), event.getVelocity( ) )
            );
            return true;

        } else if ( event.isNoteOff( ) ) {
            enqueue_event(
                output_port,
                events::channel_note_off( event.getChannel( ), event.getNoteNumber( ), event.getVelocity( ) )
            );
            return true;
        }

        return false;
    };
#endif

    template < typename T >
    bool enqueue_event( parameter_port& output_port, const T& the_event ) {
        jassert( nullptr != output_port.dispatcher );
        jassert( named_output_ports.find( output_port.name ) != named_output_ports.end( ) );
        jassert( named_output_ports.find( output_port.name )->second->dispatcher == output_port.dispatcher );

        output_port.dispatcher->queue( the_event );

        if      constexpr ( std::is_same_v< T, events::note_on          > ) return true;
        else if constexpr ( std::is_same_v< T, events::note_off         > ) return true;
        else if constexpr ( std::is_same_v< T, events::channel_note_on  > ) return true;
        else if constexpr ( std::is_same_v< T, events::channel_note_off > ) return true;
        else return false;
    }

    void dispatch_events( void ) {
        std::for_each(
            output_ports.begin( ),
            output_ports.end( ),
            [ & ] ( auto& output_port ) {
                jassert( nullptr != output_port.dispatcher );
                output_port.dispatcher->process( );
            }
        );
    }

    void set_provided( parameter_port* port ) {
        jassert( nullptr != port        );
        jassert( this    != port->owner );

        provided_inputs.insert( port );
    }

    void remove_provided( parameter_port* port ) {
        jassert( nullptr != port        );
        jassert( this    != port->owner );

        provided_inputs.erase( port );
    }

    void backtrack_process_execute( void ) {
        // TODO: avoid processing same node 2 times
        for ( auto& input_port : input_ports ) {
            if ( input_port.provider_port && provided_inputs.find( &input_port ) == provided_inputs.end( ) ) {
                input_port.provider_port->owner->backtrack_process_execute( );
            }
        }

        process( );
        dispatch_events( );

        for ( auto consumer : output_consumers ) {
            consumer->owner->set_provided( consumer->provider_port );
        }
    }

    void backtrack_process_clear( void ) {
        for ( auto& input_port : input_ports ) {
            if ( input_port.provider_port && provided_inputs.find( &input_port ) != provided_inputs.end( ) ) {
                input_port.provider_port->owner->backtrack_process_clear( );
            }
        }

        for ( auto consumer : output_consumers ) {
            consumer->owner->remove_provided( consumer->provider_port );
        }
    }

    void backtrack_process( void ) {
        backtrack_process_execute( );
        backtrack_process_clear( );
    }

    virtual void process( void ) { }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi note input 
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_note_input : public base_processor {
public:
    parameter_port& channel_notes;

public:
    midi_channel_note_input( void ) : 
        base_processor( {}, { { "channel_notes", k_midi_channel_note, 0 } } ),
        channel_notes( *get_output( "channel_notes" ) ) {
    }

    template < class T, class U, class V >
    void note_on( T channel, U note, V velocity ) {
        enqueue_event( channel_notes, events::k_note_on, channel, note, velocity );
    }

    template < class T, class U, class V >
    void note_off( T channel, U note, V velocity ) {
        enqueue_event( channel_notes, events::k_note_off, channel, note, velocity );
    }

#ifdef JUCE_VERSION
    bool generate_events( juce::MidiBuffer& midi_buffer, bool clear = false ) {
        bool needs_processing = false;
        std::for_each(
            midi_buffer.begin( ),
            midi_buffer.end( ),
            [ & ] ( juce::MidiBufferIterator::reference buffer_itr ) {
                needs_processing = enqueue_event( channel_notes, buffer_itr.getMessage( ) );
            }
        );

        if ( clear ) {
            midi_buffer.clear( );
        }

        return needs_processing;
    }
#endif
};

//---------------------------------------------------------------------------------------------------------------------------
// midi channel split
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_split : public base_processor {
public:
    std::vector< parameter_port* > channel_output_ports;

public:
    midi_channel_split( void ) :
        base_processor( 
            {
                { "notes",      k_midi_channel_note, 0 }
            },
            {
                { "channel_1",  k_midi_note,  1 },
                { "channel_2",  k_midi_note,  2 },
                { "channel_3",  k_midi_note,  3 },
                { "channel_4",  k_midi_note,  4 },
                { "channel_5",  k_midi_note,  5 },
                { "channel_6",  k_midi_note,  6 },
                { "channel_7",  k_midi_note,  7 },
                { "channel_8",  k_midi_note,  8 },
                { "channel_9",  k_midi_note,  9 },
                { "channel_10", k_midi_note, 10 },
                { "channel_11", k_midi_note, 11 },
                { "channel_12", k_midi_note, 12 },
                { "channel_13", k_midi_note, 13 },
                { "channel_14", k_midi_note, 14 },
                { "channel_15", k_midi_note, 15 },
                { "channel_16", k_midi_note, 16 },
            }
        ),
        channel_output_ports(
            {
                get_output( "channel_1"  ),
                get_output( "channel_2"  ),
                get_output( "channel_3"  ),
                get_output( "channel_4"  ),
                get_output( "channel_5"  ),
                get_output( "channel_6"  ),
                get_output( "channel_7"  ),
                get_output( "channel_8"  ),
                get_output( "channel_9"  ),
                get_output( "channel_10" ),
                get_output( "channel_11" ),
                get_output( "channel_12" ),
                get_output( "channel_13" ),
                get_output( "channel_14" ),
                get_output( "channel_15" ),
                get_output( "channel_16" ),
            }
        ) {
    }

    virtual void process_event( const events::channel_note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };
 
    virtual void process_event( const events::channel_note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };

private:
    template < typename T >
    inline void process_event_tmpl( const T& event ) {
        jassert( event.channel > 0 && event.channel <= 16 );
        enqueue_event( *channel_output_ports[ event.channel - 1 ], event.k_event_type, event.note, event.velocity );
    };
};


//---------------------------------------------------------------------------------------------------------------------------
// midi channel filter
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_filter : public base_processor {
public:
    std::vector< parameter_port* > channel_output_ports;
    std::vector< bool >            enable_output;
public:
    midi_channel_filter( void ) :
        base_processor(
            {
                { "channel_1",     k_midi_note,  1 },
                { "channel_2",     k_midi_note,  2 },
                { "channel_3",     k_midi_note,  3 },
                { "channel_4",     k_midi_note,  4 },
                { "channel_5",     k_midi_note,  5 },
                { "channel_6",     k_midi_note,  6 },
                { "channel_7",     k_midi_note,  7 },
                { "channel_8",     k_midi_note,  8 },
                { "channel_9",     k_midi_note,  9 },
                { "channel_10",    k_midi_note, 10 },
                { "channel_11",    k_midi_note, 11 },
                { "channel_12",    k_midi_note, 12 },
                { "channel_13",    k_midi_note, 13 },
                { "channel_14",    k_midi_note, 14 },
                { "channel_15",    k_midi_note, 15 },
                { "channel_16",    k_midi_note, 16 },
            },
            {
                { "channel_1",     k_midi_note,  1 },
                { "channel_2",     k_midi_note,  2 },
                { "channel_3",     k_midi_note,  3 },
                { "channel_4",     k_midi_note,  4 },
                { "channel_5",     k_midi_note,  5 },
                { "channel_6",     k_midi_note,  6 },
                { "channel_7",     k_midi_note,  7 },
                { "channel_8",     k_midi_note,  8 },
                { "channel_9",     k_midi_note,  9 },
                { "channel_10",    k_midi_note, 10 },
                { "channel_11",    k_midi_note, 11 },
                { "channel_12",    k_midi_note, 12 },
                { "channel_13",    k_midi_note, 13 },
                { "channel_14",    k_midi_note, 14 },
                { "channel_15",    k_midi_note, 15 },
                { "channel_16",    k_midi_note, 16 },
            }
            ),
        channel_output_ports(
            {
                get_output( "channel_1" ),
                get_output( "channel_2" ),
                get_output( "channel_3" ),
                get_output( "channel_4" ),
                get_output( "channel_5" ),
                get_output( "channel_6" ),
                get_output( "channel_7" ),
                get_output( "channel_8" ),
                get_output( "channel_9" ),
                get_output( "channel_10" ),
                get_output( "channel_11" ),
                get_output( "channel_12" ),
                get_output( "channel_13" ),
                get_output( "channel_14" ),
                get_output( "channel_15" ),
                get_output( "channel_16" ),
            }
        ),
        enable_output( 16, true ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_event_tmpl( event, to );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_event_tmpl( event, to );
    };

private:
    template < typename T >
    inline void process_event_tmpl( const T& event, parameter_port& to ) {
        jassert( std::get< int >( to.extra ) > 0 && std::get< int >( to.extra ) <= 16 );

        if ( enable_output[ std::get< int >( to.extra ) - 1 ] ) {
            enqueue_event( *channel_output_ports[ std::get< int >( to.extra ) - 1 ], event );
        }
    };
};


//---------------------------------------------------------------------------------------------------------------------------
// midi channel join
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_join : public base_processor {
public:
    parameter_port& channel_output;

public:
    midi_channel_join( void ) :
        base_processor(
            {
                { "channel_1",     k_midi_note,  1 },
                { "channel_2",     k_midi_note,  2 },
                { "channel_3",     k_midi_note,  3 },
                { "channel_4",     k_midi_note,  4 },
                { "channel_5",     k_midi_note,  5 },
                { "channel_6",     k_midi_note,  6 },
                { "channel_7",     k_midi_note,  7 },
                { "channel_8",     k_midi_note,  8 },
                { "channel_9",     k_midi_note,  9 },
                { "channel_10",    k_midi_note, 10 },
                { "channel_11",    k_midi_note, 11 },
                { "channel_12",    k_midi_note, 12 },
                { "channel_13",    k_midi_note, 13 },
                { "channel_14",    k_midi_note, 14 },
                { "channel_15",    k_midi_note, 15 },
                { "channel_16",    k_midi_note, 16 },
            },
            {
                { "channel_notes", k_midi_channel_note, 0 }
            }
            ),
        channel_output( *get_output( "channel_notes" ) ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_event_tmpl( event, to );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_event_tmpl( event, to );
    };

private:
    template < typename T >
    inline void process_event_tmpl( const T& event, parameter_port& to ) {
        jassert( std::get< int >( to.extra ) > 0 && std::get< int >( to.extra ) <= 16 );
        enqueue_event( channel_output, event.k_event_type, std::get< int >( to.extra ), event.note, event.velocity );
    };
};

//---------------------------------------------------------------------------------------------------------------------------
// midi note dedup
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_dedup : public base_processor {
public:
    parameter_port& output_port;
    uint8_t         refcount[ 128 ];

public:
    midi_note_dedup( void ) :
        base_processor( { { "notes", k_midi_note, 0 } },
                        { { "notes", k_midi_note, 0 } } ),
        output_port( *get_output( "notes" ) ),
        refcount( { 0 } ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );

        if ( 1 == ++refcount[ event.note ] ) {
            enqueue_event( output_port, event );
        }
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        
        if ( 0 == refcount[ event.note ] ) return;
        
        if ( 0 == --refcount[ event.note ] ) {
            enqueue_event( output_port, event );
        }
    };
};

//---------------------------------------------------------------------------------------------------------------------------
// midi sorted note store
//---------------------------------------------------------------------------------------------------------------------------

template < typename container_t >
class midi_sorted_note_store : public base_processor {
public:
    typedef               container_t          container;

    typedef midi_sorted_note_store< container_t > type;

    typedef base_processor::parameter_initializer_list parameter_initializer_list;

public:
    container       notes;

public:
    midi_sorted_note_store( parameter_initializer_list inputs,
                            parameter_initializer_list outputs ) : base_processor( inputs, outputs ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        notes.insert( event );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        notes.erase( event );
    };

    virtual void process_event( const events::channel_note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        notes.insert( event );
    };

    virtual void process_event( const events::channel_note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        notes.erase( event );
    };
};

//---------------------------------------------------------------------------------------------------------------------------
// midi lowest and highest notes
//---------------------------------------------------------------------------------------------------------------------------

template < typename container_t >
class midi_first_n_notes_of_container : public midi_sorted_note_store< container_t > {
public:
    typedef container_t                                  container;
    typedef midi_sorted_note_store< container >          base_type;
    typedef midi_first_n_notes_of_container< base_type > type;
    typedef base_type::parameter_initializer_list        parameter_initializer_list;

public:
    container       active_notes;
    uint8_t         number_notes_kept;
    parameter_port& output_port;

public:
    midi_first_n_notes_of_container( uint8_t the_number_notes_kept = 128 ) :
        midi_sorted_note_store< container_t >( { { "notes", k_midi_note, 0 } }, { { "notes", k_midi_note, 0 } } ),
        active_notes( ),
        number_notes_kept( the_number_notes_kept ),
        output_port( *this->get_output( "notes" ) ) {
    }

    virtual void process( void ) override {
        auto  active_notes_itr       = this->active_notes->begin( );
        auto  notes_itr              = this->notes->begin( );

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
                active_notes_itr != this->active_notes->end( ) &&
                notes_itr != this->notes->end( ) ) {

            if ( container::comparator_type( )( *active_notes_itr, *notes_itr ) ) {
                // A < B -> the lowest note in this is lower than in midi_note_source,
                // then it is not present on the new notes: note_off
                this->enqueue_event( output_port, events::k_note_off, active_notes_itr->note, active_notes_itr->velocity );
                active_notes_itr = this->active_notes->erase( active_notes_itr );

            } else if ( container::comparator_type( )( *notes_itr, *active_notes_itr ) ) {
                // B < A -> the lowest note in midi_note_source is lower than in this,
                // then it is a new note to be added
                this->active_notes->insert( active_notes_itr, *notes_itr );

                this->enqueue_event( output_port, events::k_note_on, notes_itr->note, notes_itr->velocity );
                ++notes_itr;
                ++current_note_num;

            } else {
                // equals - they're in both. it is cool, nothing to be done
                ++active_notes_itr;
                ++notes_itr;
                ++current_note_num;
            }
        }

        // Additional checks for completing and removing the tail of current notes
        if ( active_notes_itr == this->active_notes->end( ) ) { // complete
            // If all pre-existing notes were iterated through, it means that there might be additional tail notes to note_on
            // Tries to fill the current notes with the notes from midi_note_source till number_notes_kept notes are on
            while ( current_note_num    <  number_notes_kept          && 
                    notes_itr           != this->notes->end( ) ) {
                this->active_notes->push_back( *notes_itr );
                this->enqueue_event( output_port, events::k_note_on, notes_itr->note, notes_itr->velocity );
                ++current_note_num;
                ++notes_itr;
            }
        } else if ( ( current_note_num == number_notes_kept ) || ( notes_itr == this->notes->end( ) ) ) { // remove tail
            // if the iterator isn't pointing at the end and the desired number of notes were collected, note_off the tail of the list
            while ( active_notes_itr != this->active_notes->end( ) ) {
                this->enqueue_event( output_port, events::k_note_off, active_notes_itr->note, active_notes_itr->velocity );
                active_notes_itr = this->active_notes->erase( active_notes_itr );
            }
        }
    };
};

typedef midi_first_n_notes_of_container< containers::ascending_midi_note_list  > midi_lowest_notes;
typedef midi_first_n_notes_of_container< containers::descending_midi_note_list > midi_highest_notes;

//---------------------------------------------------------------------------------------------------------------------------
// midi note arpeggiator
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_arpeggiator : public midi_sorted_note_store< containers::ascending_midi_note_list > {
public:
    using container                     = containers::ascending_midi_note_list;
    using container_type                = container::container;
    using base_type                     = midi_sorted_note_store< container >;
    using type                          = midi_first_n_notes_of_container< base_type >;
    using parameter_initializer_list    = base_type::parameter_initializer_list;

    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

public:
    static const int                k_chord_notes    = 0;
    static const int                k_sequence_notes = 1;

    parameter_port&                 output_port; // this->notes is treated as output as it has a generic name
    container                       chord_notes;
    container                       sequence_notes;
    bool                            im_a_dirty_object;


public:
    midi_note_arpeggiator( ) :
        base_type(
            {
                { "chord_notes",    k_midi_note, k_chord_notes    },
                { "sequence_notes", k_midi_note, k_sequence_notes }
            },
            {
                { "notes", k_midi_note, 0 }
            }
        ),
        chord_notes( ),
        sequence_notes( ),
        im_a_dirty_object( false ),
        output_port( *this->get_output( "notes" ) ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );
        switch ( std::get< int >( to.extra ) ) {
            case k_chord_notes:    if ( chord_notes.insert(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes: if ( sequence_notes.insert( event ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );

        switch ( std::get< int >( to.extra ) ) {
            case k_chord_notes:    if ( chord_notes.erase(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes: if ( sequence_notes.erase( event ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    };

    virtual void process( void ) override {
        if ( im_a_dirty_object ) {
            if ( chord_notes.notes.empty( ) || sequence_notes.notes.empty( ) ) {
                // enqueue offs for whatever is on the output port
                std::for_each(
                    notes.notes.begin( ),
                    notes.notes.end(   ),
                    [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                        enqueue_event( output_port, events::note_off( note ) );
                    } 
                );
                notes.notes.clear( );
                im_a_dirty_object = false;
                return;
            }

            std::vector< std::reference_wrapper< midi_note > > chord_notes_vector( chord_notes.notes.begin( ), chord_notes.notes.end( ) );
            container active_notes;

            // arp loop, computes sequence_notes X chord_notes into active_notes
            std::for_each(
                sequence_notes.notes.begin( ),
                sequence_notes.notes.end( ),
                [ & ] ( const midi_note& sequence_note ) {
                    auto index         =   static_cast< int >( sequence_note.note ) % chord_notes_vector.size( );
                    auto octave_shift  = ( static_cast< int >( sequence_note.note ) / chord_notes_vector.size( ) ) * 12;

                    int8_t note_value  = static_cast< int8_t >( chord_notes_vector[ index ].get( ).note );
                    note_value        += static_cast< int8_t >( octave_shift );

                    // if the arp generate a note out of range, it is folded back into range
                    if ( note_value > 127 ) {
                        note_value -= ( note_value / 120 ) * 120;
                    }

                    midi_note new_note( note_value, sequence_note.velocity );
                    active_notes.insert( new_note );
                }
            );

            // based on what was outputted previous time, it can be verified what's note on and off
            container::symmetric_difference_apply( active_notes, notes,
                [ & ] ( const midi_note& note ) { // in active_notes (freshly computed) but not  - note on
                    enqueue_event( output_port, events::note_on( note ) );
                },
                [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                    enqueue_event( output_port, events::note_off( note ) );
                }
            );

            // new notes are now old
            notes.notes       = active_notes.notes;
            im_a_dirty_object = false; // good boi
        }
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi note output
//---------------------------------------------------------------------------------------------------------------------------

template < typename buffer_t, typename note_adder_t, parameter_type k_parameter_type >
class midi_note_output_tmpl : public base_processor {
public:
    typedef note_adder_t note_adder_type;
    typedef buffer_t     buffer_type;

public:
    buffer_type*                 output_midi_buffer;
    static const int8_t          k_default_channel = 1;

public:
    midi_note_output_tmpl( void ) : 
        base_processor( { { "notes", k_parameter_type, 0 } }, { } ),
        output_midi_buffer( nullptr )
        { }

    virtual void process_event( const events::note_on & event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };

    virtual void process_event( const events::channel_note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };

    virtual void process_event( const events::channel_note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        process_event_tmpl( event );
    };

    void set_output_midi_buffer( buffer_type& the_midi_buffer ) {
        output_midi_buffer = &the_midi_buffer;
    }

private:
    template < typename T >
    inline void process_event_tmpl( const T& event ) {
        jassert( nullptr != output_midi_buffer );
        jassert( event.note >= 0 && event.note < 128 );

        note_adder_type::add_to_buffer( *output_midi_buffer, event );
    };
};

#ifdef JUCE_VERSION
struct juce_midi_buffer_note_adder {
    static void add_to_buffer( juce::MidiBuffer& midi_buffer, const events::note_on& event ) {
        midi_buffer.addEvent(
            juce::MidiMessage::noteOn(
                1, // defaults to channel 1
                event.note,
                event.velocity
            ),
            0
        );
    }

    static void add_to_buffer( juce::MidiBuffer& midi_buffer, const events::note_off& event ) {
        midi_buffer.addEvent(
            juce::MidiMessage::noteOff(
                1, // defaults to channel 1
                event.note,
                event.velocity
            ),
            0
        );
    }

    static void add_to_buffer( juce::MidiBuffer& midi_buffer, const events::channel_note_on& event ) {
        midi_buffer.addEvent(
            juce::MidiMessage::noteOn(
                event.channel,
                event.note,
                event.velocity
            ),
            0
        );
    }

    static void add_to_buffer( juce::MidiBuffer& midi_buffer, const events::channel_note_off& event ) {
        midi_buffer.addEvent(
            juce::MidiMessage::noteOff(
                event.channel,
                event.note,
                event.velocity
            ),
            0
        );
    }

};

typedef midi_note_output_tmpl< juce::MidiBuffer, juce_midi_buffer_note_adder, k_midi_note         > midi_note_output;
typedef midi_note_output_tmpl< juce::MidiBuffer, juce_midi_buffer_note_adder, k_midi_channel_note > midi_channel_note_output;
#endif

} // namespace note_processor

} // namespace midi_stuff

#include "midi_stuff/tests.hpp"