#pragma once

#include "plugin_processor.h"
#include "midi_stuff_processor.h"
#include "midi_stuff_receiver.h"
#include "midi_stuff/juce_stuff.h"

class midi_stuff_plugin_processor : public plugin_processor {
public:
    midi_stuff_processor                               processor;
    midi_stuff_receiver                                receiver;

    // UI
    juce::AudioProcessorValueTreeState                 parameters;
    juce::AudioParameterBool*                          is_receiver = nullptr;

public:
    midi_stuff_plugin_processor( void );
    ~midi_stuff_plugin_processor( void ) override;


    void                        process_midi( juce::MidiBuffer& midi_buffer )            override;

    juce::AudioProcessorEditor* createEditor( void )                                     override;
    void                        getStateInformation( juce::MemoryBlock& destData )       override;
    void                        setStateInformation( const void* data, int sizeInBytes ) override;
};


constexpr juce::Grid::Px operator""_px( long double        px ) { return juce::Grid::Px{ px }; }
constexpr juce::Grid::Px operator""_px( unsigned long long px ) { return juce::Grid::Px{ px }; }
constexpr juce::Grid::Fr operator""_fr( unsigned long long fr ) { return juce::Grid::Fr{ fr }; }

//==============================================================================
/**
*/
class midi_stuff_plugin_editor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    using tab_orientation_t = juce::TabbedButtonBar::Orientation;
    using label             = juce::Label;


    static inline const tab_orientation_t tab_orientation = juce::TabbedButtonBar::TabsAtBottom;

public:
    midi_stuff_plugin_editor( midi_stuff_plugin_processor& );
    ~midi_stuff_plugin_editor( ) override;

    //==============================================================================
    void paint( juce::Graphics& ) override;
    void timerCallback( void ) override;
    void resized( void ) override;

private:
    midi_stuff_plugin_processor& processor;

    juce_stuff::components::with_tabs< tab_orientation >    tabs = {
        {
            "Processor",
            findColour( juce::ResizableWindow::backgroundColourId ),
            juce_stuff::components::factory::build< label >( "processor_tab", "Processor" ),
            true,
            [ & ] ( juce::TabBarButton* button ) {
                button->onClick = [ & ] {
                    *processor.is_receiver = false;
                };
            }
        },
        {
            "Receiver",
            findColour( juce::ResizableWindow::backgroundColourId ),
            juce_stuff::components::factory::build_and_lambda< juce_stuff::components::with_grid< > >(
                [ & ] ( juce_stuff::components::with_grid< >& component ) {
                    using Track = juce::Grid::TrackInfo;

                    juce::Grid& grid     = component.grid;

                    grid.rowGap          = 20_px;
                    grid.columnGap       = 20_px;

                    grid.templateRows    = { Track( 1_fr ), Track( 1_fr ), Track( 1_fr ) };
                    grid.templateColumns = { Track( 1_fr ) };
                    grid.autoColumns     = Track( 1_fr );
                    grid.autoRows        = Track( 1_fr );
                    grid.autoFlow        = juce::Grid::AutoFlow::column;
                },
                {
                    juce_stuff::components::factory::build_and_lambda< juce::ToggleButton >(
                        [ & ]( juce::ToggleButton& toggle_button ) {
                            toggle_button.setButtonText( "Chord Notes" );
                            toggle_button.onClick = [ & ] {
                                processor.receiver.filter.channel_mute[ 0 ] = toggle_button.getToggleState( );
                            };
                        },
                        "channel_1_toggle_button"
                    ),

                    juce_stuff::components::factory::build_and_lambda< juce::ToggleButton >(
                        [ & ] ( juce::ToggleButton& toggle_button ) {
                            toggle_button.setButtonText( "Pattern Degrees" );
                            toggle_button.onClick = [ & ] {
                                processor.receiver.filter.channel_mute[ 1 ] = toggle_button.getToggleState( );
                            };
                        },
                        "channel_2_toggle_button"
                    ),

                    juce_stuff::components::factory::build_and_lambda< juce::ToggleButton >(
                        [ & ] ( juce::ToggleButton& toggle_button ) {
                            toggle_button.setButtonText( "Notes" );
                            toggle_button.onClick = [ & ] {
                                processor.receiver.filter.channel_mute[ 2 ] = toggle_button.getToggleState( );
                            };
                        },
                        "channel_3_toggle_button"
                    )
                } 
            ),
            true,
            [ & ] ( juce::TabBarButton* button ) {
                button->onClick = [ & ] {
                    *processor.is_receiver = true;
                };
            }
        }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( midi_stuff_plugin_editor )
};

//struct ButtonsPage : public ComponentTreeNode {
//    ButtonsPage( void ) {
//        {
//            auto* group = addToList( new juce::GroupComponent( "group", "Radio buttons" ) );
//            group->setBounds( 20, 20, 220, 140 );
//        }
//        for ( int i = 0; i < 4; ++i ) {
//            auto* tb = addToList( new TextButton( "Button " + String( i + 1 ) ) );
//
//            tb->setClickingTogglesState( true );
//            tb->setRadioGroupId( 34567 );
//            tb->setColour( TextButton::textColourOffId, Colours::black );
//            tb->setColour( TextButton::textColourOnId, Colours::black );
//            tb->setColour( TextButton::buttonColourId, Colours::white );
//            tb->setColour( TextButton::buttonOnColourId, Colours::blueviolet.brighter( ) );
//
//            tb->setBounds( 20 + i * 55, 260, 55, 24 );
//            tb->setConnectedEdges( ( ( i != 0 ) ? Button::ConnectedOnLeft : 0 )
//                                   | ( ( i != 3 ) ? Button::ConnectedOnRight : 0 ) );
//
//            if ( i == 0 )
//                tb->setToggleState( true, dontSendNotification );
//        }
//    }
//
//
//};
