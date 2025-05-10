#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <optional>
#include <type_traits>
#include <unordered_set>

#include "etl/bitset.h"
#include "tinyevents/tinyevents.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace midi_stuff {

using note_t     = uint8_t;
using velocity_t = uint8_t;
using channel_t  = int8_t;

// FWD Decl
namespace containers {
    template < typename type_t, typename comparator_t, typename container_t = std::list< typename type_t > > class sorted_list;
}

struct midi_note;

namespace chord_data {

static const std::array< std::string, 12 > note_names     = { "C",      "C#",  "D",   "Eb",   "E",  "F",   "F#",   "G",   "Ab",   "A",  "Bb",  "B" };
static const std::array< std::string, 12 > degrees        = { "root",   "b2",  "2",   "b3",   "3",   "4",   "b5",  "5",   "b6",   "6",  "b7",  "7" };
static const std::array< std::string, 12 > compound_tones = { "octave", "b9",  "9",  "b10",  "10",  "11",  "#11",  "5",  "b13",  "13",  "B7",  "7" };

} // namespace chord_data
// end FWD Decl

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: utils
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace utils {

// diff range first-last from a with first-last from b - expects both to be ordered in the same
// criteria and that container has bool container::comparator_type( )( type& a, type& b )
// implemented the results are inserted in in_a_not_in_b, in_b_not_in_a, in_both
// and in_undefined ( which has tail that have shroedinger compare results )
template < typename type_t, typename comparator_t, typename container_t = std::list< typename type_t >, typename const_iterator = typename container_t::const_iterator >
static void symmetric_difference_apply( const midi_stuff::containers::sorted_list< type_t, comparator_t, container_t >& a, const midi_stuff::containers::sorted_list< type_t, comparator_t, container_t >& b,
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
    } else if ( first_b == last_b ) {
        if ( auto callback = if_in_a_not_in_b ) std::for_each( first_a, last_a, *callback );

    // if execution reaches this point, there are two tails to be copied, which doesn't makes sense,
    // regardless the rest is copied into in_undefined, if provided. first a, then b
    } else if ( auto callback = if_in_undefined ) {
        std::for_each( first_a, last_a, *callback );
        std::for_each( first_b, last_b, *callback );
    }
}

// Brian Kernighan’s Algorithm
template < typename T >
uint8_t count_bits( T n ) {
    uint8_t count = 0;

    while ( n ) {
        n &= ( n - 1 );
        count++;
    }

    return count;
}

} // namespace utils

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: math
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace math {

template < class T >
inline constexpr T pow( const T, std::integral_constant< size_t, 0 > ) {
    return 1;
}

template < class T, size_t factor >
inline constexpr T pow( const T x, std::integral_constant< size_t, factor > ) {
    return pow( x, std::integral_constant< size_t, factor - 1 >( ) ) * x;
}

template < size_t factor, class T >
inline constexpr T pow( const T x ) {
    return pow( x, std::integral_constant< size_t, factor >( ) );
}

} // namespace math


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: midi note
//---------------------------------------------------------------------------------------------------------------------------

struct midi_note {
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

    operator uint16_t( ) const {
        return static_cast< uint16_t >( ( note << 8 ) & velocity );
    }

    operator note_t( ) const {
        return note;
    }

    midi_note& operator=( note_t the_note ) {
        note     = static_cast< note_t     >( the_note );
        velocity = static_cast< velocity_t >( 100 );

        return *this;
    }

    midi_note& operator=( uint16_t value ) {
        note     = static_cast< note_t     >( ( value >> 8 ) & 0xFF );
        velocity = static_cast< velocity_t >(   value        & 0xFF );

        return *this;
    }

    midi_note( void ) :
        note(     0 ),
        velocity( 0 ) {
    }
    
    midi_note( const midi_note& other) :
        note(     other.note     ),
        velocity( other.velocity ) {
    }

    midi_note( int8_t the_note, uint8_t the_velocity = 100 ) :
        note(     the_note     ),
        velocity( the_velocity ) {
    }

    template < typename T >
    midi_note( T, int8_t the_note, uint8_t the_velocity ) :
        note(     the_note     ),
        velocity( the_velocity ) { }


    template < typename T >
    inline static const std::string& name( T note ) {
        return chord_data::note_names[ static_cast< size_t >( note ) % chord_data::note_names.size( ) ];
    }
};

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: midi channel note
//---------------------------------------------------------------------------------------------------------------------------

struct midi_channel_note : public midi_note {
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
// midi stuff :: chord_data
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace chord_data {

typedef std::vector< note_t > note_set;

template < class _note_set_t >
struct _named_note_set {
    using note_set_t   = _note_set_t;
    using type         = typename _named_note_set< note_set_t >;

    using scale_code_t = typename etl::bitset< 16, uint16_t >;

    std::string        name;
    note_set_t         notes;

    scale_code_t       ms_notes_code; //  most significant
    scale_code_t       ls_notes_code; // least significant

    scale_code_t       scale_code;

    _named_note_set( const note_set_t&  the_ms_notes,
                     const note_set_t&  the_ls_notes,
                     const std::string& the_name ) :
            name( the_name ) {

        // notes = the_ms_notes + the_ls_notes
        notes.reserve( the_ms_notes.size( ) + the_ls_notes.size( ) );
        std::copy( the_ms_notes.begin( ), the_ms_notes.end( ), std::back_inserter( notes ) );
        std::copy( the_ls_notes.begin( ), the_ls_notes.end( ), std::back_inserter( notes ) );

        set_scale_code( the_ms_notes, ms_notes_code );
        set_scale_code( the_ls_notes, ls_notes_code );

        scale_code = ms_notes_code | ls_notes_code;
    }

    _named_note_set( const note_set_t& the_ms_notes,
                     const note_set_t& the_ls_notes,
                     const char* the_name ) :
            _named_note_set( the_ms_notes, the_ls_notes, std::string( the_name ) ) { }

    template < typename T >
    static void set_scale_code( const T& iteratble_note_container, scale_code_t& output_scale_code, note_t root_shift = 0 ) {
        output_scale_code.reset( );
        size_t shifted_note = 0;

        std::for_each(
            iteratble_note_container.begin( ),
            iteratble_note_container.end( ),
            [ & ] ( const midi_stuff::midi_note& note ) {
                shifted_note = ( static_cast< size_t >( note.note ) + 12 - root_shift ) % 12;
                if ( shifted_note > 0 ) output_scale_code.set( shifted_note, true ); // skip roots
            }
        );
    }

    template < typename T >
    static void set_notes_from_scale_code( const scale_code_t& scale_code, T& insertable_note_container, note_t root_shift = 0 ) {
        insertable_note_container.clear( );

        for ( size_t i = 0; i < scale_code.size( ); ++i ) {
            if ( scale_code.test( i ) ) {
                insertable_note_container.insert(
                    midi_note(
                        static_cast< note_t     >( i + root_shift ),
                        static_cast< velocity_t >(            100 ) // arbitrary velocity
                    )
                );
            }
        }
    }
};

typedef _named_note_set< note_set >  named_noteset;
typedef std::vector< named_noteset > named_noteset_list;

static const named_noteset_list chord_descriptors = {
    // MSn                LSn               name
    { { 2, 7          }, {               }, "sus2"     },
    { { 2, 10,        }, {       7       }, "7sus2"    },
    { { 2, 11,        }, {       7       }, "maj7sus2" },
    { { 3, 7          }, {               }, "minor"    },
    { { 3, 9          }, {       7       }, "m6"       },
    { { 3, 10,        }, {       7       }, "m7"       },
    { { 3, 10, 2      }, {       7       }, "m9"       },
    { { 3, 10, 5      }, { 2,    7       }, "m11"      },
    { { 3, 10, 9,     }, { 2, 5, 7       }, "m13"      },
    { { 3, 11         }, {       7       }, "mMaj7"    },
    { { 3, 11, 2      }, {       7       }, "mMaj9"    },
    { { 3, 11, 5      }, { 2,    7       }, "mMaj11"   },
    { { 3, 11, 9      }, { 2, 5, 7       }, "mMaj13"   },
    { { 3, 6, 10      }, {               }, "ø"        },
    { { 3, 6          }, {               }, "dim"      },
    { { 3, 6, 9       }, {               }, "dim7"     },
    { { 3, 6, 2,      }, {    9          }, "dim9"     },
    { { 3, 6, 5,      }, { 2, 9          }, "dim11"    },
    { { 4, 7          }, {               }, "major"    },
    { { 4, 9          }, {       7       }, "6"        },
    { { 4, 10,        }, {       7       }, "7"        },
    { { 4, 2,         }, {       7, 10   }, "9"        },
    { { 4, 5,         }, { 2,    7, 10   }, "11"       },
    { { 4, 9,         }, { 2, 5, 7, 10   }, "13"       },
    { { 4, 11         }, {       7       }, "maj7"     },
    { { 4, 11, 2      }, {       7       }, "maj9"     },
    { { 4, 11, 5      }, { 2,    7       }, "maj11"    },
    { { 4, 11, 9      }, { 2, 5, 7       }, "maj13"    },
    { { 4, 8          }, {               }, "aug"      },
    { { 4, 8, 10      }, {               }, "aug7"     },
    { { 4, 8, 10, 2   }, {               }, "aug9"     },
    { { 4, 8, 10, 5   }, { 2,            }, "aug11"    },
    { { 4, 8, 10, 9   }, { 2, 5,         }, "aug13"    },
    { { 4, 8, 11      }, {               }, "augMaj7"  },
    { { 4, 8, 11, 2   }, {               }, "augMaj9"  },
    { { 4, 8, 11, 5   }, { 2             }, "augMaj11" },
    { { 4, 8, 11, 9   }, { 2, 5          }, "augMaj13" },
    { { 5, 7          }, {               }, "sus4"     },
    { { 5, 10,        }, {       7       }, "7sus4"    },
    { { 5, 2, 10      }, {       7       }, "9sus4"    },
    { { 5, 11,        }, {       7       }, "maj7sus4" },
    { { 5, 2, 11      }, {       7       }, "maj9sus4" },

    { { 0             }, { 0             }, "guard"    } // Security terminator
};

// get_best_fitting_named_noteset returns a note set with the most fitting chord scale (according to the algo, not to music theory)
//
// highly based on the post how [chordcat works](https://blog.s20n.dev/posts/how-chordcat-works/) and
// its [chord_db](https://github.com/shriramters/chordcat/blob/main/src/chord_db.hpp) idea.
//
// the std::find_if has a similar role of the loop implemented in
// chordcat::insert_chords(const unsigned short root, const std::set<unsigned short>& intervals, std::multiset<Chord>& res)
// see: https://github.com/shriramters/chordcat/blob/863c147999266b26ec06058c7766d5830b561fd6/src/utils.hpp#L32
//
// This algorithm uses the splitted chord definition in the static vector `chord_descriptors`. The two parts are named as
// most significant notes (MSn) notes and least significant notes (LSn), having each one of the "nibbles" stored in bit sets
// of the type named_noteset::scale_code_t in named_noteset's variables ms_notes_code, ls_notes_code, scale_code - the last 
// contains the union of MSn and LSn.
//
// Each bit in scale_code_t states the presence of a given note ( 0 <= note < 12 ) in the chord, by iterating through the chord
// descriptors, three values are extracted: number of notes matching the MSn, number of notes matching LSn and the overall distance
// of the chord from the input (number of different notes). MSn, LSn and distance are used as a heuristic to determine the chord
//

const named_noteset& get_best_fitting_named_noteset( const named_noteset::scale_code_t& input_scale_code ) {
    const named_noteset* best_fitting_named_noteset = &chord_descriptors.back( ); // set it to guard, always returning a valid hypothesis
    int32_t              max_score                  = std::numeric_limits< int32_t >::min( );

    auto discard_itr = std::find_if(
        chord_descriptors.begin( ),
        chord_descriptors.end( ),
        [ & ]( const named_noteset& hypothesis_chord ) {
            int32_t distance_score = static_cast< int32_t >( ( input_scale_code & ~hypothesis_chord.scale_code ).count( ) ); // Unmatching input scale notes

            // short circuit on a perfect match
            if ( 0 == distance_score ) {
                return true;
            }

            int32_t ms_score = static_cast< int32_t >( ( input_scale_code &  hypothesis_chord.ms_notes_code ).count( ) ); // matching MSn
            int32_t ls_score = static_cast< int32_t >( ( input_scale_code &  hypothesis_chord.ls_notes_code ).count( ) ); // matching LSn
            int32_t score    = math::pow< 3 >( ms_score ) + math::pow< 2 >( ls_score ) - distance_score;

            if ( score > max_score ) {
                max_score                  = score;
                best_fitting_named_noteset = &hypothesis_chord;
            }

            return false;
        }
    );

    return *best_fitting_named_noteset;
}

template < typename T >
const named_noteset& get_best_fitting_named_noteset( const T& iterable_note_container, const note_t root_shift ) {
    named_noteset::scale_code_t input_scale_code;
    named_noteset::set_scale_code( iterable_note_container, input_scale_code, root_shift );

    return get_best_fitting_named_noteset( input_scale_code );
}

} // namespace chord_data


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: scale_data
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace scale_data {

static const chord_data::named_noteset_list scale = {
    { { 2, 4, 5, 7, 9, 11 }, { }, "Major"               },
    { { 2, 3, 5, 7, 9, 10 }, { }, "Dorian"              },
    { { 1, 3, 5, 7, 8, 10 }, { }, "Phrygian"            },
    { { 2, 4, 6, 7, 9, 11 }, { }, "Lydian"              },
    { { 2, 4, 5, 7, 9, 10 }, { }, "Mixolydian"          },
    { { 2, 3, 5, 7, 8, 10 }, { }, "Aeolian"             },
    { { 1, 3, 5, 6, 8, 10 }, { }, "Locrian"             },
    { { 2, 3, 5, 7, 8, 11 }, { }, "Harmonic Minor"      },
    { { 1, 3, 5, 6, 9, 10 }, { }, "Locrian Natural 6"   },
    { { 2, 4, 5, 8, 9, 11 }, { }, "Augmented Major"     },
    { { 2, 3, 6, 7, 9, 10 }, { }, "Dorian #11"          },
    { { 1, 4, 5, 7, 8, 10 }, { }, "Phrygian Dominant"   },
    { { 3, 4, 6, 7, 9, 11 }, { }, "Lydian #2"           },
    { { 1, 3, 4, 6, 8,  9 }, { }, "Super Locrian bb7"   },
    { { 2, 3, 5, 7, 9, 11 }, { }, "Jazz Minor"          },
    { { 1, 3, 5, 7, 9, 10 }, { }, "Dorian b2"           },
    { { 2, 4, 6, 8, 9, 11 }, { }, "Lydian Augmented"    },
    { { 2, 4, 6, 7, 9, 10 }, { }, "Lydian Dominant"     },
    { { 2, 4, 5, 7, 8, 10 }, { }, "Aeolian Dominant"    },
    { { 2, 3, 5, 6, 8, 10 }, { }, "Half-diminished"     },
    { { 1, 3, 4, 6, 8, 10 }, { }, "Altered"             },
};

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: containers
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace containers {

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: containers :: sorted list
//---------------------------------------------------------------------------------------------------------------------------

template < typename type_t, typename comparator_t, typename container_t /* = std::list< typename type_t > */ >
class sorted_list {
public:
    //using type                     = type_t;
    using comparator_type          = comparator_t;
    using container                = container_t;
    using iterator                 = container_t::iterator;
    using const_iterator           = container_t::const_iterator;
    using class_type               = sorted_list< type_t, comparator_type, container >;

    container                      notes;

    sorted_list( void ) { }

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

    inline void clear( void ) {
        notes.clear( );
    }

    type_t& front( void ) noexcept {
        return notes.front( );
    }

    const type_t& front( void ) const noexcept {
        return notes.front( );
    }

    type_t& back( void ) noexcept {
        return notes.back( );
    }

    const type_t& back( void ) const noexcept {
        return notes.back( );
    }

    inline size_t size( void ) const {
        return notes.size( );
    }

    inline bool empty( void ) const {
        return notes.empty( );
    }

    inline iterator begin( void ) {
        return notes.begin( );
    }

    inline iterator end( void ) {
        return notes.end( );
    }

    inline const const_iterator begin( void ) const {
        return notes.begin( );
    }

    inline const_iterator end( void ) const {
        return notes.end( );
    }

    inline const_iterator cbegin( void ) const {
        return notes.cbegin( );
    }

    inline const_iterator cend( void ) const {
        return notes.cend( );
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
// midi stuff :: events :: enumerations
//---------------------------------------------------------------------------------------------------------------------------

enum event_type_enum {
    k_note_on      = 0,
    k_note_off     = 1,
    k_value_change = 2
};


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: events :: event types
//---------------------------------------------------------------------------------------------------------------------------

namespace processors { // FWD decl
struct parameter_port;
}

struct note_on_event_t      { typedef note_on_event_t      type; static constexpr event_type_enum k_event_type = k_note_on;      };
struct note_off_event_t     { typedef note_off_event_t     type; static constexpr event_type_enum k_event_type = k_note_off;     };

struct value_change_event_t {
    using type    = value_change_event_t; 
    using value_t = std::variant<
        note_t,
        velocity_t,
        uint16_t,
        int16_t,
        uint32_t,
        int32_t,
        float_t
    >;

    static constexpr event_type_enum k_event_type = k_value_change;

    template < typename T >
    value_change_event_t( T _value ) : value( static_cast< T >( _value ) ) { }

    value_change_event_t( const value_change_event_t& other ) : value( other.value ) { }

    value_t value;
 };

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: events :: base event wrapper
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
// midi stuff :: events :: midi note event definition
//---------------------------------------------------------------------------------------------------------------------------

typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_on_event_t,  midi_channel_note > channel_note_on;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_off_event_t, midi_channel_note > channel_note_off;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_on_event_t,  midi_note >         note_on;
typedef midi_stuff::events::midi_note_event_base< midi_stuff::events::note_off_event_t, midi_note >         note_off;
// TODO: implement all_notes_off


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: events :: value_change
//---------------------------------------------------------------------------------------------------------------------------

struct value_change : public value_change_event_t {
    using event_type = value_change_event_t;
    using base_type  = value_change_event_t;
    using type       = value_change;

    static constexpr event_type_enum k_event_type = event_type::k_event_type;

    value_change( void ) :
        value_change_event_t( static_cast< uint32_t >( 0 ) ) { }

    value_change( const value_change_event_t& other ) :
        value_change_event_t( other ) { }
};

} // namespace events


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: processors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace processors {

using dispatcher        = tinyevents::Dispatcher;
using dispatcher_handle = dispatcher::ListenerHandle;

enum parameter_type {
    k_midi_note,
    k_midi_channel_note,
    k_value,
    k_invalid,
};

class base_processor; // FWD decl

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: parameter port structure (both for input and output)
//---------------------------------------------------------------------------------------------------------------------------

struct parameter_port {
    using extra_t = std::variant< void*, int32_t > ;

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
// midi stuff :: processors :: base_processor
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
                  std::string     source_parameter,
                  base_processor* destination_processor,
                  std::string     destination_parameter ) :
                  from( source_processor,      source_parameter      ),
                  to(   destination_processor, destination_parameter ) {
            }
            parameter from;
            parameter to;
        };

        binder( void ) { }

        binder( bond to_bind ) {
            auto* output_port = to_bind.from.processor->get_output( to_bind.from.name );
            auto* input_port =    to_bind.to.processor->get_input( to_bind.to.name    );

            // TODO: improve errors telling I/O
            jassert( nullptr != output_port             );
            jassert( nullptr != output_port->dispatcher );
            jassert( nullptr != input_port              );

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

        output_ports.reserve(      outputs.size( ) );
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
            case k_value:
                from.dispatcher->listen< events::value_change >(
                    [ & ] ( const events::value_change& event ) {
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

    virtual void process_event( const events::note_on&,          parameter_port&, parameter_port& ) { };
    virtual void process_event( const events::note_off&,         parameter_port&, parameter_port& ) { };
    virtual void process_event( const events::channel_note_on&,  parameter_port&, parameter_port& ) { };
    virtual void process_event( const events::channel_note_off&, parameter_port&, parameter_port& ) { };
    virtual void process_event( const events::value_change&,     parameter_port&, parameter_port& ) { };

    template < class T, class U, class V >
    bool enqueue_event( parameter_port& output_port, events::event_type_enum event_type, T channel, U note, V velocity ) {
        jassert( nullptr                                                         != output_port.dispatcher    );
        jassert( named_output_ports.find( output_port.name )                     != named_output_ports.end( ) );
        jassert( named_output_ports.find( output_port.name )->second->dispatcher == output_port.dispatcher    );

        if ( events::k_note_on == event_type ) {
            enqueue_event(
                output_port,
                events::channel_note_on(
                    static_cast< int8_t  >( channel ),
                    static_cast< int8_t  >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;

        } else if ( events::k_note_off == event_type ) {
            enqueue_event(
                output_port,
                events::channel_note_off(
                    static_cast< int8_t  >( channel ),
                    static_cast< int8_t  >( note ),
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

        if constexpr (
            std::is_same_v< T, events::note_on          > ||
            std::is_same_v< T, events::note_off         > ||
            std::is_same_v< T, events::channel_note_on  > ||
            std::is_same_v< T, events::channel_note_off > ||
            std::is_same_v< T, events::value_change     >
        ) {
            return true;
        }

        // unreachable - and it should be this way
        // return false;
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
// midi stuff :: processors :: midi note input 
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
// midi stuff :: processors :: midi channel split
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
// midi stuff :: processors :: midi channel filter
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_filter : public base_processor {
public:
    std::vector< parameter_port* >  channel_output_ports;
    std::array< bool, 16 >          channel_mute;
    bool                            mute_all;

    static constexpr int32_t        k_last_channel = 16;
    static constexpr int32_t        k_channel_mute = 17;
    static constexpr int32_t        k_mute_all     = 18;

public:
    midi_channel_filter( void ) :
        base_processor(
            {
                { "channel_1",     k_midi_note,  1             },
                { "channel_2",     k_midi_note,  2             },
                { "channel_3",     k_midi_note,  3             },
                { "channel_4",     k_midi_note,  4             },
                { "channel_5",     k_midi_note,  5             },
                { "channel_6",     k_midi_note,  6             },
                { "channel_7",     k_midi_note,  7             },
                { "channel_8",     k_midi_note,  8             },
                { "channel_9",     k_midi_note,  9             },
                { "channel_10",    k_midi_note, 10             },
                { "channel_11",    k_midi_note, 11             },
                { "channel_12",    k_midi_note, 12             },
                { "channel_13",    k_midi_note, 13             },
                { "channel_14",    k_midi_note, 14             },
                { "channel_15",    k_midi_note, 15             },
                { "channel_16",    k_midi_note, 16             },
                { "channel_mute",  k_value,     k_channel_mute },
                { "mute_all",      k_value,     k_mute_all     },
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
        mute_all( false ) {
            channel_mute.fill( false ); 
        }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( std::get< int32_t >( to.extra ) > 0 && std::get< int32_t >( to.extra ) <= 16 );

        if ( mute_all || channel_mute[ std::get< int32_t >( to.extra ) - 1 ] ) {
            return;
        }

        process_note_event_tmpl( event, to );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( std::get< int32_t >( to.extra ) > 0 && std::get< int32_t >( to.extra ) <= 16 );
        process_note_event_tmpl( event, to );
    };

    virtual void process_event( const events::value_change& event, parameter_port&, parameter_port& to ) {
        if ( k_channel_mute == std::get< int32_t >( to.extra ) ) {
            jassert( std::get< int32_t >( event.value ) <= k_last_channel );
            channel_mute[ std::get< int32_t >( event.value ) ] = !channel_mute[ std::get< int32_t >( event.value ) ];
        } else if ( k_mute_all == std::get< int32_t >( to.extra ) ) {
            mute_all = !mute_all;
        }
    };

private:
    template < typename T >
    inline void process_note_event_tmpl( const T& event, parameter_port& to ) {
        enqueue_event( *channel_output_ports[ std::get< int32_t >( to.extra ) - 1 ], event );
    };
};


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi channel join
//---------------------------------------------------------------------------------------------------------------------------

class midi_channel_join : public base_processor {
public:
    parameter_port&        channel_output;

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
        channel_output( *get_output( "channel_notes" ) )
        {
        }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_note_event_tmpl( event, to );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        process_note_event_tmpl( event, to );
    };


private:
    template < typename T >
    inline void process_note_event_tmpl( const T& event, parameter_port& to ) {
        jassert( std::get< int32_t >( to.extra ) > 0 && std::get< int32_t >( to.extra ) <= 16 );

        enqueue_event( channel_output, event.k_event_type, std::get< int32_t >( to.extra ), event.note, event.velocity );
    };
};

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi note dedup
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
// midi stuff :: processors :: midi sorted note store
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
// midi stuff :: processors :: midi lowest and highest notes
//---------------------------------------------------------------------------------------------------------------------------

template < typename container_t >
class midi_first_n_notes_of_container : public midi_sorted_note_store< container_t > {
public:
    typedef container_t                                  container;
    typedef midi_sorted_note_store< container >          base_type;
    typedef midi_first_n_notes_of_container< base_type > type;
    typedef base_type::parameter_initializer_list        parameter_initializer_list;

    static constexpr int32_t k_note_input         = 0;
    static constexpr int32_t k_number_notes_input = 1;

    static constexpr int32_t k_note_output        = 0;

public:
    container       active_notes;
    uint32_t        number_notes_kept;

    parameter_port& output_port;

public:
    midi_first_n_notes_of_container( uint8_t the_number_notes_kept = 128 ) :
        midi_sorted_note_store< container_t >(
            {
                { "notes",        k_midi_note, k_note_input         },
                { "number_notes", k_value,     k_number_notes_input }
            },
            {
                { "notes",        k_midi_note, k_note_output        }
            }
        ),
        active_notes( ),
        number_notes_kept( the_number_notes_kept ),
        output_port( *this->get_output( "notes" ) ) {
    }

    virtual void process_event( const events::value_change& event, parameter_port&, parameter_port& to ) override {
        if ( std::get< int32_t >( to.extra ) == k_number_notes_input ) {
            number_notes_kept = std::get< uint32_t >( event.value );
        }
    };

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
// midi stuff :: processors :: midi note arpeggiator
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_arpeggiator : public midi_sorted_note_store< containers::ascending_midi_note_list > {
public:
    using container                     = containers::ascending_midi_note_list;
    using container_type                = container::container;
    using base_type                     = midi_sorted_note_store< container >;
    using parameter_initializer_list    = base_type::parameter_initializer_list;

    template < class... Ts >
    struct overloaded : Ts... { using Ts::operator( )...; };

public:
    static constexpr int32_t        k_chord_notes_input    = 0;
    static constexpr int32_t        k_sequence_notes_input = 1;

    static constexpr int32_t        k_notes_output         = 0;

    parameter_port&                 output_port; // this->notes is treated as output as it has a generic name
    container                       chord_notes;
    container                       sequence_notes;

    // TODO: param_work, add shift for input and output

    bool                            im_a_dirty_object;


public:
    midi_note_arpeggiator( ) :
        base_type(
            {
                { "chord_notes",    k_midi_note, k_chord_notes_input    },
                { "sequence_notes", k_midi_note, k_sequence_notes_input }
            },
            {
                { "notes",          k_midi_note, k_notes_output         }
            }
        ),
        chord_notes( ),
        sequence_notes( ),
        im_a_dirty_object( false ),
        output_port( *this->get_output( "notes" ) ) { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );
        switch ( std::get< int32_t >( to.extra ) ) {
            case k_chord_notes_input:    if ( chord_notes.insert(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes_input: if ( sequence_notes.insert( event ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );

        switch ( std::get< int32_t >( to.extra ) ) {
            case k_chord_notes_input:    if ( chord_notes.erase(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes_input: if ( sequence_notes.erase( event ) ) { im_a_dirty_object = true; } break;
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
            utils::symmetric_difference_apply( active_notes, notes,
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
// midi stuff :: processors :: chord intervals
//---------------------------------------------------------------------------------------------------------------------------


// TODO: step 2: abstract this in a way that it can have multiple different "decision engines" for the chord picking

class midi_chord_intervals : public base_processor {
public:
    using container                     = containers::ascending_midi_note_list;
    using container_type                = container::container;
    using named_noteset                 = chord_data::named_noteset;
    using scale_code_t                  = named_noteset::scale_code_t;

    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };

public:
    static constexpr int32_t        k_root_note_input      = 0;
    static constexpr int32_t        k_chord_notes_input    = 1;
    static constexpr int32_t        k_pattern_notes_input  = 2;

    static constexpr int32_t        k_chord_notes_output     = 0;
    static constexpr int32_t        k_scale_code_output      = 1;
    static constexpr int32_t        k_pattern_degrees_output = 2;

    std::optional< midi_note >      root_note;
    container                       chord_notes;
    container                       pattern_notes;

    // TODO: step 1: midi_notes_to_intervals, according to a tertiary input propose : { pattern_notes + chord_notes } -> { pattern_intervals_output_port, pattern_intervals_output } )
    // attention: consider accidents, it will solidify the interface for the abstract engine - first round the note to the map, and progress to finding an appropriate scale in the scale map
    // 

    parameter_port&                 chord_notes_output_port;
    container                       chord_notes_output;

    parameter_port&                 scale_code_output_port;
    events::value_change            scale_code_output;
    scale_code_t                    scale_code;

    parameter_port&                 pattern_degrees_output_port;
    container                       pattern_degrees_output;

    const named_noteset*            chosen_chord;

    bool                            im_a_dirty_object; // when to run process

public:
    midi_chord_intervals( void ) :
        base_processor(
            {
                { "root_note",       k_midi_note, k_root_note_input        },
                { "chord_notes",     k_midi_note, k_chord_notes_input      },
                { "pattern_notes",   k_midi_note, k_pattern_notes_input    }
            },
            {
                { "chord_notes",     k_midi_note, k_chord_notes_output     },
                { "scale_code",      k_value,     k_scale_code_output      },
                { "pattern_degrees", k_midi_note, k_pattern_degrees_output }
            }
        ),
        root_note( std::nullopt ),
        chord_notes( ),
        pattern_notes( ),
        chord_notes_output_port( *this->get_output( "chord_notes" ) ),
        chord_notes_output( ),
        scale_code_output_port( *this->get_output( "scale_code" ) ),
        scale_code_output( ),
        pattern_degrees_output_port( *this->get_output( "pattern_degrees" ) ),
        pattern_degrees_output( ),
        chosen_chord( 0 ),
        im_a_dirty_object( false ) {
        scale_code_output.value = static_cast< uint16_t >( 0 );
    }


    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        note_t abs_note = static_cast< note_t >( event.note % 12 );

        switch ( std::get< int32_t >( to.extra ) ) {
            case k_root_note_input:        {           root_note = static_cast< note_t >( abs_note );      im_a_dirty_object = true; } break;
            case k_chord_notes_input:   if (   chord_notes.insert( static_cast< note_t >( abs_note ) ) ) { im_a_dirty_object = true; } break;
            case k_pattern_notes_input: if ( pattern_notes.insert(                        event      ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    }

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        note_t abs_note = static_cast< note_t >( event.note % 12 );

        switch ( std::get< int32_t >( to.extra ) ) {
            case k_root_note_input:     if ( root_note && root_note.value( ).note == abs_note   ) { root_note = std::nullopt; im_a_dirty_object = true; } break;
            case k_chord_notes_input:   if ( chord_notes.erase(                      abs_note ) ) {                           im_a_dirty_object = true; } break;
            case k_pattern_notes_input: if ( pattern_notes.erase(                    event    ) ) {                           im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    }

    void all_notes_off( void ) {
        // enqueue offs for whatever is on the output port
        std::for_each(
            chord_notes_output.begin( ),
            chord_notes_output.end( ),
            [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                enqueue_event( chord_notes_output_port, events::note_off( note ) );
            }
        );
        chord_notes_output.clear( );

        std::for_each(
            pattern_degrees_output.begin( ),
            pattern_degrees_output.end( ),
            [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                enqueue_event( pattern_degrees_output_port, events::note_off( note ) );
            }
        );
        pattern_degrees_output.clear( );

        scale_code.reset( );
        scale_code_output.value = static_cast< uint16_t >( 0 );
        enqueue_event( scale_code_output_port, scale_code_output );
    }

    virtual void process( void ) override {
        // Desired output state:
        // chord_notes     to contain all chord notes from 0 - 11 (even the predicted)
        // scale_code      to the chord scale

        if ( !im_a_dirty_object ) return;

        // short circuit: if no complete input, no output.
        if ( chord_notes.empty( ) ) {
            // clears chord_notes_output and pattern_degrees_output,
            // calls scale_code.reset( ) and propagate the events
            all_notes_off( );
            im_a_dirty_object = false; // mwah. took the short(cir)cut.
            return;
        } // end short circuit

        // root shift is root note. all other incoming notes are shifted by root_shift. 0 <= root_shift < 12
        note_t root_shift = 0;
        if ( root_note ) { // if not provided in channel 1, use lowest
            root_shift = root_note.value( ).note;
        } else {
            root_shift = chord_notes.front( ).note;
        }

        // refresh internal scale_code and check if the notes changed the output
        named_noteset::set_scale_code( chord_notes, scale_code, root_shift );
        chosen_chord = &( chord_data::get_best_fitting_named_noteset( scale_code ));

        // ------- scale_code output ------- //
        // if the execution reached this point, it means that the output is going to change
        // dispatch the scale code value change
        scale_code              = chosen_chord->scale_code;
        scale_code_output.value = static_cast< uint16_t >( scale_code.to_ulong( ) );
        enqueue_event( scale_code_output_port, scale_code_output );

        // ------- chord_notes output ------- //
        // and dispatch the note on/off events required to make the output to be equal to the new scale
        container active_notes;
        named_noteset::set_notes_from_scale_code( scale_code, active_notes, root_shift );
        active_notes.insert( root_shift );

        // according to previous publication of note events, the new notes can be prescribed by the mutual exclution of sets A and B </blablabla>
        utils::symmetric_difference_apply( active_notes, chord_notes_output,
            // in active_notes (freshly computed) but not in chord_notes_output - note on
            [ & ] ( const midi_note& note ) {
                enqueue_event( chord_notes_output_port, events::note_on( note ) );
            },
            // in chord_notes_output (output) but not in active_notes (freshly computed) - note off
            [ & ] ( const midi_note& note ) {
                enqueue_event( chord_notes_output_port, events::note_off( note ) );
            }
        );

        // new notes are now part of music history
        chord_notes_output.notes = active_notes.notes;

        // ------- pattern_degrees output ------- //
        active_notes.clear( );

        std::for_each(
            pattern_notes.begin( ),
            pattern_notes.end( ),
            [ & ] ( const midi_note& note ) {
                note_t  abs_note    = ( ( note.note + 12 ) - root_shift ) % 12;
                uint8_t note_degree = 0;
            
                // linear lower bound search for the abs_note in chord_notes_output
                if (
                    std::find_if(
                        chord_notes_output.begin( ),
                        chord_notes_output.end( ),
                        [ & ] ( const midi_note& chord_note ) {
                            ++note_degree;
                            // lower bound degree
                            return abs_note <= chord_note.note;
                        }
                    ) == chord_notes_output.end( ) ) {
                    // when not found, it means that the pattern note is an accident upper bound
                    // other accidents are going to be auto lower bound
                    // possible experiments:
                    // - use a scale?
                    // - accidents_output?
                    note_degree = 0;
                }

                note_t new_note = static_cast< note_t >( note_degree + ( ( ( note.note + 12 ) - root_shift ) / 12 - 1 ) * chord_notes_output.size( ) );
                active_notes.insert( new_note );
            }
        );

        // once again, loudly blaber the words cibtained in the ancient scrolls of union theory, 
        // commanding events for mutually exclusive existing notes
        utils::symmetric_difference_apply( active_notes, pattern_degrees_output,
            // in active_notes (freshly computed) but not in chord_notes_output - note on
            [ & ] ( const midi_note& note ) {
                enqueue_event( pattern_degrees_output_port, events::note_on( note ) );
            },
            // in chord_notes_output (output) but not in active_notes (freshly computed) - note off
            [ & ] ( const midi_note& note ) {
                enqueue_event( pattern_degrees_output_port, events::note_off( note ) );
            }
        );

        im_a_dirty_object = false; // yeessss.... yes... excellent...
    }
};


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi note output
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
        output_midi_buffer( nullptr ) {
    }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
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