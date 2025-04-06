/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "chordcat.hpp"

#include "midi_stuff/basic_types.h"
#include "midi_stuff/double_link.h"
#include "midi_stuff/ref_counted_double_link.h"
// #include "midi_stuff/note.h"
#include <string>
#include <ostream>

using namespace midi_stuff;



//==============================================================================
SmarpeggiatorAudioProcessor::SmarpeggiatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , chord_changed( false )

{
    note_keeper.consumes_from( note_input_dispatcher );
    dispatcher_handle = note_keeper.dispatcher.listen< note_event_t >(
        [this]( const note_event_t& the_event ) {
            if ( !this->current_midi_buffer ) {
                return;
            }

            if ( note_event_t::k_note_on == the_event.event_type ) {
                this->current_midi_buffer->addEvent(
                    juce::MidiMessage::noteOn(
                        the_event.midi_note->channel + 1,
                        the_event.midi_note->note,
                        the_event.midi_note->velocity
                    ),
                    0
                );
            } else if ( note_event_t::k_note_off == the_event.event_type ) {
                this->current_midi_buffer->addEvent(
                    juce::MidiMessage::noteOff(
                        the_event.midi_note->channel + 1,
                        the_event.midi_note->note
                    ),
                    0
                );
            } if ( note_event_t::k_all_notes_off == the_event.event_type ) {
                this->current_midi_buffer->addEvent(
                    juce::MidiMessage::allNotesOff( the_event.midi_note->channel + 1 ),
                    0
                );
            }
        }
    );
}

SmarpeggiatorAudioProcessor::~SmarpeggiatorAudioProcessor()
{
}

//==============================================================================
const juce::String SmarpeggiatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SmarpeggiatorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SmarpeggiatorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SmarpeggiatorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SmarpeggiatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SmarpeggiatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SmarpeggiatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SmarpeggiatorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SmarpeggiatorAudioProcessor::getProgramName (int index)
{
    return {};
}

void SmarpeggiatorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SmarpeggiatorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SmarpeggiatorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SmarpeggiatorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SmarpeggiatorAudioProcessor::processBlockBypassed( juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages ) {
    process_midi( midi_messages, true );
}

void SmarpeggiatorAudioProcessor::process_midi( juce::MidiBuffer& midi_messages, bool read_only ) {
    for ( const auto metadata : midi_messages ) {
        const juce::MidiMessage         midi_message = metadata.getMessage( );
        const int                       channel     = midi_message.getChannel( );

        if ( midi_message.isNoteOn( ) ) {
            note_input_dispatcher.note_on(
                midi_message.getChannel( ),
                static_cast< chordcat::t::note >( midi_message.getNoteNumber( ) ),
                midi_message.getVelocity( )
            );
        } else if ( midi_message.isNoteOff( ) ) {
            note_input_dispatcher.note_off(
                midi_message.getChannel( ),
                static_cast< chordcat::t::note >( midi_message.getNoteNumber( ) )
            );
        } else if ( midi_message.isAllNotesOff( ) || midi_message.isAllSoundOff( ) ) {
            note_input_dispatcher.all_notes_off( midi_message.getChannel( ) );
        }
    }

    // current_midi_buffer is used by the callback defined in the constructor of this class,
    // activated when note_keeper.dispatcher.process() is called
    current_midi_buffer = &midi_messages;
    midi_messages.clear( );

    note_input_dispatcher.dispatcher.process( );
    note_keeper.dispatcher.process( );
}

void SmarpeggiatorAudioProcessor::processBlock( juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages ) {
    buffer.clear( );
    process_midi( midi_messages, false );
}




//==============================================================================
bool SmarpeggiatorAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)c
}

juce::AudioProcessorEditor* SmarpeggiatorAudioProcessor::createEditor()
{
    return new SmarpeggiatorAudioProcessorEditor (*this);
}

//==============================================================================
void SmarpeggiatorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SmarpeggiatorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SmarpeggiatorAudioProcessor();
}
