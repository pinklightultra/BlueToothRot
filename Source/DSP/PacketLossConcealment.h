#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "Parameters.h"

namespace BlueToothDSP
{
    class PacketLossConcealment
    {
    public:
        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            historyLength = static_cast<int>(0.08 * sampleRate); // 80ms history
            history[0].assign(historyLength, 0.0f);
            history[1].assign(historyLength, 0.0f);
            historyWriteIndex[0] = 0;
            historyWriteIndex[1] = 0;

            pitchPeriod[0] = static_cast<int>(0.01 * sampleRate); // Default 100 Hz
            pitchPeriod[1] = static_cast<int>(0.01 * sampleRate);
            freezePhase[0] = 0;
            freezePhase[1] = 0;
            consecutiveLostFrames[0] = 0;
            consecutiveLostFrames[1] = 0;
        }

        void reset()
        {
            for (auto& h : history)
                std::fill(h.begin(), h.end(), 0.0f);
            historyWriteIndex[0] = 0;
            historyWriteIndex[1] = 0;
            consecutiveLostFrames[0] = 0;
            consecutiveLostFrames[1] = 0;
        }

        // Stores a known good sample into history
        void recordGoodSample(int channel, float sample)
        {
            int ch = channel % 2;
            history[ch][historyWriteIndex[ch]] = sample;
            historyWriteIndex[ch] = (historyWriteIndex[ch] + 1) % historyLength;
            consecutiveLostFrames[ch] = 0;
            freezePhase[ch] = 0;
        }

        // Called when a packet loss begins: estimates fundamental pitch period
        void onPacketLossStarted(int channel)
        {
            int ch = channel % 2;
            consecutiveLostFrames[ch]++;

            // Find pitch period tau via auto-correlation over the last 40ms
            int minLag = static_cast<int>(currentSampleRate / 600.0); // 600 Hz max
            int maxLag = static_cast<int>(currentSampleRate / 50.0);  // 50 Hz min
            maxLag = std::min(maxLag, historyLength / 2);

            int bestLag = minLag;
            float maxCorr = -1.0f;

            int head = (historyWriteIndex[ch] - 1 + historyLength) % historyLength;

            for (int lag = minLag; lag < maxLag; lag += 2)
            {
                float corr = 0.0f;
                float normA = 0.0f;
                float normB = 0.0f;

                for (int n = 0; n < 256; ++n)
                {
                    int idxA = (head - n + historyLength) % historyLength;
                    int idxB = (head - n - lag + historyLength) % historyLength;
                    float sA = history[ch][idxA];
                    float sB = history[ch][idxB];

                    corr += sA * sB;
                    normA += sA * sA;
                    normB += sB * sB;
                }

                float denom = std::sqrt(normA * normB) + 1e-6f;
                float normCorr = corr / denom;

                if (normCorr > maxCorr)
                {
                    maxCorr = normCorr;
                    bestLag = lag;
                }
            }

            pitchPeriod[ch] = std::max(minLag, bestLag);
            freezePhase[ch] = 0;
        }

        // Generates a concealed replacement sample for a dropped frame
        float generateConcealedSample(int channel, PLCMode mode, float maxHoldMs)
        {
            int ch = channel % 2;
            int head = (historyWriteIndex[ch] - 1 + historyLength) % historyLength;

            // Maximum samples before squelch/mute takes over
            float maxHoldSamples = (maxHoldMs * 0.001f) * static_cast<float>(currentSampleRate);
            float age = static_cast<float>(freezePhase[ch]);
            float decay = std::clamp(1.0f - (age / maxHoldSamples), 0.0f, 1.0f);

            if (decay <= 0.0f)
                return 0.0f;

            float out = 0.0f;

            switch (mode)
            {
                case PLCMode::RobotFreeze:
                {
                    // Loop single fundamental pitch period with crossfading
                    int period = pitchPeriod[ch];
                    int offset = freezePhase[ch] % period;
                    int readIdx = (head - offset + historyLength) % historyLength;
                    out = history[ch][readIdx] * decay;
                    break;
                }

                case PLCMode::FrameRepeat:
                {
                    // Repeat last ~256 sample frame with progressive attenuation
                    int frameSize = 256;
                    int offset = freezePhase[ch] % frameSize;
                    int readIdx = (head - offset + historyLength) % historyLength;
                    float frameDecay = std::pow(0.88f, static_cast<float>(freezePhase[ch] / frameSize));
                    out = history[ch][readIdx] * frameDecay;
                    break;
                }

                case PLCMode::HardSquelch:
                {
                    // Instant 2ms fast fade to silence
                    int fadeLen = static_cast<int>(0.002 * currentSampleRate);
                    if (freezePhase[ch] < fadeLen)
                    {
                        float ramp = 1.0f - (static_cast<float>(freezePhase[ch]) / static_cast<float>(fadeLen));
                        int readIdx = (head - freezePhase[ch] + historyLength) % historyLength;
                        out = history[ch][readIdx] * ramp;
                    }
                    else
                    {
                        out = 0.0f;
                    }
                    break;
                }
            }

            freezePhase[ch]++;
            return out;
        }

    private:
        double currentSampleRate = 44100.0;
        int historyLength = 4096;
        std::vector<float> history[2];
        int historyWriteIndex[2] = { 0, 0 };
        int pitchPeriod[2] = { 200, 200 };
        int freezePhase[2] = { 0, 0 };
        int consecutiveLostFrames[2] = { 0, 0 };
    };
}
