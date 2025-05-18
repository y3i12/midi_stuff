#include "midi_stuff_juce_plugin.h"
#include "BinaryData.h"

namespace parameter_id {
static juce::String receiver{ "receiver" };
}

midi_stuff_plugin_processor::midi_stuff_plugin_processor( void ) :
    plugin_processor( ),
    parameters(
        *this,
        nullptr,
        juce::Identifier( "midi_stuff" ),
        {
            std::make_unique< juce::AudioParameterFloat >(
                juce::ParameterID( parameter_id::receiver, 1 ),
                "Receiver",
                juce::NormalisableRange<float>( 0.0f, 1.0f, 1.0f ),
                0.0f
            )
        }
    ) {

    is_receiver = dynamic_cast< juce::AudioParameterFloat* >( parameters.getParameter( parameter_id::receiver ) );

    // ui
    // magicState.setGuiValueTree( BinaryData::midi_stuff_xml, BinaryData::midi_stuff_xmlSize );
}

midi_stuff_plugin_processor::~midi_stuff_plugin_processor( void ) {

}

void midi_stuff_plugin_processor::process_midi( juce::MidiBuffer& midi_buffer ) {
    auto should_recveive = is_receiver->get();

    if ( should_recveive > 0.0f ) {
        receiver.process_midi( midi_buffer );
    } else {
        processor.process_midi( midi_buffer );
    }
}


juce::AudioProcessorEditor* midi_stuff_plugin_processor::createEditor( void ) {
    return new midi_stuff_plugin_editor( *this ); 
}

void midi_stuff_plugin_processor::getStateInformation( juce::MemoryBlock& destData ) {
    //auto s = MemoryOutputStream( destData, true );

    //s.writeInt( *instanceBehaviour );
    //s.writeInt( *firstDegreeCode );
    //s.writeInt( *whenNoChordNote );
    //s.writeInt( *whenSingleChordNote );
    //s.writeInt( *patternNotesMapping );
    //s.writeInt( *numMillisecsOfLatency );
    //s.writeInt( *patternNotesWraparound );
    //s.writeInt( *unmappedNotesBehaviour );
}

// Reload state info
void midi_stuff_plugin_processor::setStateInformation( const void* data, int sizeInBytes ) {
    //auto s = MemoryInputStream( data, static_cast< size_t >( sizeInBytes ), false );
    //*instanceBehaviour = s.readInt( );
    //*firstDegreeCode = s.readInt( );
    //*whenNoChordNote = s.readInt( );
    //*whenSingleChordNote = s.readInt( );
    //*patternNotesMapping = s.readInt( );
    //*numMillisecsOfLatency = s.readInt( );
    //*patternNotesWraparound = s.readInt( );
    //*unmappedNotesBehaviour = s.readInt( );
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter( ) {
    return new midi_stuff_plugin_processor( );
}


//==============================================================================
midi_stuff_plugin_editor::midi_stuff_plugin_editor( midi_stuff_plugin_processor& p )
    : AudioProcessorEditor( &p ), Timer( ), processor( p ) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize( 400, 300 );
    startTimerHz( 30 );
}

midi_stuff_plugin_editor::~midi_stuff_plugin_editor( ) {
    stopTimer( );
}

//==============================================================================
void midi_stuff_plugin_editor::paint( juce::Graphics& g ) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (juce::FontOptions (15.0f));

     // fill the whole window white
    g.fillAll( juce::Colours::white );

    // set the current drawing colour to black
    g.setColour( juce::Colours::black );

    // set the font size and draw text to the screen
    g.setFont( 15.0f );

    g.drawFittedText( "Yo Modafacka", getLocalBounds( ), juce::Justification::centred, 10 );
}

void midi_stuff_plugin_editor::timerCallback( void ) {
    /*if ( processor.chord_changed ) {
        repaint( );

        audioProcessor.chord_changed = false;
    }*/
}

void midi_stuff_plugin_editor::resized( void ) {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
