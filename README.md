# BlueToothRot

BlueToothRot is a stereo JUCE plugin (VST3 and Standalone) and a Python WAV script for anyone who wants a signal to sound like a bad Bluetooth link: dropped packets, jitter-buffer pitch drift, a simple codec, and the response of earbuds, over-ears, or a bone-conduction transducer.

## Quick start

The script is [`scripts/test_bluetooth_dsp.py`](scripts/test_bluetooth_dsp.py). It needs Python 3.11 or newer, NumPy, and SciPy. From the repository root:

```bash
python3 -m pip install -r requirements.txt
python3 scripts/test_bluetooth_dsp.py --output degraded.wav --distance 0.75 --codec sbc --headphone earbuds
```

With no `--input`, the script synthesizes a 5 second chord at 44.1 kHz and writes `--output`. On Python 3.12.3, NumPy 2.5.4, and SciPy 1.18.1 that command printed:

```text
No input WAV specified; generating 5-second synth chord demo...
Processing with BlueToothRot DSP (Codec: sbc, Headphone: earbuds, Distance: 0.75)...
Successfully generated degraded audio with earbuds acoustics at: degraded.wav
```

The process exited 0 in about 4 seconds. `degraded.wav` was 882044 bytes: stereo, 16-bit PCM, 44100 Hz, 220500 frames (5.0 seconds).

`--input` reads a 16-bit or 32-bit integer PCM WAV. Mono is copied to both channels. Any other sample width raises `ValueError`. Passing [`earbuds_demo.wav`](earbuds_demo.wav) printed:

```text
Loading input file: earbuds_demo.wav
Processing with BlueToothRot DSP (Codec: sbc, Headphone: earbuds, Distance: 0.75)...
Successfully generated degraded audio with earbuds acoustics at: earbuds_again.wav
```

That command was:

```bash
python3 scripts/test_bluetooth_dsp.py --input earbuds_demo.wav --output earbuds_again.wav --distance 0.75 --codec sbc --headphone earbuds
```

`--help` lists the knobs. Defaults, if you leave them off: `--distance 0.55`, `--obstacle 0.25`, `--interference 0.20`, `--codec sbc`, `--bitpool 14`, `--warble 0.75`, `--plc robot_freeze`, `--desync 0.35`, `--headphone bone_conduction`, `--seal 0.65`. `--bitpool` is read only for `sbc`. `--seal` moves the earbud high-pass and scales the bone-conduction buzz. Flat and over-ear ignore it.

`--codec` on the script is `sbc`, `hfp_cvsd`, or `passthrough`. The plugin menu is a different set: A2DP SBC, AAC Ringing, HFP mSBC (16 kHz), and HFP CVSD (8 kHz), plus dry/wet and output gain. Same idea, separate code. The script's SBC path quantizes each sample. The plugin's SBC path is the 8-band split in [`Source/DSP/SBCFilterBank.h`](Source/DSP/SBCFilterBank.h).

## Tests

From the repository root:

```bash
python3 -m unittest discover tests
```

A run on the same Python printed:

```text
...
----------------------------------------------------------------------
Ran 3 tests in 0.064s

OK
```

[`tests/test_dsp.py`](tests/test_dsp.py) checks the tone generator, the four headphone paths, and the three script codecs. Each result has to be finite stereo audio of the same length.

## Build the plugin

You need CMake 3.22 or newer and a C++20 compiler. [`CMakeLists.txt`](CMakeLists.txt) fetches [JUCE 8.0.4](https://github.com/juce-framework/JUCE/tree/8.0.4) and builds VST3 and Standalone.

On Ubuntu the audio and GUI libraries are:

```bash
sudo apt-get update
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  cmake g++ pkg-config \
  libasound2-dev libjack-jackd2-dev \
  ladspa-sdk \
  libfreetype6-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libopengl-dev
```

From the repository root, the same two commands as [`.github/workflows/ci.yml`](.github/workflows/ci.yml):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 2
```

CI runs those on `ubuntu-latest` and `windows-latest`. The binaries land in `build/BlueToothRot_artefacts/Release/`:

- `VST3/BlueToothRot.vst3`
- `Standalone/BlueToothRot` on Linux
- `Standalone/BlueToothRot.exe` on Windows

A local configure-and-build with CMake 3.28.3 and g++ 13.3.0 produced the Linux Standalone binary and the VST3 bundle at those paths.

## What the plugin does

[`Source/PluginProcessor.cpp`](Source/PluginProcessor.cpp) walks the buffer in packets of 128 samples.

1. Distance, obstacle, interference, and earbud desync go into the two-state loss model in [`Source/DSP/RFSimulationEngine.h`](Source/DSP/RFSimulationEngine.h). The right channel is pushed further into loss as desync rises. Antenna Touch drops the packet outright.
2. A dropped packet is filled by [`Source/DSP/PacketLossConcealment.h`](Source/DSP/PacketLossConcealment.h). Robot Freeze loops one detected period and fades it out over the max-hold time. Frame Repeat loops 256 samples and attenuates. Hard Squelch fades out in 2 ms.
3. A packet that arrives goes through the codec. SBC splits the sample into 8 bands and gives the higher bands fewer bits as the bitpool falls. AAC Ringing multiplies the sample by `1 + 0.08 * sin(sampleIndex * 0.4)`. HFP mSBC bandpasses from 80 Hz to 7 kHz and then applies `tanh`. HFP CVSD is a 1-bit delta modulator whose step grows when the last four bits are identical.
4. [`Source/DSP/JitterBuffer.h`](Source/DSP/JitterBuffer.h) plays the stream back at a speed that follows how full the buffer is. The speed stays between 0.88 and 1.15. That is the pitch warble.
5. The right channel can pick up an extra delay in [`Source/DSP/StereoDesyncEngine.h`](Source/DSP/StereoDesyncEngine.h), up to 50 ms, that wanders when desync is above zero.
6. Dry/wet and output gain are applied, then [`Source/DSP/HeadphoneAcoustics.h`](Source/DSP/HeadphoneAcoustics.h) filters the whole block. The dry signal is filtered too. The types are flat, cheap in-ear, consumer over-ear, and bone conduction. The filter frequencies live in `configureFilters()`.

## Audio already in the repo

`bone_conduction_demo.wav`, `degraded_demo.wav`, `earbuds_demo.wav`, and `over_ear_demo.wav` are each stereo, 16-bit, 44100 Hz, 220500 frames (5.0 seconds), 882044 bytes. They are examples of that length and format. The unit tests exercise the script on a generated tone, and the repository does not record which flags rendered these four files.

## Layout

```text
CMakeLists.txt                 JUCE 8.0.4, VST3 and Standalone
requirements.txt               numpy, scipy
scripts/test_bluetooth_dsp.py  WAV prototype
tests/test_dsp.py              unittest checks for the script
.github/workflows/ci.yml       script tests and the plugin build
Source/PluginProcessor.cpp     signal path
Source/PluginEditor.cpp        knobs and the link meter
Source/DSP/                    loss, jitter, concealment, codecs, headphones
```

## License

There is no license file in this repository.
