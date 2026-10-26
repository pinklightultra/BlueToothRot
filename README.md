# BlueToothRot — Real-World Bluetooth Connection Degradation & Glitch Audio Plugin

A specialized lo-fi, glitch, and connection-degradation audio plugin (VST3 / CLAP / AU / Standalone) that authentically models the physical RF, transport, jitter-buffer, and psychoacoustic codec failure modes of degraded Bluetooth audio links.

---

## Technical Features & Modeled Phenomena

1. **RF Link & Channel Contention (Physical Layer)**:
   * **Gilbert-Elliott 2-state Markov chain**: Simulates burst packet loss rather than artificial random noise.
   * **RSSI / Distance & Obstacle Modeling**: Simulates 2.4 GHz signal attenuation through air, human tissue, and drywall.
   * **2.4 GHz Channel Clashing**: Emulates microwave oven 50/60 Hz radiation leakage and Wi-Fi beacon collisions.

2. **Jitter Buffer & Resynchronization (The "Warble" Engine)**:
   * **Asynchronous Sample Rate Conversion (ASRC)**: Dynamic time-compression and expansion ($0.90\times - 1.12\times$) as the receiver buffer drains or floods, producing rubbery, tape-like pitch warbles.
   * **Buffer Underruns & Overflows**: Micro-clicks on empty buffer and instantaneous frame-skips on overflow.

3. **Packet Loss Concealment (PLC - The "Robotic Pitch Freeze")**:
   * **Pitch-Synchronous Extrapolation**: Auto-correlates the last audio frame to find the fundamental pitch period and repeats that single period in a crossfaded loop. Recreates the eerie metallic "drone freeze" heard on dropping wireless calls.
   * **Frame Repeat & Soft Mute**: Fallback modes with exponential decay.

4. **Authentic Codec Emulation**:
   * **A2DP SBC (Subband Codec)**: 8-band polyphase analysis/synthesis filter bank with dynamic bitpool starvation (allocating bits to subbands and truncating high frequencies).
   * **HFP 1.5 CVSD (Continuously Variable Slope Delta)**: 8 kHz telephone bandpass with 1-bit delta modulation and syllabic companding slope-overload distortion.
   * **HFP 1.6 mSBC (Wideband Speech)**: 16 kHz bandpass with 7.5ms voice framing.

5. **Headphone Transducer & Bone Conduction Acoustics**:
   * **Cheap In-Ear Earbuds**: Broken silicone tip seal modeling (steep sub-bass roll-off), 3.2 kHz ear-canal acoustic resonance peak, and plasticky micro-driver transient clipping.
   * **Consumer Over-Ear**: Boomy 110 Hz bass boost, 320 Hz hollow earcup cavity enclosure resonance, foam pad high-frequency damping, and acoustic ear cup crossfeed.
   * **Bone Conduction (Cheekbone Transducers)**: Sub-bass transmission cutoff (< 150 Hz), steep skull-bone acoustic low-pass filtering (> 4.8 kHz), 1.8 kHz speech clarity band boost, and tactile transducer motor skin buzz harmonic distortion.

6. **True Wireless Stereo (TWS) Desync**:
   * Independent RF packet loss on Left and Right channels.
   * Cross-head RF shadowing: The secondary earbud experiences random dropouts and 5–30ms micro-delay slips before snapping back.

---

## Repository Structure

```
BlueToothRot/
├── CMakeLists.txt              # Standard JUCE CMake build script (VST3, AU, Standalone)
├── README.md
├── scripts/
│   └── test_bluetooth_dsp.py   # Standalone Python prototype to hear the DSP on any WAV immediately
└── Source/
    ├── PluginProcessor.h       # JUCE AudioProcessor & APVTS parameter layout
    ├── PluginProcessor.cpp
    ├── PluginEditor.h          # Custom UI with Bluetooth signal meters and controls
    ├── PluginEditor.cpp
    └── DSP/
        ├── Parameters.h        # Parameter IDs, enums, and layout constants
        ├── RFSimulationEngine.h # Gilbert-Elliott Markov model & RF interference
        ├── JitterBuffer.h      # Fractional Hermite resampler & dynamic ASRC warble
        ├── PacketLossConcealment.h # Pitch auto-correlation & metallic loop freeze
        ├── SBCFilterBank.h     # 8-band polyphase SBC subband quantizer
        ├── HandsFreeProfile.h  # CVSD delta modulation & 8k/16k telecom filtering
        └── StereoDesyncEngine.h# True Wireless earbud desynchronizer
```

---

## Building the Plugin

### Prerequisites
* CMake 3.22 or newer
* A C++20 compliant compiler (MSVC 2019/2022, Clang 14+, or GCC 11+)

### Build Commands
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
The compiled VST3 and Standalone binaries will be located in `build/BlueToothRot_artefacts/Release/`.

---

## Testing the DSP Immediately (Python Prototype)
You can test the entire DSP chain immediately without compiling C++:
```bash
python scripts/test_bluetooth_dsp.py --input your_song.wav --output degraded.wav --distance 0.75 --codec sbc
```
