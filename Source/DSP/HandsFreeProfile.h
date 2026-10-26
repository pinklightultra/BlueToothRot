#pragma once

#include <cmath>
#include <algorithm>

namespace BlueToothDSP
{
    // Emulates Hands-Free Profile (HFP 1.5 CVSD 8 kHz & HFP 1.6 mSBC 16 kHz) telephone modes
    class HandsFreeProfile
    {
    public:
        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            reset();
        }

        void reset()
        {
            for (int i = 0; i < 2; ++i)
            {
                hpState[i] = 0.0f;
                lpState[i] = 0.0f;
                accumulator[i] = 0.0f;
                stepSize[i] = minStep;
                historyBits[i] = 0;
            }
        }

        // Processes audio sample through CVSD (8 kHz telephone)
        float processCVSD(int channel, float sample)
        {
            int ch = channel % 2;

            // 1. Telecom Bandpass Filter (300 Hz - 3400 Hz)
            float filtered = applyBandpass(ch, sample, 300.0f, 3400.0f);

            // 2. CVSD 1-Bit Delta Modulation with Syllabic Companding
            // Check sign of error: input vs accumulator
            bool bit = filtered > accumulator[ch];

            // Update shift register of last 4 bits
            historyBits[ch] = ((historyBits[ch] << 1) | (bit ? 1 : 0)) & 0x0F;

            // Syllabic compander: if 4 consecutive ones or zeros occur, slope overload is detected
            if (historyBits[ch] == 0x0F || historyBits[ch] == 0x00)
            {
                stepSize[ch] = std::min(maxStep, stepSize[ch] * 1.45f); // Expand step size
            }
            else
            {
                stepSize[ch] = std::max(minStep, stepSize[ch] * 0.92f); // Syllabic decay
            }

            // Accumulator tracking
            if (bit)
                accumulator[ch] += stepSize[ch];
            else
                accumulator[ch] -= stepSize[ch];

            // Leaky integration to prevent DC runaway
            accumulator[ch] *= 0.96f;

            // Output reconstructed signal through post-filter
            return accumulator[ch] * 0.85f;
        }

        // Processes audio sample through mSBC (16 kHz wideband speech)
        float processMSBC(int channel, float sample)
        {
            int ch = channel % 2;
            // 50 Hz - 7000 Hz Wideband speech filter with mild saturation
            float filtered = applyBandpass(ch, sample, 80.0f, 7000.0f);
            return std::tanh(filtered * 1.25f);
        }

    private:
        double currentSampleRate = 44100.0;

        float hpState[2] = { 0.0f, 0.0f };
        float lpState[2] = { 0.0f, 0.0f };

        // CVSD state
        float accumulator[2] = { 0.0f, 0.0f };
        float stepSize[2] = { 0.02f, 0.02f };
        int historyBits[2] = { 0, 0 };

        const float minStep = 0.005f;
        const float maxStep = 0.45f;

        float applyBandpass(int ch, float in, float lowCut, float highCut)
        {
            // Simple 1-pole HP + 1-pole LP
            float alphaHP = std::clamp(static_cast<float>(2.0 * 3.141592653589793 * lowCut / currentSampleRate), 0.001f, 0.95f);
            hpState[ch] += alphaHP * (in - hpState[ch]);
            float highPassed = in - hpState[ch];

            float alphaLP = std::clamp(static_cast<float>(2.0 * 3.141592653589793 * highCut / currentSampleRate), 0.001f, 0.95f);
            lpState[ch] += alphaLP * (highPassed - lpState[ch]);
            return lpState[ch];
        }
    };
}
