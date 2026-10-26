#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace BlueToothDSP
{
    // Parameter IDs
    inline const juce::ParameterID paramDistance        {"distance", 1};
    inline const juce::ParameterID paramObstacle        {"obstacle", 1};
    inline const juce::ParameterID paramInterference    {"interference", 1};
    inline const juce::ParameterID paramCodecProfile    {"codecProfile", 1};
    inline const juce::ParameterID paramBitpool         {"bitpool", 1};
    inline const juce::ParameterID paramJitterDrift     {"jitterDrift", 1};
    inline const juce::ParameterID paramBufferSizeMs    {"bufferSizeMs", 1};
    inline const juce::ParameterID paramPLCMode         {"plcMode", 1};
    inline const juce::ParameterID paramMaxHoldMs       {"maxHoldMs", 1};
    inline const juce::ParameterID paramEarbudDesync    {"earbudDesync", 1};
    inline const juce::ParameterID paramGlitchTrigger   {"glitchTrigger", 1};
    inline const juce::ParameterID paramHeadphoneType   {"headphoneType", 1};
    inline const juce::ParameterID paramSealOrDrive     {"sealOrDrive", 1};
    inline const juce::ParameterID paramDryWet          {"dryWet", 1};
    inline const juce::ParameterID paramOutputGain      {"outputGain", 1};

    enum class CodecProfile
    {
        A2DP_SBC = 0,
        AAC_Transient,
        HFP_MSBC_16k,
        HFP_CVSD_8k
    };

    enum class PLCMode
    {
        RobotFreeze = 0,
        FrameRepeat,
        HardSquelch
    };

    enum class HeadphoneType
    {
        Flat = 0,
        CheapEarbuds,
        OverEar,
        BoneConduction
    };

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // RF / Physical Link
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramDistance, "Distance (RSSI)", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.25f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramObstacle, "Obstruction / Wall", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.10f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramInterference, "2.4 GHz Clashing (Microwave)", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        // Codec Section
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            paramCodecProfile, "Codec Profile",
            juce::StringArray{"A2DP SBC (Subband)", "AAC Ringing", "HFP mSBC (16 kHz)", "HFP CVSD (8 kHz)"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            paramBitpool, "SBC Bitpool", 2, 53, 35));

        // Jitter & Timing Resync
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramJitterDrift, "Jitter Pitch Warble", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramBufferSizeMs, "Jitter Buffer Size", juce::NormalisableRange<float>(10.0f, 250.0f, 1.0f), 45.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        // Packet Loss Concealment (PLC)
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            paramPLCMode, "PLC Concealment Mode",
            juce::StringArray{"Robot Freeze (PSOLA)", "Frame Repeat", "Hard Squelch"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramMaxHoldMs, "Max PLC Drone Duration", juce::NormalisableRange<float>(20.0f, 1000.0f, 5.0f), 250.0f,
            juce::AudioParameterFloatAttributes().withLabel("ms")));

        // Spatial / True Wireless
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramEarbudDesync, "Earbud Slip (TWS Desync)", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.20f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            paramGlitchTrigger, "Antenna Touch (Instant Drop)", false));

        // Headphone Acoustics
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            paramHeadphoneType, "Headphone Type",
            juce::StringArray{"Flat / Neutral", "Cheap In-Ear Earbuds", "Consumer Over-Ear", "Bone Conduction"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramSealOrDrive, "Ear Seal / Bone Buzz", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.50f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        // Master Output
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramDryWet, "Dry / Wet", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f,
            juce::AudioParameterFloatAttributes().withLabel("%")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            paramOutputGain, "Output Gain", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        return { params.begin(), params.end() };
    }
}
