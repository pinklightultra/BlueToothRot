#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

namespace BlueToothDSP
{
    // Emulates True Wireless Earbuds (TWS) master-slave cross-head link slipping
    class StereoDesyncEngine
    {
    public:
        StereoDesyncEngine()
            : rng(42), dist01(0.0f, 1.0f)
        {
        }

        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            maxDelaySamples = static_cast<int>(0.05 * sampleRate); // 50ms max slip
            delayLine.assign(maxDelaySamples, 0.0f);
            writeIdx = 0;
            currentDelay = 0.0f;
            targetDelay = 0.0f;
        }

        void reset()
        {
            std::fill(delayLine.begin(), delayLine.end(), 0.0f);
            writeIdx = 0;
            currentDelay = 0.0f;
            targetDelay = 0.0f;
        }

        // Processes Channel 1 (Secondary / Slave earbud) with micro-delay jitter and desync slip
        float processSlaveChannel(float inSample, float earbudDesyncAmount)
        {
            if (earbudDesyncAmount < 0.01f)
                return inSample;

            // Push sample into slave delay line
            delayLine[writeIdx] = inSample;

            // Randomly update slip target to simulate earbud clock drift & relay lag
            if (dist01(rng) < 0.0005f)
            {
                float slipMs = dist01(rng) * 35.0f * earbudDesyncAmount;
                targetDelay = (slipMs * 0.001f) * static_cast<float>(currentSampleRate);
            }

            // Smoothly track target delay
            currentDelay += (targetDelay - currentDelay) * 0.002f;
            currentDelay = std::clamp(currentDelay, 0.0f, static_cast<float>(maxDelaySamples - 2));

            // Fractional read pointer
            float readPos = static_cast<float>(writeIdx) - currentDelay;
            if (readPos < 0.0f)
                readPos += static_cast<float>(maxDelaySamples);

            int i0 = static_cast<int>(std::floor(readPos));
            int i1 = (i0 + 1) % maxDelaySamples;
            float frac = readPos - static_cast<float>(i0);

            float outSample = delayLine[i0] + frac * (delayLine[i1] - delayLine[i0]);

            writeIdx = (writeIdx + 1) % maxDelaySamples;
            return outSample;
        }

    private:
        double currentSampleRate = 44100.0;
        int maxDelaySamples = 2048;
        std::vector<float> delayLine;
        int writeIdx = 0;
        float currentDelay = 0.0f;
        float targetDelay = 0.0f;

        std::mt19937 rng;
        std::uniform_real_distribution<float> dist01;
    };
}
