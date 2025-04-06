#pragma once

#include <map>

#include "chordcat.hpp"

class note_map {
public:
    typedef chordcat::t::note                           note_t;
    typedef int8_t                                      refcount_t;

    // TODO: if this is to be made hardware compatible, std:: stuff needs to go away
    typedef std::map< note_t, refcount_t >              refcount_map_t;
    typedef std::map< note_t, refcount_t >              refcount_ordered_map_t;

protected:
    note_t                                   lowest;
    refcount_map_t                           mod_note_ref;
    refcount_ordered_map_t                   note_ref;
    
    template < typename T >
    bool inc_ref( note_t note, T& map ) {
        auto insert_result = map.insert(
            std::pair< note_t, refcount_t >(
                static_cast<note_t>( note ),
                static_cast<refcount_t>( 1 )
            )
        );

        if ( !insert_result.second ) {
            ++insert_result.first->second;
        } else {
            return true;
        }

        return false;
    }

    template < typename T >
    bool dec_ref( note_t note, T& map ) {
        auto itr = map.find( static_cast<note_t>( note ) );

        if ( itr == map.end( ) ) {
            return false;
        }

        if ( --itr->second < 1 ) {
            map.erase( itr );
            return true;
        }

        return false;
    }

 public:
    note_map( void ) : lowest( 128 ) { };

    void clear( void ) {
        lowest = 128;
        mod_note_ref.clear( );
        note_ref.clear( );
    }

    bool note_on( note_t note ) {
        bool dirty = inc_ref( note % 12, mod_note_ref );
        inc_ref( note, note_ref );

        if ( note < lowest ) {
            lowest = note;
            dirty  = true;
        }

        return dirty;
    }

    bool note_off( note_t note ) {

        bool dirty = dec_ref( note % 12, mod_note_ref );
        dec_ref( note, note_ref );

        lowest = note_ref.empty() ? static_cast< chordcat::t::note>( 128 ) : note_ref.begin()->first;

        return dirty;
    }

    inline bool operator+=( const note_t note ) { return note_on(  note ); }
    inline bool operator-=( const note_t note ) { return note_off( note ); }

    const refcount_map_t&         getCounters( void ) { return mod_note_ref;          }
    note_t                        getLowest( void )   { return lowest;                }
    bool                          hasNotes( void )    { return !mod_note_ref.empty(); }
};