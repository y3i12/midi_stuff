/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "plugin_processor.h"

//==============================================================================
plugin_processor::plugin_processor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : foleys::MagicProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    FOLEYS_SET_SOURCE_PATH( __FILE__ );
}

plugin_processor::~plugin_processor()
{
}

//==============================================================================
const juce::String plugin_processor::getName() const
{
    return JucePlugin_Name;
}

bool plugin_processor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool plugin_processor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool plugin_processor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double plugin_processor::getTailLengthSeconds() const
{
    return 0.0;
}

int plugin_processor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int plugin_processor::getCurrentProgram()
{
    return 0;
}

void plugin_processor::setCurrentProgram (int index)
{
}

const juce::String plugin_processor::getProgramName (int index)
{
    return {};
}

void plugin_processor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void plugin_processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void plugin_processor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool plugin_processor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void plugin_processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    for ( auto i = 0; i < buffer.getNumChannels( ); ++i )
        buffer.clear( i, 0, buffer.getNumSamples( ) );
    process_midi( midiMessages );
}