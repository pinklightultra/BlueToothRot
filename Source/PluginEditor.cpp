#include "PluginEditor.h"

BlueToothRotAudioProcessorEditor::BlueToothRotAudioProcessorEditor(BlueToothRotAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(720, 510);

    // Setup Knobs
    setupRotarySlider(distanceSlider, distanceLabel, "DISTANCE (RSSI)");
    setupRotarySlider(obstacleSlider, obstacleLabel, "OBSTACLE / WALL");
    setupRotarySlider(interferenceSlider, interferenceLabel, "2.4G CLASH");
    setupRotarySlider(bitpoolSlider, bitpoolLabel, "SBC BITPOOL");
    setupRotarySlider(jitterDriftSlider, jitterDriftLabel, "WARBLE DRIFT");
    setupRotarySlider(bufferSizeSlider, bufferSizeLabel, "JITTER BUFFER");
    setupRotarySlider(maxHoldSlider, maxHoldLabel, "MAX DRONE");
    setupRotarySlider(earbudDesyncSlider, earbudDesyncLabel, "TWS EARBUD SLIP");
    setupRotarySlider(sealDriveSlider, sealDriveLabel, "SEAL / BUZZ");
    setupRotarySlider(dryWetSlider, dryWetLabel, "DRY / WET");
    setupRotarySlider(outputGainSlider, outputGainLabel, "OUTPUT GAIN");

    // Codec ComboBox
    codecBox.addItemList(audioProcessor.getAPVTS().getParameter(BlueToothDSP::paramCodecProfile.getParamID())->getAllValueStrings(), 1);
    addAndMakeVisible(codecBox);
    codecLabel.setText("CODEC PROFILE", juce::dontSendNotification);
    codecLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    codecLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(codecLabel);

    // PLC Mode ComboBox
    plcBox.addItemList(audioProcessor.getAPVTS().getParameter(BlueToothDSP::paramPLCMode.getParamID())->getAllValueStrings(), 1);
    addAndMakeVisible(plcBox);
    plcLabel.setText("PLC CONCEALMENT", juce::dontSendNotification);
    plcLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    plcLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(plcLabel);

    // Headphone Type ComboBox
    headphoneBox.addItemList(audioProcessor.getAPVTS().getParameter(BlueToothDSP::paramHeadphoneType.getParamID())->getAllValueStrings(), 1);
    addAndMakeVisible(headphoneBox);
    headphoneLabel.setText("HEADPHONE ACOUSTICS", juce::dontSendNotification);
    headphoneLabel.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    headphoneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(headphoneLabel);

    // Antenna Touch Button
    glitchButton.setButtonText("ANTENNA TOUCH");
    glitchButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff8a1a1a));
    glitchButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff2222));
    addAndMakeVisible(glitchButton);

    // Attachments
    auto& apvts = audioProcessor.getAPVTS();
    distanceAttachment     = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramDistance.getParamID(), distanceSlider);
    obstacleAttachment     = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramObstacle.getParamID(), obstacleSlider);
    interferenceAttachment = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramInterference.getParamID(), interferenceSlider);
    codecAttachment        = std::make_unique<ComboBoxAttachment>(apvts, BlueToothDSP::paramCodecProfile.getParamID(), codecBox);
    bitpoolAttachment      = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramBitpool.getParamID(), bitpoolSlider);
    jitterDriftAttachment  = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramJitterDrift.getParamID(), jitterDriftSlider);
    bufferSizeAttachment   = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramBufferSizeMs.getParamID(), bufferSizeSlider);
    plcAttachment          = std::make_unique<ComboBoxAttachment>(apvts, BlueToothDSP::paramPLCMode.getParamID(), plcBox);
    maxHoldAttachment      = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramMaxHoldMs.getParamID(), maxHoldSlider);
    earbudDesyncAttachment = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramEarbudDesync.getParamID(), earbudDesyncSlider);
    headphoneAttachment    = std::make_unique<ComboBoxAttachment>(apvts, BlueToothDSP::paramHeadphoneType.getParamID(), headphoneBox);
    sealDriveAttachment    = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramSealOrDrive.getParamID(), sealDriveSlider);
    glitchAttachment       = std::make_unique<ButtonAttachment>(apvts, BlueToothDSP::paramGlitchTrigger.getParamID(), glitchButton);
    dryWetAttachment       = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramDryWet.getParamID(), dryWetSlider);
    outputGainAttachment   = std::make_unique<SliderAttachment>(apvts, BlueToothDSP::paramOutputGain.getParamID(), outputGainSlider);

    startTimerHz(30);
}

BlueToothRotAudioProcessorEditor::~BlueToothRotAudioProcessorEditor()
{
    stopTimer();
}

void BlueToothRotAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d4ff));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffa0b0c0));
    addAndMakeVisible(label);
}

void BlueToothRotAudioProcessorEditor::timerCallback()
{
    repaint();
}

void BlueToothRotAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient: dark futuristic slate navy
    g.fillAll(juce::Colour(0xff0d131a));

    // Header bar
    g.setColour(juce::Colour(0xff141d27));
    g.fillRect(0, 0, getWidth(), 50);

    // Logo & title
    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::FontOptions(20.0f).withStyle("Bold"));
    g.drawText("BLUETOOTH-ROT", 20, 10, 200, 30, juce::Justification::left);

    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colour(0xff688096));
    g.drawText("2.4 GHz LINK DEGRADATION & ACOUSTIC ROT", 185, 12, 280, 28, juce::Justification::left);

    // RSSI Signal Bars
    float quality = audioProcessor.getLinkQuality();
    int activeBars = static_cast<int>(std::round(quality * 4.0f));

    int barX = getWidth() - 170;
    for (int b = 0; b < 4; ++b)
    {
        int h = 6 + b * 5;
        int y = 34 - h;
        bool lit = b < activeBars;

        g.setColour(lit ? juce::Colour(0xff00e5ff) : juce::Colour(0xff253545));
        g.fillRect(barX + b * 10, y, 6, h);
    }

    // Packet Loss Status LED
    bool isDropping = audioProcessor.isChannelDropping(0) || audioProcessor.isChannelDropping(1);
    g.setColour(isDropping ? juce::Colour(0xffff2233) : juce::Colour(0xff00e588));
    g.fillEllipse(static_cast<float>(barX + 50), 20.0f, 10.0f, 10.0f);

    g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
    g.drawText(isDropping ? "PACKET LOSS" : "LINK STABLE", barX + 66, 17, 80, 16, juce::Justification::left);

    // Section outlines / borders
    auto drawSectionBox = [&](int x, int y, int w, int h, const juce::String& title)
    {
        g.setColour(juce::Colour(0xff182230));
        g.drawRoundedRectangle(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h), 6.0f, 1.5f);

        g.setColour(juce::Colour(0xff00d4ff));
        g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
        g.drawText(title, x + 12, y + 6, w - 24, 18, juce::Justification::left);
    };

    drawSectionBox(15, 60, 215, 200, "1. RF LINK & DISTANCE");
    drawSectionBox(240, 60, 230, 200, "2. CODEC & BITPOOL");
    drawSectionBox(480, 60, 225, 200, "3. JITTER BUFFER (WARBLE)");

    drawSectionBox(15, 275, 215, 215, "4. PACKET LOSS CONCEALMENT");
    drawSectionBox(240, 275, 230, 215, "5. HEADPHONE ACOUSTICS & TWS");
    drawSectionBox(480, 275, 225, 215, "6. MASTER & TOUCH");
}

void BlueToothRotAudioProcessorEditor::resized()
{
    // Section 1: RF Link
    distanceSlider.setBounds(25, 95, 90, 90);
    distanceLabel.setBounds(25, 185, 90, 16);

    obstacleSlider.setBounds(125, 95, 90, 90);
    obstacleLabel.setBounds(125, 185, 90, 16);

    interferenceSlider.setBounds(75, 205, 90, 50);
    interferenceLabel.setBounds(65, 240, 110, 16);

    // Section 2: Codec
    codecBox.setBounds(255, 95, 200, 25);
    codecLabel.setBounds(255, 122, 200, 16);

    bitpoolSlider.setBounds(310, 145, 90, 90);
    bitpoolLabel.setBounds(310, 235, 90, 16);

    // Section 3: Jitter Buffer
    jitterDriftSlider.setBounds(490, 95, 95, 95);
    jitterDriftLabel.setBounds(490, 190, 95, 16);

    bufferSizeSlider.setBounds(595, 95, 95, 95);
    bufferSizeLabel.setBounds(595, 190, 95, 16);

    // Section 4: PLC
    plcBox.setBounds(30, 305, 185, 25);
    plcLabel.setBounds(30, 332, 185, 16);

    maxHoldSlider.setBounds(75, 360, 95, 95);
    maxHoldLabel.setBounds(75, 455, 95, 16);

    // Section 5: Headphone Acoustics & TWS
    headphoneBox.setBounds(255, 305, 200, 25);
    headphoneLabel.setBounds(255, 332, 200, 16);

    earbudDesyncSlider.setBounds(255, 355, 85, 85);
    earbudDesyncLabel.setBounds(250, 440, 95, 16);

    sealDriveSlider.setBounds(365, 355, 85, 85);
    sealDriveLabel.setBounds(360, 440, 95, 16);

    // Section 6: Master
    dryWetSlider.setBounds(490, 310, 90, 85);
    dryWetLabel.setBounds(490, 395, 90, 16);

    outputGainSlider.setBounds(600, 310, 90, 85);
    outputGainLabel.setBounds(600, 395, 90, 16);

    glitchButton.setBounds(500, 435, 185, 36);
}
