#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Parameters.h"
#include "DSP/RFSimulationEngine.h"
#include "DSP/JitterBuffer.h"
#include "DSP/PacketLossConcealment.h"
#include "DSP/SBCFilterBank.h"
#include "DSP/HandsFreeProfile.h"
#include "DSP/StereoDesyncEngine.h"
#include "DSP/HeadphoneAcoustics.h"

class BlueToothRotAudioProcessor : public juce::AudioProcessor
{
public:
    BlueToothRotAudioProcessor();
    ~BlueToothRotAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Real-time link telemetry for GUI meters
    bool isChannelDropping(int ch) const { return currentDropState[ch % 2]; }
    float getLinkQuality() const { return currentLinkQuality; }

private:
    juce::AudioProcessorValueTreeState apvts;

    // DSP Subsystems
    BlueToothDSP::RFSimulationEngine rfEngine;
    BlueToothDSP::JitterBuffer jitterBuffer;
    BlueToothDSP::PacketLossConcealment plcEngine;
    BlueToothDSP::SBCFilterBank sbcFilter;
    BlueToothDSP::HandsFreeProfile hfpEngine;
    BlueToothDSP::StereoDesyncEngine desyncEngine;
    BlueToothDSP::HeadphoneAcoustics headphoneEngine;

    // Telemetry state
    std::atomic<bool> currentDropState[2] { false, false };
    std::atomic<float> currentLinkQuality { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlueToothRotAudioProcessor)
};
