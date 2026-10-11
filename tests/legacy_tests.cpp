// The original processor tests run as static initializers (see the tuple at the end of tests.hpp).
// Previously midi_stuff.h included them, so they ran inside the plugin; now they live here.
// Failures are reported by jassert (JUCE logs "JUCE Assertion failure" in debug builds).

#include "midi_stuff/tests.hpp"

#include <cstdio>

int main( ) {
    std::puts( "midi_stuff legacy tests ran (look for JUCE assertion failures above)" );
    return 0;
}
