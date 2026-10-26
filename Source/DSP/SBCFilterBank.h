#pragma once

#include <vector>
#include <cmath>
#include <array>
#include <algorithm>

namespace BlueToothDSP
{
    // Emulates the 8-band polyphase analysis/synthesis filter bank and dynamic bitpool starvation of the A2DP SBC codec
    class SBCFilterBank
    {
    public:
        SBCFilterBank()
        {
            initFilterBank();
        }

        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            reset();
        }

        void reset()
        {
            for (auto& s : subbandHistory)
                std::fill(s.begin(), s.end(), 0.0f);
        }

        // Processes an audio sample through the 8-band SBC subband quantization engine
        float processSample(int channel, float inputSample, int bitpool)
        {
            int ch = channel % 2;

            // 8-band state-variable / polyphase subband approximation
            // Decompose incoming sample into 8 logarithmically/linearly spaced subbands
            std::array<float, 8> bands;
            splitSubbands(ch, inputSample, bands);

            // Compute bit allocation according to SBC bitpool (ranges from 2 to 53)
            // Bitpool is distributed across subbands starting from low-frequency bands
            std::array<int, 8> bitAllocation = calculateBitAllocation(bitpool);

            // Quantize each subband according to its assigned bit budget
            float reconstructed = 0.0f;
            for (int b = 0; b < 8; ++b)
            {
                int bits = bitAllocation[b];
                if (bits <= 0)
                {
                    // Band is dropped / starved
                    bands[b] = 0.0f;
                }
                else
                {
                    // Mid-tread uniform quantizer scaled to subband envelope
                    float levels = std::pow(2.0f, static_cast<float>(bits)) - 1.0f;
                    float absVal = std::abs(bands[b]);
                    float scale = std::max(0.01f, absVal);

                    float normalized = std::clamp(bands[b] / scale, -1.0f, 1.0f);
                    float q = std::round(normalized * (levels * 0.5f)) / (levels * 0.5f);
                    bands[b] = q * scale;
                }

                reconstructed += bands[b];
            }

            return reconstructed * 0.75f;
        }

    private:
        double currentSampleRate = 44100.0;
        std::array<std::array<float, 8>, 2> subbandHistory{};
        std::array<float, 8> bandFrequencies{ 200.0f, 500.0f, 1200.0f, 2500.0f, 5000.0f, 8000.0f, 12000.0f, 16000.0f };

        void initFilterBank()
        {
            for (auto& ch : subbandHistory)
                ch.fill(0.0f);
        }

        void splitSubbands(int ch, float in, std::array<float, 8>& bands)
        {
            // 8-band crossover ladder network
            float current = in;
            for (int b = 0; b < 7; ++b)
            {
                float alpha = std::clamp(static_cast<float>(2.0 * 3.141592653589793 * bandFrequencies[b] / currentSampleRate), 0.001f, 0.99f);
                subbandHistory[ch][b] += alpha * (current - subbandHistory[ch][b]);
                bands[b] = subbandHistory[ch][b];
                current -= bands[b];
            }
            bands[7] = current;
        }

        std::array<int, 8> calculateBitAllocation(int bitpool)
        {
            std::array<int, 8> bits{};
            bits.fill(0);

            // Real SBC bit allocation: Subbands compete for bitpool.
            // Higher frequencies are starved first when bitpool < 35
            int remainingBits = bitpool;

            // Base allocation to low/mid bands
            for (int b = 0; b < 8; ++b)
            {
                int priorityWeight = 8 - b; // Lower bands have higher priority
                int assigned = std::clamp(remainingBits / priorityWeight, 0, 16);

                if (b >= 5 && bitpool < 28)
                    assigned = 0; // Cut off high frequencies entirely
                if (b >= 3 && bitpool < 14)
                    assigned = 0;

                bits[b] = assigned;
                remainingBits -= assigned;
                if (remainingBits <= 0)
                    break;
            }

            return bits;
        }
    };
}
