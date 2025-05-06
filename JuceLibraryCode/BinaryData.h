/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   midi_stuff_xml;
    const int            midi_stuff_xmlSize = 1607;

    extern const char*   LICENSE;
    const int            LICENSESize = 1235;

    extern const char*   README_md;
    const int            README_mdSize = 40;

    extern const char*   mid_point_flt_typo_absa_should_be_absb_png;
    const int            mid_point_flt_typo_absa_should_be_absb_pngSize = 926107;

    extern const char*   mid_point_ptr_png;
    const int            mid_point_ptr_pngSize = 325251;

    extern const char*   mid_point_png;
    const int            mid_point_pngSize = 501223;

    extern const char*   generate_bat;
    const int            generate_batSize = 704;

    extern const char*   generate_fsm_bat;
    const int            generate_fsm_batSize = 66;

    extern const char*   generate_largest_bat;
    const int            generate_largest_batSize = 71;

    extern const char*   generate_message_packet_bat;
    const int            generate_message_packet_batSize = 88;

    extern const char*   generate_message_router_bat;
    const int            generate_message_router_batSize = 88;

    extern const char*   generate_smallest_bat;
    const int            generate_smallest_batSize = 73;

    extern const char*   generate_type_lookup_bat;
    const int            generate_type_lookup_batSize = 79;

    extern const char*   generate_type_select_bat;
    const int            generate_type_select_batSize = 79;

    extern const char*   generate_type_traits_bat;
    const int            generate_type_traits_batSize = 80;

    extern const char*   generate_variant_pool_bat;
    const int            generate_variant_pool_batSize = 81;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 16;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
