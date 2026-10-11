#pragma once

#ifdef MIDI_STUFF_WITH_JUCE
    #include <JuceHeader.h>
#endif

#ifndef jassert
    #include <cassert>
    #define jassert( expression ) assert( expression )
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <atomic>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <list>
#include <optional>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace midi_stuff {

using note_t     = uint8_t;
using velocity_t = uint8_t;
using channel_t  = int8_t;

// FWD Decl
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

void while_condition_do_loop( auto condition, auto loop ) {
    while ( condition( ) ) {
        loop( );
    }
}

void do_loop_while_condition( auto loop, auto condition ) {
    while ( true ) {
        loop( );
        if ( !condition( ) ) return;
    }
}

} // namespace utils

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: threads
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace threads {

// many readers or one writer. backed by std::shared_mutex: it blocks, so keep the critical sections short.
// TODO: the audio thread shouldn't block at all - consider a lock-free double buffer / seqlock
struct read_write_lock {
    std::shared_mutex   mutex;

    struct read_guard {
        read_write_lock& lock;

        read_guard( void ) = delete;
        read_guard( read_write_lock& _lock ) : lock( _lock ) { lock.read_start( ); }
        ~read_guard( void )                                  { lock.read_end(   ); }
    };

    struct write_guard {
        read_write_lock& lock;

        write_guard( void ) = delete;
        write_guard( read_write_lock& _lock ) : lock( _lock ) { lock.write_start( ); }
        ~write_guard( void )                                  { lock.write_end(   ); }
    };

    void read_start(  void ) { mutex.lock_shared(   ); }
    void read_end(    void ) { mutex.unlock_shared( ); }
    void write_start( void ) { mutex.lock(          ); }
    void write_end(   void ) { mutex.unlock(        ); }
};

template < typename object_t >
struct concurrent_object {
private:
    read_write_lock     lock;
    object_t            object;
    std::atomic_uint8_t revision;

public:
    void read_visit( auto func ) {
        read_write_lock::read_guard guard( lock );
        func( object );
    }

    void write_visit( auto func ) {
        read_write_lock::write_guard guard( lock );
        func( object );
        ++revision;
    }

    uint8_t get_revision( void ) { return revision; }

};

template < typename object_t >
struct buffered_object {
public:
    using object_type             = object_t;
    using concurrent_object_type  = midi_stuff::threads::concurrent_object< object_type >;

public:
    object_type buffer;

protected:
    uint8_t     revision;

public:
    buffered_object( void ) : revision( std::numeric_limits< uint8_t >::max( ) ) { }

    void flush( void ) {
        get_concurrent_object( ).write_visit(
            [ & ] ( object_type& write_locked_object ) {
                write_locked_object = buffer;
            }
        );
        sync_revision( );
    }

    bool read( void ) {
        concurrent_object_type& concurrent_object( get_concurrent_object( ) );
        const uint8_t concurrent_object_revision = concurrent_object.get_revision( );

        if ( concurrent_object_revision == revision ) {
            return false;
        }

        concurrent_object.read_visit(
            [ & ] ( object_type& write_locked_object ) {
                buffer = write_locked_object;
            }
        );

        sync_revision( );

        return true;
    }

    void sync_revision( void ) {
        revision = get_concurrent_object( ).get_revision( );
    }

protected:
    virtual concurrent_object_type& get_concurrent_object( void ) = 0;
};

template < typename object_t, uint32_t _uuid >
struct buffered_object_instance : buffered_object< object_t > {
public:
    using            object_type                         = object_t;
    using            parent_type                         = buffered_object< object_type >;
    using            concurrent_object_type              = parent_type::concurrent_object_type;

    static constexpr uint32_t uuid                       = _uuid;

protected:
    virtual concurrent_object_type& get_concurrent_object( void ) override { return concurrent_object; }

private:
    inline static concurrent_object_type concurrent_object; // one per (object_t, uuid), shared by every instance in the process
};

} // namespace threads

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
        return static_cast< uint16_t >( ( note << 8 ) | velocity );
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
    
    midi_note( const midi_note& ) = default;
    midi_note& operator=( const midi_note& ) = default;

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

    midi_channel_note( const midi_channel_note& ) = default;
    midi_channel_note& operator=( const midi_channel_note& ) = default;
    
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
    using type         = _named_note_set< note_set_t >;

    using scale_code_t = std::bitset< 16 >;

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
struct midi_stuff_chord_guesser {
    static const named_noteset& get_best_fitting_named_noteset( const named_noteset::scale_code_t& input_scale_code ) {
        const named_noteset* best_fitting_named_noteset = &chord_descriptors.back( ); // set it to guard, always returning a valid hypothesis
        int32_t              max_score                  = std::numeric_limits< int32_t >::min( );

        std::find_if(
            chord_descriptors.begin( ),
            chord_descriptors.end( ),
            [ & ]( const named_noteset& hypothesis_chord ) {
                // short circuit on a perfect match
                if ( input_scale_code == hypothesis_chord.scale_code ) {
                    best_fitting_named_noteset = &hypothesis_chord;
                    return true;
                }

                int32_t distance_score = static_cast< int32_t >( ( input_scale_code & ~hypothesis_chord.scale_code ).count( ) ); // Unmatching input scale notes
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
    static const named_noteset& get_best_fitting_named_noteset( const T& iterable_note_container, const note_t root_shift ) {
        named_noteset::scale_code_t input_scale_code;
        named_noteset::set_scale_code( iterable_note_container, input_scale_code, root_shift );

        return midi_stuff_chord_guesser::get_best_fitting_named_noteset( input_scale_code );
    }
};

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
// midi stuff :: containers :: note set
//
// the notes ( 0 - 127 ) that are on, with their velocities. fixed size, trivially copyable and never allocates:
// insert / erase / contains are O( 1 ), iteration is a bit scan ( ascending, or descending through descending( ) ),
// and diff( ) - the note offs and note ons that turn one set into another - is a couple of masks.
//
// velocities of absent notes are kept at 0, so two sets compare equal when they hold the same notes and velocities.
//---------------------------------------------------------------------------------------------------------------------------

class note_set {
public:
    static constexpr size_t k_capacity  = 128;
    static constexpr int    k_end       = -1;

private:
    using word_t = uint64_t;

    static constexpr size_t k_word_bits = 64;
    static constexpr size_t k_words     = k_capacity / k_word_bits;

    std::array< word_t,     k_words    > words{ };
    std::array< velocity_t, k_capacity > velocities{ };

public:
    class const_iterator {
    public:
        using iterator_category = std::input_iterator_tag; // dereferences to a value, not a reference
        using value_type        = midi_note;
        using difference_type   = std::ptrdiff_t;
        using pointer           = void;
        using reference         = midi_note;

        const_iterator( void ) = default;
        const_iterator( const note_set* the_set, int the_position, bool is_ascending ) :
            set( the_set ), position( the_position ), ascending( is_ascending ) { }

        midi_note operator * ( void ) const { return set->at( static_cast< note_t >( position ) ); }

        const_iterator& operator ++ ( void ) {
            position = ascending ? set->first_from( position + 1 ) : set->last_from( position - 1 );
            return *this;
        }

        const_iterator operator ++ ( int ) { const_iterator previous = *this; ++( *this ); return previous; }

        bool operator == ( const const_iterator& other ) const { return position == other.position; }
        bool operator != ( const const_iterator& other ) const { return position != other.position; }

    private:
        const note_set* set       = nullptr;
        int             position  = k_end;
        bool            ascending = true;
    };

    struct descending_view {
        const note_set& set;

        const_iterator begin( void ) const { return const_iterator( &set, set.last_from( k_capacity - 1 ), false ); }
        const_iterator end(   void ) const { return const_iterator( &set, k_end,                           false ); }
    };

    // returns false if the note is already there ( its velocity is kept ) or out of range
    bool insert( const midi_note& note ) {
        if ( note.note >= k_capacity ) {
            jassert( false ); // a note above 127 is a bug upstream; release builds drop it
            return false;
        }

        if ( contains( note.note ) ) {
            return false;
        }

        word_of( note.note ) |= bit_of( note.note );
        velocities[ note.note ] = note.velocity;
        return true;
    }

    // returns false if the note wasn't there
    bool erase( const midi_note& note ) {
        if ( !contains( note.note ) ) {
            return false;
        }

        word_of( note.note ) &= ~bit_of( note.note );
        velocities[ note.note ] = 0;
        return true;
    }

    bool contains( note_t note ) const {
        return note < k_capacity && ( word_of( note ) & bit_of( note ) ) != 0;
    }

    velocity_t velocity( note_t note ) const {
        return contains( note ) ? velocities[ note ] : 0;
    }

    midi_note at( note_t note ) const {
        return midi_note( static_cast< int8_t >( note ), velocity( note ) );
    }

    void clear( void ) {
        words.fill( 0 );
        velocities.fill( 0 );
    }

    size_t size( void ) const {
        size_t count = 0;
        for ( word_t word : words ) count += static_cast< size_t >( std::popcount( word ) );
        return count;
    }

    bool empty( void ) const {
        return std::all_of( words.begin( ), words.end( ), [ ] ( word_t word ) { return word == 0; } );
    }

    // lowest / highest note - the set must not be empty
    midi_note front( void ) const { jassert( !empty( ) ); return at( static_cast< note_t >( first_from( 0 ) ) );              }
    midi_note back(  void ) const { jassert( !empty( ) ); return at( static_cast< note_t >( last_from( k_capacity - 1 ) ) ); }

    const_iterator  begin(      void ) const { return const_iterator( this, first_from( 0 ), true ); }
    const_iterator  end(        void ) const { return const_iterator( this, k_end,           true ); }
    descending_view descending( void ) const { return descending_view{ *this }; }

    bool operator == ( const note_set& other ) const = default;

    // lowest note >= from, or k_end
    int first_from( int from ) const {
        if ( from < 0 ) from = 0;

        for ( size_t word_index = static_cast< size_t >( from ) / k_word_bits; word_index < k_words; ++word_index ) {
            word_t bits = words[ word_index ];

            if ( word_index == static_cast< size_t >( from ) / k_word_bits ) {
                bits &= ~word_t( 0 ) << ( static_cast< size_t >( from ) % k_word_bits ); // drop the notes below `from`
            }

            if ( bits ) {
                return static_cast< int >( word_index * k_word_bits + static_cast< size_t >( std::countr_zero( bits ) ) );
            }
        }

        return k_end;
    }

    // highest note <= from, or k_end
    int last_from( int from ) const {
        if ( from < 0 ) return k_end; // past the lowest note ( -1 / 64 would truncate to word 0 )
        if ( from >= static_cast< int >( k_capacity ) ) from = k_capacity - 1;

        for ( int word_index = from / static_cast< int >( k_word_bits ); word_index >= 0; --word_index ) {
            word_t bits = words[ static_cast< size_t >( word_index ) ];

            if ( word_index == from / static_cast< int >( k_word_bits ) ) {
                const size_t keep = static_cast< size_t >( from ) % k_word_bits + 1; // drop the notes above `from`
                if ( keep < k_word_bits ) bits &= ( word_t( 1 ) << keep ) - 1;
            }

            if ( bits ) {
                return word_index * static_cast< int >( k_word_bits ) + static_cast< int >( k_word_bits - 1 ) - std::countl_zero( bits );
            }
        }

        return k_end;
    }

    // what turns `from` into `to`: on_removed( note ) for each note only in `from` ( with its `from` velocity ),
    // then on_added( note ) for each note only in `to` ( with its `to` velocity ). note offs first, both ascending.
    template < typename on_removed_t, typename on_added_t >
    static void diff( const note_set& from, const note_set& to, on_removed_t&& on_removed, on_added_t&& on_added ) {
        for ( size_t word_index = 0; word_index < k_words; ++word_index ) {
            for_each_bit( from.words[ word_index ] & ~to.words[ word_index ], word_index, [ & ] ( note_t note ) { on_removed( from.at( note ) ); } );
        }

        for ( size_t word_index = 0; word_index < k_words; ++word_index ) {
            for_each_bit( to.words[ word_index ] & ~from.words[ word_index ], word_index, [ & ] ( note_t note ) { on_added( to.at( note ) ); } );
        }
    }

private:
          word_t& word_of( note_t note )       { return words[ note / k_word_bits ]; }
    const word_t& word_of( note_t note ) const { return words[ note / k_word_bits ]; }

    static word_t bit_of( note_t note ) { return word_t( 1 ) << ( note % k_word_bits ); }

    template < typename callback_t >
    static void for_each_bit( word_t bits, size_t word_index, callback_t&& callback ) {
        while ( bits ) {
            callback( static_cast< note_t >( word_index * k_word_bits + static_cast< size_t >( std::countr_zero( bits ) ) ) );
            bits &= bits - 1; // Kernighan: clear the lowest set bit
        }
    }
};

static_assert( std::is_trivially_copyable_v< note_set >, "note_set must stay trivially copyable ( shared across threads, Teensy )" );

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

// a value is an integer ( notes, velocities, channels, counts, scale codes... ) or a real number. it is set from any
// arithmetic type and read back as any arithmetic type - get< T >( ) converts from whatever is stored and never throws
struct value_change_event_t {
    using type    = value_change_event_t;
    using value_t = std::variant< int32_t, float >;

    static constexpr event_type_enum k_event_type = k_value_change;

    template < typename T >
    value_change_event_t( T _value ) { set( _value ); }

    template < typename T >
    void set( T _value ) {
        static_assert( std::is_arithmetic_v< T >, "a value is a number" );

        if constexpr ( std::is_floating_point_v< T > ) {
            value = static_cast< float   >( _value );
        } else {
            value = static_cast< int32_t >( _value );
        }
    }

    template < typename T >
    T get( void ) const {
        static_assert( std::is_arithmetic_v< T >, "a value is a number" );

        if ( const int32_t* integer = std::get_if< int32_t >( &value ) ) {
            return static_cast< T >( *integer );
        }

        return static_cast< T >( *std::get_if< float >( &value ) );
    }

    bool is_integer( void ) const { return std::holds_alternative< int32_t >( value ); }

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
        value_change_event_t( static_cast< int32_t >( 0 ) ) { }

    value_change( const value_change_event_t& other ) :
        value_change_event_t( other ) { }
};

} // namespace events


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// midi stuff :: processors
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace processors {

enum parameter_type {
    k_midi_note,
    k_midi_channel_note,
    k_integer_value,
    k_real_value,
    k_invalid,
};

// which port type carries an event type - emitting an event on a port of another type is a bug
template < typename event_t >
constexpr parameter_type port_type_of( void ) {
    if constexpr ( std::is_same_v< event_t, events::note_on > || std::is_same_v< event_t, events::note_off > ) {
        return k_midi_note;
    } else if constexpr ( std::is_same_v< event_t, events::channel_note_on > || std::is_same_v< event_t, events::channel_note_off > ) {
        return k_midi_channel_note;
    } else if constexpr ( std::is_same_v< event_t, events::value_change > ) {
        return k_integer_value; // or k_real_value: value ports are checked against the value itself, see emit( )
    } else {
        static_assert( sizeof( event_t ) == 0, "not an event type" );
    }
}

class base_processor; // FWD decl

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: parameter port structure (both for input and output)
//---------------------------------------------------------------------------------------------------------------------------

struct parameter_port {
    using extra_t = int32_t; // per-port tag the owner chooses ( channel number, input id... )

    parameter_port( base_processor* the_owner, const std::string& the_name, parameter_type the_type, extra_t the_extra = 0 ) :
        owner( the_owner ),
        provider_port( nullptr ),
        name( the_name ),
        type( the_type ),
        extra( the_extra ) {
    }

    parameter_port( base_processor* the_owner, const char* the_name, parameter_type the_type, extra_t the_extra = 0 ) :
        parameter_port( the_owner, std::string( the_name ), the_type, the_extra ) {
    }

    bool operator  < ( const parameter_port& rhs ) { return name  < rhs.name; }
    bool operator == ( const parameter_port& rhs ) { return name == rhs.name; }

    base_processor*                 owner;
    parameter_port*                 provider_port; // inputs: the output port feeding it
    std::vector< parameter_port* >  consumers;     // outputs: the input ports it feeds - only grows while binding

    const std::string               name;
    parameter_type                  type;

    extra_t                         extra;
};

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: base_processor
//
// events are delivered immediately: emit( ) calls process_event( ) on every bound consumer before returning. so:
// - process_event( ) may emit ( that is how forwarders - split, filter, join, dedup - work ), the event travels the chain
// - the graph has no cycles ( checked when binding )
// - process_event( ) never calls process( ): it updates state, or forwards
//
// backtrack_process( ) pulls: every provider runs its process( ) - once per pass - before its consumers.
//---------------------------------------------------------------------------------------------------------------------------

class base_processor {
protected:
    std::vector< parameter_port >                               input_ports;
    std::unordered_map< std::string, parameter_port* >          named_input_ports;

    std::vector< parameter_port >                               output_ports;
    std::unordered_map< std::string, parameter_port* >          named_output_ports;

private:
    // identifies a pull: which processor started it, and its count. a processor runs once per pass
    struct pass_token {
        const base_processor* root  = nullptr;
        uint32_t              count = 0;

        bool operator == ( const pass_token& ) const = default;
    };

    pass_token                                                  last_pass;
    uint32_t                                                    pass_count = 0; // passes started by this processor

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
            jassert( nullptr != output_port );
            jassert( nullptr != input_port  );

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
    // ports keep `this` and point into the port vectors: copying or moving would leave them dangling
    base_processor(             const base_processor& ) = delete;
    base_processor(                   base_processor&& ) = delete;
    base_processor& operator = ( const base_processor& ) = delete;
    base_processor& operator = (       base_processor&& ) = delete;

    virtual ~base_processor( void ) = default;

    base_processor( parameter_initializer_list inputs,
                    parameter_initializer_list outputs ) {

        input_ports.reserve( inputs.size( ) );
        for ( auto port_decl : inputs ) {
            auto [ name, type, extra ] = port_decl;
            input_ports.push_back( parameter_port( this, name, type, extra ) );
            auto& port_added = input_ports[ input_ports.size( ) - 1 ];
            named_input_ports.insert( std::pair< std::string, parameter_port* >( port_added.name, &port_added ) );
        }

        output_ports.reserve( outputs.size( ) );
        for ( auto port_decl : outputs ) {
            auto [ name, type, extra ] = port_decl;
            output_ports.push_back( parameter_port( this, name, type, extra ) );
            auto& port_added = output_ports[ output_ports.size( ) - 1 ];
            named_output_ports.insert( std::pair< std::string, parameter_port* >( port_added.name, &port_added ) );
        }
    }

    void bind_output( parameter_port& from, parameter_port& to ) {
        jassert( from.owner == this );
        jassert( from.type  == to.type );
        jassert( nullptr    == to.provider_port );                              // an input has a single provider
        jassert( to.owner   != this && !depends_on( to.owner ) );                // no cycles

        to.provider_port = &from;
        from.consumers.push_back( &to );
    }

    // true if `processor` is upstream of this one
    bool depends_on( const base_processor* processor ) const {
        for ( const auto& input_port : input_ports ) {
            if ( input_port.provider_port ) {
                const base_processor* provider = input_port.provider_port->owner;
                if ( provider == processor || provider->depends_on( processor ) ) return true;
            }
        }
        return false;
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

    // delivers the event to every input bound to output_port, right now
    template < typename event_t >
    void emit( parameter_port& output_port, const event_t& event ) {
        jassert( output_port.owner == this );

        if constexpr ( std::is_same_v< event_t, events::value_change > ) {
            jassert( output_port.type == ( event.is_integer( ) ? k_integer_value : k_real_value ) );
        } else {
            jassert( output_port.type == port_type_of< event_t >( ) );
        }

        for ( parameter_port* consumer : output_port.consumers ) {
            consumer->owner->process_event( event, output_port, *consumer );
        }
    }

    template < class T, class U, class V >
    bool emit( parameter_port& output_port, events::event_type_enum event_type, T channel, U note, V velocity ) {
        if ( events::k_note_on == event_type ) {
            emit(
                output_port,
                events::channel_note_on(
                    static_cast< int8_t  >( channel ),
                    static_cast< int8_t  >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;

        } else if ( events::k_note_off == event_type ) {
            emit(
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
    bool emit( parameter_port& output_port, events::event_type_enum event_type, U note, V velocity ) {
        if ( events::k_note_on == event_type ) {
            emit(
                output_port,
                events::note_on(
                    static_cast< int8_t  >( note ),
                    static_cast< uint8_t >( velocity )
                )
            );
            return true;

        } else if ( events::k_note_off == event_type ) {
            emit(
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
    bool emit( parameter_port& output_port, const juce::MidiMessage& event ) {
        if ( event.isNoteOn( ) ) {
            emit(
                output_port,
                events::channel_note_on( event.getChannel( ), event.getNoteNumber( ), event.getVelocity( ) )
            );
            return true;

        } else if ( event.isNoteOff( ) ) {
            emit(
                output_port,
                events::channel_note_off( event.getChannel( ), event.getNoteNumber( ), event.getVelocity( ) )
            );
            return true;
        }

        return false;
    };
#endif

    // pulls the graph: runs every provider upstream ( once ), then this processor
    void backtrack_process( void ) {
        backtrack_process( pass_token{ this, ++pass_count } );
    }

    virtual void process( void ) { }

private:
    void backtrack_process( pass_token pass ) {
        if ( last_pass == pass ) {
            return; // already ran in this pass ( a provider feeding several branches )
        }
        last_pass = pass;

        for ( auto& input_port : input_ports ) {
            if ( input_port.provider_port ) {
                input_port.provider_port->owner->backtrack_process( pass );
            }
        }

        process( );
    }
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
        emit( channel_notes, events::k_note_on, channel, note, velocity );
    }

    template < class T, class U, class V >
    void note_off( T channel, U note, V velocity ) {
        emit( channel_notes, events::k_note_off, channel, note, velocity );
    }

#ifdef JUCE_VERSION
    bool generate_events( juce::MidiBuffer& midi_buffer, bool clear = false ) {
        bool needs_processing = false;
        std::for_each(
            midi_buffer.begin( ),
            midi_buffer.end( ),
            [ & ] ( juce::MidiBufferIterator::reference buffer_itr ) {
                needs_processing |= emit( channel_notes, buffer_itr.getMessage( ) );
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
        emit( *channel_output_ports[ event.channel - 1 ], event.k_event_type, event.note, event.velocity );
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
                { "channel_mute",  k_integer_value,     k_channel_mute },
                { "mute_all",      k_integer_value,     k_mute_all     },
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
        jassert( to.extra > 0 && to.extra <= 16 );

        if ( mute_all || channel_mute[ to.extra - 1 ] ) {
            return;
        }

        process_note_event_tmpl( event, to );
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( to.extra > 0 && to.extra <= 16 );
        process_note_event_tmpl( event, to );
    };

    virtual void process_event( const events::value_change& event, parameter_port&, parameter_port& to ) override {
        if ( k_channel_mute == to.extra ) {
            const int32_t channel = event.get< int32_t >( ); // 1-based, like the ports
            jassert( channel > 0 && channel <= k_last_channel );
            channel_mute[ channel - 1 ] = !channel_mute[ channel - 1 ];
        } else if ( k_mute_all == to.extra ) {
            mute_all = !mute_all;
        }
    };

private:
    template < typename T >
    inline void process_note_event_tmpl( const T& event, parameter_port& to ) {
        emit( *channel_output_ports[ to.extra - 1 ], event );
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
        jassert( to.extra > 0 && to.extra <= 16 );

        emit( channel_output, event.k_event_type, to.extra, event.note, event.velocity );
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
        refcount{ } { }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );

        if ( 1 == ++refcount[ event.note ] ) {
            emit( output_port, event );
        }
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& /*to*/ ) override {
        jassert( event.note >= 0 && event.note < 128 );
        
        if ( 0 == refcount[ event.note ] ) return;
        
        if ( 0 == --refcount[ event.note ] ) {
            emit( output_port, event );
        }
    };
};

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi note store
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_store : public base_processor {
public:
    using container                  = containers::note_set;
    using parameter_initializer_list = base_processor::parameter_initializer_list;

public:
    container       notes;

public:
    midi_note_store( parameter_initializer_list inputs,
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

template < bool keep_lowest >
class midi_first_n_notes : public midi_note_store {
public:
    static constexpr int32_t k_note_input         = 0;
    static constexpr int32_t k_number_notes_input = 1;

    static constexpr int32_t k_note_output        = 0;

public:
    container       active_notes;
    uint32_t        number_notes_kept;

    parameter_port& output_port;

public:
    midi_first_n_notes( uint8_t the_number_notes_kept = 128 ) :
        midi_note_store(
            {
                { "notes",        k_midi_note, k_note_input         },
                { "number_notes", k_integer_value,     k_number_notes_input }
            },
            {
                { "notes",        k_midi_note, k_note_output        }
            }
        ),
        active_notes( ),
        number_notes_kept( the_number_notes_kept ),
        output_port( *this->get_output( "notes" ) ) {
    }

    using midi_note_store::process_event;

    virtual void process_event( const events::value_change& event, parameter_port&, parameter_port& to ) override {
        if ( to.extra == k_number_notes_input ) {
            number_notes_kept = event.get< uint32_t >( );
        }
    };

    virtual void process( void ) override {
        // the first number_notes_kept notes, from the bottom ( lowest ) or from the top ( highest )
        container kept;
        uint32_t  kept_count = 0;

        auto keep_first = [ & ] ( const auto& ordered_notes ) {
            for ( const midi_note& note : ordered_notes ) {
                if ( kept_count++ >= number_notes_kept ) break;
                kept.insert( note );
            }
        };

        if constexpr ( keep_lowest ) {
            keep_first( notes );
        } else {
            keep_first( notes.descending( ) );
        }

        container::diff( active_notes, kept,
            [ & ] ( const midi_note& note ) { this->emit( output_port, events::k_note_off, note.note, note.velocity ); },
            [ & ] ( const midi_note& note ) { this->emit( output_port, events::k_note_on,  note.note, note.velocity ); }
        );

        active_notes = kept;
    };
};

using midi_lowest_notes  = midi_first_n_notes< true  >;
using midi_highest_notes = midi_first_n_notes< false >;

//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi note arpeggiator
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_arpeggiator : public midi_note_store {
public:
    using base_type                     = midi_note_store;

public:
    static constexpr int32_t        k_chord_notes_input    = 0;
    static constexpr int32_t        k_sequence_notes_input = 1;

    static constexpr int32_t        k_notes_output         = 0;

    container                       chord_notes;
    container                       sequence_notes;

    parameter_port&                 output_port; // this->notes is treated as output as it has a generic name

    bool                            im_a_dirty_object;


public:
    midi_note_arpeggiator( ) :
        base_type(
            {
                { "chord_notes",       k_midi_note, k_chord_notes_input    },
                { "sequence_notes",    k_midi_note, k_sequence_notes_input }
            },
            {
                { "notes",             k_midi_note, k_notes_output         }
            }
        ),
        chord_notes( ),
        sequence_notes( ),
        output_port( *this->get_output( "notes" ) ),
        im_a_dirty_object( false ) { }

    using base_type::process_event;

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );
        switch ( to.extra ) {
            case k_chord_notes_input:    if ( chord_notes.insert(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes_input: if ( sequence_notes.insert( event ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    };

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( event.note >= 0 && event.note < 128 );

        switch ( to.extra ) {
            case k_chord_notes_input:    if ( chord_notes.erase(    event ) ) { im_a_dirty_object = true; } break;
            case k_sequence_notes_input: if ( sequence_notes.erase( event ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    };

    virtual void process( void ) override {
        if ( !im_a_dirty_object ) return;

        container active_notes; // stays empty - all notes off - without both chord and sequence notes

        if ( !chord_notes.empty( ) && !sequence_notes.empty( ) ) {
            std::array< midi_note, container::k_capacity > chord_tones; // indexable copy of the chord, lowest first
            size_t                                         chord_size = 0;

            for ( const midi_note& note : chord_notes ) {
                chord_tones[ chord_size++ ] = note;
            }

            // arp loop, computes sequence_notes X chord_notes into active_notes
            for ( const midi_note& sequence_note : sequence_notes ) {
                const size_t index        =   static_cast< size_t >( sequence_note.note ) % chord_size;
                const size_t octave_shift = ( static_cast< size_t >( sequence_note.note ) / chord_size ) * 12;

                int note_value = static_cast< int >( chord_tones[ index ].note ) + static_cast< int >( octave_shift );

                // if the arp generate a note out of range, it is folded back into range, an octave at a time (keeps the pitch class)
                while ( note_value > 127 ) {
                    note_value -= 12;
                }

                active_notes.insert( midi_note( static_cast< int8_t >( note_value ), sequence_note.velocity ) );
            }
        }

        // based on what was outputted previous time, it can be verified what's note on and off
        container::diff( notes, active_notes,
            [ & ] ( const midi_note& note ) { emit( output_port, events::note_off( note ) ); },
            [ & ] ( const midi_note& note ) { emit( output_port, events::note_on(  note ) ); }
        );

        // new notes are now old
        notes             = active_notes;
        im_a_dirty_object = false; // good boi
    }
};


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: chord intervals
//---------------------------------------------------------------------------------------------------------------------------


// TODO: step 2: abstract this in a way that it can have multiple different "decision engines" for the chord picking

class midi_chord_intervals : public base_processor {
public:
    using container                     = containers::note_set;
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
                { "scale_code",      k_integer_value,     k_scale_code_output      },
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
        scale_code_output.set( 0 );
    }


    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        note_t abs_note = static_cast< note_t >( event.note % 12 );

        switch ( to.extra ) {
            case k_root_note_input:        {           root_note = static_cast< note_t >( abs_note );      im_a_dirty_object = true; } break;
            case k_chord_notes_input:   if (   chord_notes.insert(                        event      ) ) { im_a_dirty_object = true; } break; // keeps the octave: the lowest note is the fallback root
            case k_pattern_notes_input: if ( pattern_notes.insert(                        event      ) ) { im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    }

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        note_t abs_note = static_cast< note_t >( event.note % 12 );

        switch ( to.extra ) {
            case k_root_note_input:     if ( root_note && root_note.value( ).note == abs_note   ) { root_note = std::nullopt; im_a_dirty_object = true; } break;
            case k_chord_notes_input:   if ( chord_notes.erase(                      event    ) ) {                           im_a_dirty_object = true; } break;
            case k_pattern_notes_input: if ( pattern_notes.erase(                    event    ) ) {                           im_a_dirty_object = true; } break;
            default: jassert( 0 );
        }
    }

    void all_notes_off( void ) {
        // note offs for whatever is on the output port
        std::for_each(
            chord_notes_output.begin( ),
            chord_notes_output.end( ),
            [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                emit( chord_notes_output_port, events::note_off( note ) );
            }
        );
        chord_notes_output.clear( );

        std::for_each(
            pattern_degrees_output.begin( ),
            pattern_degrees_output.end( ),
            [ & ] ( const midi_note& note ) { // in notes (output) but not in active_notes (freshly computed) - note off
                emit( pattern_degrees_output_port, events::note_off( note ) );
            }
        );
        pattern_degrees_output.clear( );

        scale_code.reset( );
        scale_code_output.set( 0 );
        emit( scale_code_output_port, scale_code_output );
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
            root_shift = static_cast< note_t >( chord_notes.front( ).note % 12 );
        }

        // refresh internal scale_code and check if the notes changed the output
        named_noteset::set_scale_code( chord_notes, scale_code, root_shift );
        chosen_chord = &( chord_data::midi_stuff_chord_guesser::get_best_fitting_named_noteset( scale_code ));

        // ------- scale_code output ------- //
        // if the execution reached this point, it means that the output is going to change
        // dispatch the scale code value change
        scale_code              = chosen_chord->scale_code;
        scale_code_output.set( scale_code.to_ulong( ) );
        emit( scale_code_output_port, scale_code_output );

        // ------- chord_notes output ------- //
        // and dispatch the note on/off events required to make the output to be equal to the new scale
        container active_notes;
        named_noteset::set_notes_from_scale_code( scale_code, active_notes, root_shift );
        active_notes.insert( root_shift );

        // according to previous publication of note events, the new notes can be prescribed by the mutual exclution of sets A and B </blablabla>
        container::diff( chord_notes_output, active_notes,
            // in chord_notes_output (output) but not in active_notes (freshly computed) - note off
            [ & ] ( const midi_note& note ) {
                emit( chord_notes_output_port, events::note_off( note ) );
            },
            // in active_notes (freshly computed) but not in chord_notes_output - note on
            [ & ] ( const midi_note& note ) {
                emit( chord_notes_output_port, events::note_on( note ) );
            }
        );

        // new notes are now part of music history
        chord_notes_output = active_notes;

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
                            // lower bound degree - chord_notes_output holds root_shift + interval, compare intervals
                            return abs_note <= chord_note.note - root_shift;
                        }
                    ) == chord_notes_output.end( ) ) {
                    // when not found, it means that the pattern note is an accident upper bound
                    // other accidents are going to be auto lower bound
                    // possible experiments:
                    // - use a scale?
                    // - accidents_output?
                    note_degree = 0;
                }

                const int new_note = static_cast< int >( note_degree ) + ( ( ( note.note + 12 ) - root_shift ) / 12 - 1 ) * static_cast< int >( chord_notes_output.size( ) );

                if ( new_note >= 0 && new_note < 128 ) { // pattern notes below the lowest root ( midi notes < root_shift ) have no degree
                    active_notes.insert( static_cast< note_t >( new_note ) );
                }
            }
        );

        // once again, loudly blaber the words cibtained in the ancient scrolls of union theory, 
        // commanding events for mutually exclusive existing notes
        container::diff( pattern_degrees_output, active_notes,
            // in pattern_degrees_output (output) but not in active_notes (freshly computed) - note off
            [ & ] ( const midi_note& note ) {
                emit( pattern_degrees_output_port, events::note_off( note ) );
            },
            // in active_notes (freshly computed) but not in pattern_degrees_output - note on
            [ & ] ( const midi_note& note ) {
                emit( pattern_degrees_output_port, events::note_on( note ) );
            }
        );

        pattern_degrees_output = active_notes;

        im_a_dirty_object = false; // yeessss.... yes... excellent...
    }
};


//---------------------------------------------------------------------------------------------------------------------------
// midi stuff :: processors :: midi note wormhole
// 
// makes notes available across threads
//---------------------------------------------------------------------------------------------------------------------------

class midi_note_wormhole : public base_processor {
public:
    using container              = midi_stuff::containers::note_set;

    template < uint32_t _uuid >
    using buffered_container     = midi_stuff::threads::buffered_object_instance< container, _uuid >;

    template < uint32_t _uuid >
    struct input_output_strip {
        input_output_strip( parameter_port& _output_port ) : output_port( _output_port ) {}

        buffered_container< _uuid > buffered_input;
        bool                        needs_input_flush = false;

        parameter_port&             output_port;
        container                   output;
    };

    input_output_strip<  1 > io_strip_1;
    input_output_strip<  2 > io_strip_2;
    input_output_strip<  3 > io_strip_3;
    input_output_strip<  4 > io_strip_4;
    input_output_strip<  5 > io_strip_5;
    input_output_strip<  6 > io_strip_6;
    input_output_strip<  7 > io_strip_7;
    input_output_strip<  8 > io_strip_8;
    input_output_strip<  9 > io_strip_9;
    input_output_strip< 10 > io_strip_10;
    input_output_strip< 11 > io_strip_11;
    input_output_strip< 12 > io_strip_12;
    input_output_strip< 13 > io_strip_13;
    input_output_strip< 14 > io_strip_14;
    input_output_strip< 15 > io_strip_15;
    input_output_strip< 16 > io_strip_16;
public:
    midi_note_wormhole( void ) : base_processor(
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
    io_strip_1(  *this->get_output( "channel_1"  ) ),
    io_strip_2(  *this->get_output( "channel_2"  ) ),
    io_strip_3(  *this->get_output( "channel_3"  ) ),
    io_strip_4(  *this->get_output( "channel_4"  ) ),
    io_strip_5(  *this->get_output( "channel_5"  ) ),
    io_strip_6(  *this->get_output( "channel_6"  ) ),
    io_strip_7(  *this->get_output( "channel_7"  ) ),
    io_strip_8(  *this->get_output( "channel_8"  ) ),
    io_strip_9(  *this->get_output( "channel_9"  ) ),
    io_strip_10( *this->get_output( "channel_10" ) ),
    io_strip_11( *this->get_output( "channel_11" ) ),
    io_strip_12( *this->get_output( "channel_12" ) ),
    io_strip_13( *this->get_output( "channel_13" ) ),
    io_strip_14( *this->get_output( "channel_14" ) ),
    io_strip_15( *this->get_output( "channel_15" ) ),
    io_strip_16( *this->get_output( "channel_16" ) ) {
    }

    void do_with_strip( int32_t strip, auto lambda ) {
        switch ( strip ) {
            case 1  : lambda( io_strip_1  ); break;
            case 2  : lambda( io_strip_2  ); break;
            case 3  : lambda( io_strip_3  ); break;
            case 4  : lambda( io_strip_4  ); break;
            case 5  : lambda( io_strip_5  ); break;
            case 6  : lambda( io_strip_6  ); break;
            case 7  : lambda( io_strip_7  ); break;
            case 8  : lambda( io_strip_8  ); break;
            case 9  : lambda( io_strip_9  ); break;
            case 10 : lambda( io_strip_10 ); break;
            case 11 : lambda( io_strip_11 ); break;
            case 12 : lambda( io_strip_12 ); break;
            case 13 : lambda( io_strip_13 ); break;
            case 14 : lambda( io_strip_14 ); break;
            case 15 : lambda( io_strip_15 ); break;
            case 16 : lambda( io_strip_16 ); break;
            default:                         break;
        }
    }

    void do_with_all_strips( auto lambda ) {
        lambda( io_strip_1 );
        lambda( io_strip_2 );
        lambda( io_strip_3 );
        lambda( io_strip_4 );
        lambda( io_strip_5 );
        lambda( io_strip_6 );
        lambda( io_strip_7 );
        lambda( io_strip_8 );
        lambda( io_strip_9 );
        lambda( io_strip_10 );
        lambda( io_strip_11 );
        lambda( io_strip_12 );
        lambda( io_strip_13 );
        lambda( io_strip_14 );
        lambda( io_strip_15 );
        lambda( io_strip_16 );
    }

    virtual void process_event( const events::note_on& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( to.extra <= 16 && to.extra > 0 );

        do_with_strip(
            to.extra,
            [ & ]( auto& strip ){
                if ( strip.buffered_input.buffer.insert( event ) ) {
                    strip.needs_input_flush = true;
                }
            }
        );
    }

    virtual void process_event( const events::note_off& event, parameter_port& /*from*/, parameter_port& to ) override {
        jassert( to.extra <= 16 && to.extra > 0 );

        do_with_strip(
            to.extra,
            [ & ] ( auto& strip ) {
                if ( strip.buffered_input.buffer.erase( event ) ) {
                    strip.needs_input_flush = true;
                }
            }
        );
    }

    void all_notes_off( void ) {
        do_with_all_strips(
            [ & ] ( auto& strip ) {
                if ( !strip.buffered_input.buffer.empty( ) ) {
                    strip.buffered_input.buffer.clear( );
                    strip.needs_input_flush = true;
                }
            }
        );
    }

    virtual void process( void ) override {
        do_with_all_strips(
            [ & ] ( auto& strip ) {

                bool should_update = false;
                // if it is flushing the input, uptates the output, based on the input
                if ( strip.needs_input_flush ) {
                    strip.needs_input_flush = false;
                    strip.buffered_input.flush( );
                    should_update = true;
                } else {
                    should_update = strip.buffered_input.read( );
                }

                if ( !should_update ) {
                    return;
                }

                container::diff(
                    strip.output,
                    strip.buffered_input.buffer,
                    [ & ] ( const midi_note& note ) {
                        emit( strip.output_port, events::note_off( note ) );
                    },
                    [ & ] ( const midi_note& note ) {
                        emit( strip.output_port, events::note_on( note ) );
                    }
                );

                strip.output = strip.buffered_input.buffer;
            }
        );
        
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
