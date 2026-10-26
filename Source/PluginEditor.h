#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class BlueToothRotAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit BlueToothRotAudioProcessorEditor(BlueToothRotAudioProcessor&);
    ~BlueToothRotAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    BlueToothRotAudioProcessor& audioProcessor;

    // Sliders & ComboBoxes
    juce::Slider distanceSlider;
    juce::Slider obstacleSlider;
    juce::Slider interferenceSlider;
    juce::ComboBox codecBox;
    juce::Slider bitpoolSlider;
    juce::Slider jitterDriftSlider;
    juce::Slider bufferSizeSlider;
    juce::ComboBox plcBox;
    juce::Slider maxHoldSlider;
    juce::Slider earbudDesyncSlider;
    juce::ComboBox headphoneBox;
    juce::Slider sealDriveSlider;
    juce::TextButton glitchButton;
    juce::Slider dryWetSlider;
    juce::Slider outputGainSlider;

    // Labels
    juce::Label distanceLabel;
    juce::Label obstacleLabel;
    juce::Label interferenceLabel;
    juce::Label codecLabel;
    juce::Label bitpoolLabel;
    juce::Label jitterDriftLabel;
    juce::Label bufferSizeLabel;
    juce::Label plcLabel;
    juce::Label maxHoldLabel;
    juce::Label earbudDesyncLabel;
    juce::Label headphoneLabel;
    juce::Label sealDriveLabel;
    juce::Label dryWetLabel;
    juce::Label outputGainLabel;

    // Attachments
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment>   distanceAttachment;
    std::unique_ptr<SliderAttachment>   obstacleAttachment;
    std::unique_ptr<SliderAttachment>   interferenceAttachment;
    std::unique_ptr<ComboBoxAttachment> codecAttachment;
    std::unique_ptr<SliderAttachment>   bitpoolAttachment;
    std::unique_ptr<SliderAttachment>   jitterDriftAttachment;
    std::unique_ptr<SliderAttachment>   bufferSizeAttachment;
    std::unique_ptr<ComboBoxAttachment> plcAttachment;
    std::unique_ptr<SliderAttachment>   maxHoldAttachment;
    std::unique_ptr<SliderAttachment>   earbudDesyncAttachment;
    std::unique_ptr<ComboBoxAttachment> headphoneAttachment;
    std::unique_ptr<SliderAttachment>   sealDriveAttachment;
    std::unique_ptr<ButtonAttachment>   glitchAttachment;
    std::unique_ptr<SliderAttachment>   dryWetAttachment;
    std::unique_ptr<SliderAttachment>   outputGainAttachment;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlueToothRotAudioProcessorEditor)
};
