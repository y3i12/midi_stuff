#include "midi_stuff.h"


#define define_wormhole_static_buffer_instance( id )                                                                             \
    midi_stuff::threads::concurrent_object<        midi_stuff::processors::midi_note_wormhole::container     >                   \
    midi_stuff::threads::buffered_object_instance< midi_stuff::processors::midi_note_wormhole::container, id >::concurrent_object;

define_wormhole_static_buffer_instance(  1 )
define_wormhole_static_buffer_instance(  2 )
define_wormhole_static_buffer_instance(  3 )
define_wormhole_static_buffer_instance(  4 )
define_wormhole_static_buffer_instance(  5 )
define_wormhole_static_buffer_instance(  6 )
define_wormhole_static_buffer_instance(  7 )
define_wormhole_static_buffer_instance(  8 )
define_wormhole_static_buffer_instance(  9 )
define_wormhole_static_buffer_instance( 10 )
define_wormhole_static_buffer_instance( 11 )
define_wormhole_static_buffer_instance( 12 )
define_wormhole_static_buffer_instance( 13 )
define_wormhole_static_buffer_instance( 14 )
define_wormhole_static_buffer_instance( 15 )
define_wormhole_static_buffer_instance( 16 )