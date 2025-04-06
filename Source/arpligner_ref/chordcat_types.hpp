
#pragma once

#include <set>
#include <string>
#include <vector>

namespace chordcat {
namespace t {

typedef unsigned char     note;
typedef std::set< note >  note_set;

template < class _note_set_t > struct _named_note_set {
    typedef _note_set_t _note_set_t;

    std::string        name;
    _note_set_t        notes;

    _named_note_set( const std::string&  _name, const _note_set_t&  _notes ) : name( _name ), notes( _notes ) { }
    _named_note_set( const std::string&& _name, const _note_set_t&& _notes ) : name( _name ), notes( _notes ) { }
    _named_note_set( const char*         _name, const _note_set_t&  _notes ) : name( _name ), notes( _notes ) { }
    _named_note_set( const char*         _name, const _note_set_t&& _notes ) : name( _name ), notes( _notes ) { }
};

typedef _named_note_set< note_set >  named_noteset;
typedef std::vector< named_noteset > named_noteset_list;

} // namespace t
} // namespace chordcat