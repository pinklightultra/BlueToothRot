#include "PluginProcessor.h"
#include "PluginEditor.h"

BlueToothRotAudioProcessor::BlueToothRotAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", BlueToothDSP::createParameterLayout())
{
}

BlueToothRotAudioProcessor::~BlueToothRotAudioProcessor()
{
}

const juce::String BlueToothRotAudioProcessor::getName() const
{
    return "BlueToothRot";
}

bool BlueToothRotAudioProcessor::acceptsMidi() const { return false; }
bool BlueToothRotAudioProcessor::producesMidi() const { return false; }
bool BlueToothRotAudioProcessor::isMidiEffect() const { return false; }
double BlueToothRotAudioProcessor::getTailLengthSeconds() const { return 0.2; }
int BlueToothRotAudioProcessor::getNumPrograms() { return 1; }
int BlueToothRotAudioProcessor::getCurrentProgram() { return 0; }
void BlueToothRotAudioProcessor::setCurrentProgram(int) {}
const juce::String BlueToothRotAudioProcessor::getProgramName(int) { return {}; }
void BlueToothRotAudioProcessor::changeProgramName(int, const juce::String&) {}

void BlueToothRotAudioProcessor::prepareToPlay(double sampleRate, int)
{
    rfEngine.prepare(sampleRate);
    jitterBuffer.prepare(sampleRate, 400);
    plcEngine.prepare(sampleRate);
    sbcFilter.prepare(sampleRate);
    hfpEngine.prepare(sampleRate);
    desyncEngine.prepare(sampleRate);
    headphoneEngine.prepare(sampleRate);
}

void BlueToothRotAudioProcessor::releaseResources()
{
    jitterBuffer.reset();
    plcEngine.reset();
    sbcFilter.reset();
    hfpEngine.reset();
    desyncEngine.reset();
    headphoneEngine.reset();
}

bool BlueToothRotAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void BlueToothRotAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0)
        return;

    // Fetch Parameter Values
    const float distance       = apvts.getRawParameterValue(BlueToothDSP::paramDistance.getParamID())->load();
    const float obstacle       = apvts.getRawParameterValue(BlueToothDSP::paramObstacle.getParamID())->load();
    const float interference   = apvts.getRawParameterValue(BlueToothDSP::paramInterference.getParamID())->load();
    const int   codecIdx       = static_cast<int>(apvts.getRawParameterValue(BlueToothDSP::paramCodecProfile.getParamID())->load());
    const int   bitpool        = static_cast<int>(apvts.getRawParameterValue(BlueToothDSP::paramBitpool.getParamID())->load());
    const float jitterDrift    = apvts.getRawParameterValue(BlueToothDSP::paramJitterDrift.getParamID())->load();
    const float bufferSizeMs   = apvts.getRawParameterValue(BlueToothDSP::paramBufferSizeMs.getParamID())->load();
    const int   plcModeIdx     = static_cast<int>(apvts.getRawParameterValue(BlueToothDSP::paramPLCMode.getParamID())->load());
    const float maxHoldMs      = apvts.getRawParameterValue(BlueToothDSP::paramMaxHoldMs.getParamID())->load();
    const float earbudDesync   = apvts.getRawParameterValue(BlueToothDSP::paramEarbudDesync.getParamID())->load();
    const bool  glitchTrigger  = apvts.getRawParameterValue(BlueToothDSP::paramGlitchTrigger.getParamID())->load() > 0.5f;
    const int   headphoneIdx   = static_cast<int>(apvts.getRawParameterValue(BlueToothDSP::paramHeadphoneType.getParamID())->load());
    const float sealOrDrive    = apvts.getRawParameterValue(BlueToothDSP::paramSealOrDrive.getParamID())->load();
    const float dryWet         = apvts.getRawParameterValue(BlueToothDSP::paramDryWet.getParamID())->load();
    const float outputGainDb   = apvts.getRawParameterValue(BlueToothDSP::paramOutputGain.getParamID())->load();

    const float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDb);
    auto codecProfile  = static_cast<BlueToothDSP::CodecProfile>(codecIdx);
    auto plcMode       = static_cast<BlueToothDSP::PLCMode>(plcModeIdx);
    auto headphoneType = static_cast<BlueToothDSP::HeadphoneType>(headphoneIdx);

    // Update real-time link quality telemetry for GUI (1.0 = perfect, 0.0 = total blackout)
    float qualityEstimate = 1.0f - (distance * 0.5f + obstacle * 0.4f + interference * 0.3f);
    currentLinkQuality.store(std::clamp(qualityEstimate, 0.0f, 1.0f));

    // Process in audio frames / packets (typical Bluetooth packet size ~128 samples / ~3ms)
    const int packetSize = 128;
    int samplesProcessed = 0;

    while (samplesProcessed < numSamples)
    {
        int chunkLen = std::min(packetSize, numSamples - samplesProcessed);

        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            // Evaluate RF Packet Drop for this chunk
            bool isDropped = rfEngine.shouldDropFrame(ch, distance, obstacle, interference, earbudDesync, glitchTrigger);
            currentDropState[ch % 2].store(isDropped);

            if (isDropped)
                plcEngine.onPacketLossStarted(ch);

            auto* channelData = buffer.getWritePointer(ch);

            for (int s = 0; s < chunkLen; ++s)
            {
                int sampleIdx = samplesProcessed + s;
                float inSample = channelData[sampleIdx];
                float processedSample = 0.0f;

                if (isDropped)
                {
                    // Generate Packet Loss Concealment (robotic pitch-freeze, repeat, or squelch)
                    processedSample = plcEngine.generateConcealedSample(ch, plcMode, maxHoldMs);
                }
                else
                {
                    // Codec Processing
                    switch (codecProfile)
                    {
                        case BlueToothDSP::CodecProfile::A2DP_SBC:
                            processedSample = sbcFilter.processSample(ch, inSample, bitpool);
                            break;

                        case BlueToothDSP::CodecProfile::AAC_Transient:
                            processedSample = inSample * (1.0f + 0.08f * std::sin(sampleIdx * 0.4f));
                            break;

                        case BlueToothDSP::CodecProfile::HFP_MSBC_16k:
                            processedSample = hfpEngine.processMSBC(ch, inSample);
                            break;

                        case BlueToothDSP::CodecProfile::HFP_CVSD_8k:
                            processedSample = hfpEngine.processCVSD(ch, inSample);
                            break;
                    }

                    plcEngine.recordGoodSample(ch, processedSample);
                }

                // Push to Jitter Buffer
                jitterBuffer.pushSample(ch, processedSample);

                // Pull from Jitter Buffer with dynamic ASRC pitch warble
                float jittered = jitterBuffer.pullSample(ch, bufferSizeMs, jitterDrift);

                // Apply True Wireless Stereo slave channel desynchronization
                if (ch == 1)
                    jittered = desyncEngine.processSlaveChannel(jittered, earbudDesync);

                // Dry / Wet & Output gain
                float finalSample = ((1.0f - dryWet) * inSample + dryWet * jittered) * outputGainLinear;
                channelData[sampleIdx] = std::clamp(finalSample, -1.5f, 1.5f);
            }
        }

        samplesProcessed += chunkLen;
    }

    // Apply Headphone Transducer Acoustics (Earbuds, Over-Ears, Bone Conduction)
    if (totalNumInputChannels >= 2)
    {
        headphoneEngine.processBlock(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples, headphoneType, sealOrDrive);
    }
    else if (totalNumInputChannels == 1)
    {
        headphoneEngine.processBlock(buffer.getWritePointer(0), buffer.getWritePointer(0), numSamples, headphoneType, sealOrDrive);
    }
}

bool BlueToothRotAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* BlueToothRotAudioProcessor::createEditor()
{
    return new BlueToothRotAudioProcessorEditor(*this);
}

void BlueToothRotAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BlueToothRotAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueToothRotAudioProcessor();
}
