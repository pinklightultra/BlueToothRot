#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "Parameters.h"

namespace BlueToothDSP
{
    // Biquad IIR filter for acoustic shaping
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        void reset() { x1 = x2 = y1 = y2 = 0.0f; }

        float process(float in)
        {
            float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1;
            x1 = in;
            y2 = y1;
            y1 = out;
            return out;
        }

        void setPeaking(double sampleRate, float freq, float gainDb, float Q)
        {
            double w0 = 2.0 * 3.141592653589793 * freq / sampleRate;
            double A = std::pow(10.0, gainDb / 40.0);
            double alpha = std::sin(w0) / (2.0 * Q);

            double a0 = 1.0 + alpha / A;
            b0 = static_cast<float>((1.0 + alpha * A) / a0);
            b1 = static_cast<float>((-2.0 * std::cos(w0)) / a0);
            b2 = static_cast<float>((1.0 - alpha * A) / a0);
            a1 = static_cast<float>((-2.0 * std::cos(w0)) / a0);
            a2 = static_cast<float>((1.0 - alpha / A) / a0);
        }

        void setHighpass(double sampleRate, float freq, float Q = 0.707f)
        {
            double w0 = 2.0 * 3.141592653589793 * freq / sampleRate;
            double alpha = std::sin(w0) / (2.0 * Q);
            double cosw0 = std::cos(w0);

            double a0 = 1.0 + alpha;
            b0 = static_cast<float>(((1.0 + cosw0) * 0.5) / a0);
            b1 = static_cast<float>(-(1.0 + cosw0) / a0);
            b2 = static_cast<float>(((1.0 + cosw0) * 0.5) / a0);
            a1 = static_cast<float>((-2.0 * cosw0) / a0);
            a2 = static_cast<float>((1.0 - alpha) / a0);
        }

        void setLowpass(double sampleRate, float freq, float Q = 0.707f)
        {
            double w0 = 2.0 * 3.141592653589793 * freq / sampleRate;
            double alpha = std::sin(w0) / (2.0 * Q);
            double cosw0 = std::cos(w0);

            double a0 = 1.0 + alpha;
            b0 = static_cast<float>(((1.0 - cosw0) * 0.5) / a0);
            b1 = static_cast<float>((1.0 - cosw0) / a0);
            b2 = static_cast<float>(((1.0 - cosw0) * 0.5) / a0);
            a1 = static_cast<float>((-2.0 * cosw0) / a0);
            a2 = static_cast<float>((1.0 - alpha) / a0);
        }

        void setLowShelf(double sampleRate, float freq, float gainDb)
        {
            double w0 = 2.0 * 3.141592653589793 * freq / sampleRate;
            double A = std::pow(10.0, gainDb / 40.0);
            double alpha = std::sin(w0) / (2.0 * 0.707);
            double cosw0 = std::cos(w0);
            double sqrtA = std::sqrt(A);

            double a0 = (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
            b0 = static_cast<float>((A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha)) / a0);
            b1 = static_cast<float>((2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0)) / a0);
            b2 = static_cast<float>((A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha)) / a0);
            a1 = static_cast<float>((-2.0 * ((A - 1.0) + (A + 1.0) * cosw0)) / a0);
            a2 = static_cast<float>(((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha) / a0);
        }
    };

    class HeadphoneAcoustics
    {
    public:
        void prepare(double sampleRate)
        {
            currentSampleRate = sampleRate;
            reset();
            configureFilters();
        }

        void reset()
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                earbudHP[ch].reset();
                earbudPeak[ch].reset();
                overEarShelf[ch].reset();
                overEarCavity[ch].reset();
                overEarFoam[ch].reset();
                boneHP[ch].reset();
                bonePeak[ch].reset();
                boneLP[ch].reset();
                boneBuzzLP[ch].reset();
            }
        }

        void processBlock(float* left, float* right, int numSamples, HeadphoneType type, float sealOrDrive)
        {
            if (type == HeadphoneType::Flat)
                return;

            for (int s = 0; s < numSamples; ++s)
            {
                float inL = left[s];
                float inR = right[s];

                float outL = inL;
                float outR = inR;

                switch (type)
                {
                    case HeadphoneType::CheapEarbuds:
                    {
                        // 1. Broken seal: variable high-pass cutoff (50Hz tightly sealed down to 220Hz completely loose)
                        float hpFreq = 50.0f + (180.0f * (1.0f - sealOrDrive));
                        earbudHP[0].setHighpass(currentSampleRate, hpFreq, 0.707f);
                        earbudHP[1].setHighpass(currentSampleRate, hpFreq, 0.707f);

                        outL = earbudHP[0].process(inL);
                        outR = earbudHP[1].process(inR);

                        // 2. 3.0 kHz ear-canal acoustic resonance peak
                        outL = earbudPeak[0].process(outL);
                        outR = earbudPeak[1].process(outR);

                        // 3. Cheap micro-dynamic driver edge clipping on loud transients
                        outL = std::clamp(outL * 1.15f, -1.0f, 1.0f);
                        outR = std::clamp(outR * 1.15f, -1.0f, 1.0f);
                        break;
                    }

                    case HeadphoneType::OverEar:
                    {
                        // 1. Boomy consumer bass tuning
                        outL = overEarShelf[0].process(inL);
                        outR = overEarShelf[1].process(inR);

                        // 2. Earcup enclosure cavity resonance (320 Hz boxiness)
                        outL = overEarCavity[0].process(outL);
                        outR = overEarCavity[1].process(outR);

                        // 3. Acoustic damping from foam earpads (gentle lowpass at 11 kHz)
                        outL = overEarFoam[0].process(outL);
                        outR = overEarFoam[1].process(outR);

                        // 4. Subtle interaural earcup leakage / crossfeed (0.07 bleed)
                        float crossL = outL * 0.93f + outR * 0.07f;
                        float crossR = outR * 0.93f + outL * 0.07f;
                        outL = crossL;
                        outR = crossR;
                        break;
                    }

                    case HeadphoneType::BoneConduction:
                    {
                        // Bone conduction physics:
                        // Sound passes through the zygomatic arch and temporal bone directly to the cochlea.
                        // 1. Severe low-end cutoff: human skull bone does not resonate sub-bass (< 140 Hz)
                        outL = boneHP[0].process(inL);
                        outR = boneHP[1].process(inR);

                        // 2. High frequency absorption: skull bone acts as a steep low-pass filter above 4.5 kHz
                        outL = boneLP[0].process(outL);
                        outR = boneLP[1].process(outR);

                        // 3. Speech clarity formant peak (1.8 kHz bone conduction resonance)
                        outL = bonePeak[0].process(outL);
                        outR = bonePeak[1].process(outR);

                        // 4. Tactile transducer buzz: When bass frequencies (80Hz - 250Hz) are present,
                        // the mechanical motor buzzes violently against the cheekbone skin.
                        float buzzL = boneBuzzLP[0].process(inL);
                        float buzzR = boneBuzzLP[1].process(inR);

                        // Generate odd harmonics from mechanical skin displacement
                        float buzzHarmonicsL = std::tanh(buzzL * (3.0f + 5.0f * sealOrDrive));
                        float buzzHarmonicsR = std::tanh(buzzR * (3.0f + 5.0f * sealOrDrive));

                        outL = (outL * 0.85f) + (buzzHarmonicsL * 0.35f * sealOrDrive);
                        outR = (outR * 0.85f) + (buzzHarmonicsR * 0.35f * sealOrDrive);
                        break;
                    }

                    case HeadphoneType::Flat:
                    default:
                        break;
                }

                left[s] = outL;
                right[s] = outR;
            }
        }

    private:
        double currentSampleRate = 44100.0;

        // Cheap Earbud filters
        Biquad earbudHP[2];
        Biquad earbudPeak[2];

        // Over-Ear filters
        Biquad overEarShelf[2];
        Biquad overEarCavity[2];
        Biquad overEarFoam[2];

        // Bone Conduction filters
        Biquad boneHP[2];
        Biquad bonePeak[2];
        Biquad boneLP[2];
        Biquad boneBuzzLP[2];

        void configureFilters()
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                // Earbud: 3.2 kHz resonance (+5 dB, Q=2.2)
                earbudPeak[ch].setPeaking(currentSampleRate, 3200.0f, 5.0f, 2.2f);

                // Over-Ear: 110 Hz bass boost (+4 dB), 320 Hz boxiness (+3 dB, Q=2.5), 11 kHz damping
                overEarShelf[ch].setLowShelf(currentSampleRate, 110.0f, 4.0f);
                overEarCavity[ch].setPeaking(currentSampleRate, 320.0f, 3.0f, 2.5f);
                overEarFoam[ch].setLowpass(currentSampleRate, 11000.0f, 0.707f);

                // Bone Conduction: 150 Hz high-pass, 1800 Hz peak (+6 dB, Q=1.4), 4800 Hz low-pass
                boneHP[ch].setHighpass(currentSampleRate, 150.0f, 0.707f);
                bonePeak[ch].setPeaking(currentSampleRate, 1800.0f, 6.0f, 1.4f);
                boneLP[ch].setLowpass(currentSampleRate, 4800.0f, 0.707f);
                boneBuzzLP[ch].setLowpass(currentSampleRate, 220.0f, 0.707f);
            }
        }
    };
}
