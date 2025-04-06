// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "chordcat_types.hpp"

namespace chordcat {

static const chordcat::t::named_noteset_list scale_db = {
    { "Major",              { 2, 4, 5, 7, 9, 11 } },
    { "Dorian",             { 2, 3, 5, 7, 9, 10 } },
    { "Phrygian",           { 1, 3, 5, 7, 8, 10 } },
    { "Lydian",             { 2, 4, 6, 7, 9, 11 } },
    { "Mixolydian",         { 2, 4, 5, 7, 9, 10 } },
    { "Aeolian",            { 2, 3, 5, 7, 8, 10 } },
    { "Locrian",            { 1, 3, 5, 6, 8, 10 } },
    { "Harmonic Minor",     { 2, 3, 5, 7, 8, 11 } },
    { "Locrian Natural 6",  { 1, 3, 5, 6, 9, 10 } },
    { "Augmented Major",    { 2, 4, 5, 8, 9, 11 } },
    { "Dorian #11",         { 2, 3, 6, 7, 9, 10 } },
    { "Phrygian Dominant",  { 1, 4, 5, 7, 8, 10 } },
    { "Lydian #2",          { 3, 4, 6, 7, 9, 11 } },
    { "Super Locrian bb7",  { 1, 3, 4, 6, 8,  9 } },
    { "Jazz Minor",         { 2, 3, 5, 7, 9, 11 } },
    { "Dorian b2",          { 1, 3, 5, 7, 9, 10 } },
    { "Lydian Augmented",   { 2, 4, 6, 8, 9, 11 } },
    { "Lydian Dominant",    { 2, 4, 6, 7, 9, 10 } },
    { "Aeolian Dominant",   { 2, 4, 5, 7, 8, 10 } },
    { "Half-diminished",    { 2, 3, 5, 6, 8, 10 } },
    { "Altered",            { 1, 3, 4, 6, 8, 10 } },
};

} // namespace chordcat