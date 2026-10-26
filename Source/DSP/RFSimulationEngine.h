#pragma once

#include <cmath>
#include <random>

namespace BlueToothDSP
{
    class RFSimulationEngine
    {
    public:
        RFSimulationEngine()
            : rng(1337), dist01(0.0f, 1.0f)
        {
        }

        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            interferencePhase = 0.0;
            stateGood[0] = true;
            stateGood[1] = true;
        }

        // Evaluates packet loss for an audio frame (e.g., 5ms - 15ms block)
        // channel: 0 = Master / Left, 1 = Slave / Right
        bool shouldDropFrame(int channel,
                             float distance,
                             float obstacle,
                             float interference,
                             float earbudDesync,
                             bool forceGlitch)
        {
            if (forceGlitch)
                return true;

            // Physical RSSI decay curve (logarithmic path loss)
            float effectiveDistance = std::clamp(distance, 0.0f, 1.0f);
            float effectiveObstacle = std::clamp(obstacle, 0.0f, 1.0f);

            // Channel 1 (Slave/Secondary earbud) experiences additional human head RF attenuation
            if (channel == 1)
            {
                effectiveDistance += earbudDesync * 0.25f;
                effectiveObstacle += earbudDesync * 0.35f;
            }

            effectiveDistance = std::clamp(effectiveDistance, 0.0f, 1.0f);
            effectiveObstacle = std::clamp(effectiveObstacle, 0.0f, 1.0f);

            // Gilbert-Elliott transition probabilities
            // P(G -> B): Probability of entering burst loss
            float p_GB = 0.002f + (0.28f * std::pow(effectiveDistance, 2.5f)) + (0.45f * effectiveObstacle);
            
            // P(B -> G): Probability of recovering to good link state
            float p_BG = 0.65f - (0.45f * effectiveObstacle);
            p_BG = std::clamp(p_BG, 0.05f, 0.95f);

            // 2.4 GHz Microwave / Wi-Fi channel collision envelope
            // Microwave leakage occurs at 2x line frequency (120 Hz in US, 100 Hz in EU)
            float interferenceBurst = 0.0f;
            if (interference > 0.01f)
            {
                interferencePhase += (120.0 / currentSampleRate) * 256.0; // 256 sample frame step
                if (interferencePhase >= 1.0)
                    interferencePhase -= 1.0;

                // Rectified half-sine magnetron pulse with ~50% duty cycle
                float wave = std::sin(interferencePhase * 2.0 * 3.141592653589793);
                if (wave > 0.3f)
                    interferenceBurst = wave * interference;
            }

            // Update Markov State
            bool& isGood = stateGood[channel % 2];
            float randVal = dist01(rng);

            if (isGood)
            {
                if (randVal < (p_GB + (interferenceBurst * 0.7f)))
                    isGood = false;
            }
            else
            {
                if (randVal < p_BG)
                    isGood = true;
            }

            // In Bad state, packet loss rate is ~85%. In Good state, background residual loss is ~0.4%
            float lossProbability = isGood ? 0.004f : 0.88f;

            if (interferenceBurst > 0.5f)
                lossProbability = std::max(lossProbability, 0.95f);

            return dist01(rng) < lossProbability;
        }

    private:
        double currentSampleRate = 44100.0;
        double interferencePhase = 0.0;
        bool stateGood[2] = { true, true };

        std::mt19937 rng;
        std::uniform_real_distribution<float> dist01;
    };
}
