#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace BlueToothDSP
{
    class JitterBuffer
    {
    public:
        void prepare(double sampleRate, int maxDelayMs = 500)
        {
            currentSampleRate = sampleRate;
            bufferSize = static_cast<int>((maxDelayMs * 0.001) * sampleRate) + 2048;
            buffer.assign(2, std::vector<float>(bufferSize, 0.0f));

            writePos[0] = 0;
            writePos[1] = 0;
            readPos[0] = 0.0;
            readPos[1] = 0.0;
            playbackSpeed[0] = 1.0;
            playbackSpeed[1] = 1.0;
            initialized = false;
        }

        void reset()
        {
            for (auto& ch : buffer)
                std::fill(ch.begin(), ch.end(), 0.0f);
            writePos[0] = 0;
            writePos[1] = 0;
            readPos[0] = 0.0;
            readPos[1] = 0.0;
            playbackSpeed[0] = 1.0;
            playbackSpeed[1] = 1.0;
            initialized = false;
        }

        // Push incoming real-time audio chunk into ring buffer
        void pushSample(int channel, float sample)
        {
            int ch = channel % 2;
            buffer[ch][writePos[ch]] = sample;
            writePos[ch] = (writePos[ch] + 1) % bufferSize;

            if (!initialized && writePos[0] > static_cast<int>(0.04 * currentSampleRate))
            {
                readPos[0] = 0.0;
                readPos[1] = 0.0;
                initialized = true;
            }
        }

        // Pull resampled output sample with dynamic ASRC pitch warble
        float pullSample(int channel, float targetBufferMs, float jitterDriftIntensity)
        {
            int ch = channel % 2;
            if (!initialized)
                return 0.0f;

            // Compute current buffer fill level in samples
            double fill = static_cast<double>(writePos[ch]) - readPos[ch];
            if (fill < 0.0)
                fill += bufferSize;

            double targetFillSamples = (targetBufferMs * 0.001) * currentSampleRate;
            double fillError = fill - targetFillSamples;

            // ASRC Clock Resync PID/Proportional Control:
            // When error is negative (buffer emptying) -> slow down playback (pitch drops)
            // When error is positive (buffer overfilled) -> speed up playback (pitch rises)
            double targetSpeed = 1.0 + (fillError / targetFillSamples) * (0.08 * jitterDriftIntensity);
            targetSpeed = std::clamp(targetSpeed, 0.88, 1.15);

            // Smooth speed transitions to create rubbery tape-like inertia
            playbackSpeed[ch] += (targetSpeed - playbackSpeed[ch]) * 0.001;

            // Read with 4-point Hermite cubic interpolation
            int i1 = static_cast<int>(std::floor(readPos[ch]));
            float frac = static_cast<float>(readPos[ch] - i1);

            int i0 = (i1 - 1 + bufferSize) % bufferSize;
            int i2 = (i1 + 1) % bufferSize;
            int i3 = (i1 + 2) % bufferSize;

            float y0 = buffer[ch][i0];
            float y1 = buffer[ch][i1];
            float y2 = buffer[ch][i2];
            float y3 = buffer[ch][i3];

            // Hermite polynomial
            float c0 = y1;
            float c1 = 0.5f * (y2 - y0);
            float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
            float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
            float outSample = ((c3 * frac + c2) * frac + c1) * frac + c0;

            // Advance fractional read position
            readPos[ch] += playbackSpeed[ch];
            if (readPos[ch] >= bufferSize)
                readPos[ch] -= bufferSize;

            // Hard underrun handling (buffer exhausted)
            if (fill < 8.0)
            {
                // Micro-click / stutter hold
                readPos[ch] = writePos[ch] - 16.0;
                if (readPos[ch] < 0.0)
                    readPos[ch] += bufferSize;
            }

            return outSample;
        }

    private:
        double currentSampleRate = 44100.0;
        int bufferSize = 32768;
        std::vector<std::vector<float>> buffer;
        int writePos[2] = { 0, 0 };
        double readPos[2] = { 0.0, 0.0 };
        double playbackSpeed[2] = { 1.0, 1.0 };
        bool initialized = false;
    };
}
