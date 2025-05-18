/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class plugin_processor : public juce::AudioProcessor
{
public:
    //==============================================================================
    plugin_processor();
    ~plugin_processor() override;


    virtual void process_midi( juce::MidiBuffer& ) = 0;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;


    virtual bool hasEditor( ) const override { return true; }
    virtual juce::AudioProcessorEditor* createEditor( void ) = 0;

    virtual void getStateInformation( juce::MemoryBlock& destData ) override = 0;
    virtual void setStateInformation( const void* data, int sizeInBytes ) override = 0;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (plugin_processor)
};
