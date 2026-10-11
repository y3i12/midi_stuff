cmake_minimum_required( VERSION 3.22 )

project( midi_stuff VERSION 0.0.1 LANGUAGES C CXX )

set( CMAKE_CXX_STANDARD          20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS        OFF )
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

option( MIDI_STUFF_BUILD_PLUGIN "Build the JUCE plugin (VST3 + Standalone)" ON )
option( MIDI_STUFF_BUILD_TESTS  "Build the JUCE-free core tests"            ON )

include( FetchContent )

#----------------------------------------------------------------------------------------------------------------------------
# midi_stuff core - header only, no JUCE required
#----------------------------------------------------------------------------------------------------------------------------

add_library( midi_stuff_core INTERFACE )
add_library( midi_stuff::core ALIAS midi_stuff_core )

target_include_directories( midi_stuff_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/midi_stuff
)

if( MSVC )
    target_compile_options( midi_stuff_core INTERFACE /utf-8 /permissive- /Zc:__cplusplus )
endif( )

#----------------------------------------------------------------------------------------------------------------------------
# plugin
#----------------------------------------------------------------------------------------------------------------------------

if( MIDI_STUFF_BUILD_PLUGIN )
    # to use a local JUCE checkout instead of downloading:
    #   cmake -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE ...
    FetchContent_Declare( JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable( JUCE )

    juce_add_plugin( midi_stuff
        PRODUCT_NAME                "midi_stuff"
        VERSION                     ${PROJECT_VERSION}
        COMPANY_NAME                "y3i12"
        COMPANY_WEBSITE             "y3i12.com"
        COMPANY_COPYRIGHT           "Yarp."
        DESCRIPTION                 "does stuff to midi"
        PLUGIN_MANUFACTURER_CODE    y313
        PLUGIN_CODE                 Ibgt   # same code the .jucer generated (0x49626774), keeps DAW sessions valid
        FORMATS                     VST3 Standalone
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           TRUE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES             Fx Instrument Tools
        COPY_PLUGIN_AFTER_BUILD     FALSE
    )

    juce_generate_juce_header( midi_stuff )

    target_sources( midi_stuff PRIVATE
        Source/plugin_processor.cpp
        Source/midi_stuff_juce_plugin.cpp
    )

    target_compile_definitions( midi_stuff PUBLIC
        MIDI_STUFF_WITH_JUCE=1
        DONT_SET_USING_JUCE_NAMESPACE=1   # same as the .jucer's addUsingNamespaceToJuceHeader="0"
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_link_libraries( midi_stuff
        PRIVATE
            midi_stuff::core
            juce::juce_audio_utils
            juce::juce_gui_extra
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
    )

    # the original static-init tests (Source/midi_stuff/tests.hpp), now in their own executable
    juce_add_console_app( midi_stuff_legacy_tests PRODUCT_NAME "midi_stuff_legacy_tests" )
    juce_generate_juce_header( midi_stuff_legacy_tests )
    target_sources( midi_stuff_legacy_tests PRIVATE tests/legacy_tests.cpp )
    target_compile_definitions( midi_stuff_legacy_tests PRIVATE
        MIDI_STUFF_WITH_JUCE=1
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )
    target_link_libraries( midi_stuff_legacy_tests
        PRIVATE
            midi_stuff::core
            juce::juce_audio_basics
            juce::juce_recommended_config_flags
    )
endif( )

#----------------------------------------------------------------------------------------------------------------------------
# tests
#----------------------------------------------------------------------------------------------------------------------------

if( MIDI_STUFF_BUILD_TESTS )
    FetchContent_Declare( doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable( doctest )

    enable_testing( )

    add_executable( midi_stuff_tests
        tests/main.cpp
        tests/chord_guesser_tests.cpp
        tests/processor_tests.cpp
    )

    target_link_libraries( midi_stuff_tests PRIVATE midi_stuff::core doctest::doctest )

    if( NOT MSVC )
        target_compile_options( midi_stuff_tests PRIVATE -Wall -Wextra )
    endif( )

    add_test( NAME midi_stuff_tests COMMAND midi_stuff_tests )
endif( )
