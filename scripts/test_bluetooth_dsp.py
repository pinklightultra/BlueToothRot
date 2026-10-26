#!/usr/bin/env python3
"""
BlueToothRot DSP Prototype & Audio Processor
Authentically simulates degraded Bluetooth connection physics, jitter warbles,
packet loss concealment (PLC), codec starvation, and transducer/headphone acoustics
(Earbuds, Over-Ears, and Bone Conduction).
"""

import argparse
import math
import wave
import numpy as np
from scipy import signal

def read_wav(file_path):
    with wave.open(file_path, 'rb') as wf:
        n_channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        framerate = wf.getframerate()
        n_frames = wf.getnframes()
        raw_data = wf.readframes(n_frames)

    if sampwidth == 2:
        dtype = np.int16
        max_val = 32768.0
    elif sampwidth == 4:
        dtype = np.int32
        max_val = 2147483648.0
    else:
        raise ValueError(f"Unsupported sample width: {sampwidth}")

    audio = np.frombuffer(raw_data, dtype=dtype).astype(np.float32) / max_val
    if n_channels > 1:
        audio = audio.reshape(-1, n_channels)
    else:
        audio = np.column_stack((audio, audio))  # Make stereo
    return audio, framerate

def write_wav(file_path, audio, framerate):
    audio = np.clip(audio, -1.0, 1.0)
    int_data = (audio * 32767.0).astype(np.int16)
    with wave.open(file_path, 'wb') as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(framerate)
        wf.writeframes(int_data.tobytes())

def generate_test_tone(sample_rate=44100, duration_sec=5.0):
    t = np.linspace(0, duration_sec, int(sample_rate * duration_sec), endpoint=False)
    # A rich synth chord with bass and highs to test all headphone acoustics
    bass = 0.40 * np.sin(2 * np.pi * 65.41 * t)   # C2 sub-bass
    chord = (
        0.30 * np.sin(2 * np.pi * 261.63 * t) +  # C4
        0.25 * np.sin(2 * np.pi * 329.63 * t) +  # E4
        0.20 * np.sin(2 * np.pi * 392.00 * t) +  # G4
        0.15 * np.sin(2 * np.pi * 523.25 * t) +  # C5
        0.10 * np.sin(2 * np.pi * 2093.00 * t)   # C7 high sizzle
    )
    tone = bass + chord
    left = tone * (0.6 + 0.3 * np.cos(2 * np.pi * 0.5 * t))
    right = tone * (0.6 - 0.3 * np.cos(2 * np.pi * 0.5 * t))
    return np.column_stack((left, right)), sample_rate

def apply_headphone_acoustics(audio, sample_rate, headphone_type, seal_or_buzz=0.5):
    if headphone_type == 'flat':
        return audio

    out = np.copy(audio)

    if headphone_type == 'earbuds':
        # Broken/Loose Ear-Tip Seal: variable high-pass cutoff (50Hz - 220Hz)
        hp_freq = 50.0 + 170.0 * (1.0 - seal_or_buzz)
        sos_hp = signal.butter(2, hp_freq, 'hp', fs=sample_rate, output='sos')
        out[:, 0] = signal.sosfilt(sos_hp, out[:, 0])
        out[:, 1] = signal.sosfilt(sos_hp, out[:, 1])

        # 3.2 kHz Ear Canal Concha Resonance Peak (+5 dB)
        b_peak, a_peak = signal.iirpeak(3200.0, 2.2, fs=sample_rate)
        out[:, 0] = out[:, 0] + 0.45 * signal.lfilter(b_peak, a_peak, out[:, 0])
        out[:, 1] = out[:, 1] + 0.45 * signal.lfilter(b_peak, a_peak, out[:, 1])

        # Cheap dynamic driver edge saturation
        out = np.clip(out * 1.15, -1.0, 1.0)

    elif headphone_type == 'over_ear':
        # Boomy consumer bass boost (low shelf ~120 Hz)
        sos_shelf = signal.butter(1, 120.0, 'lp', fs=sample_rate, output='sos')
        out[:, 0] = out[:, 0] + 0.55 * signal.sosfilt(sos_shelf, out[:, 0])
        out[:, 1] = out[:, 1] + 0.55 * signal.sosfilt(sos_shelf, out[:, 1])

        # Earcup cavity enclosure boxiness (320 Hz resonance)
        b_cav, a_cav = signal.iirpeak(320.0, 2.5, fs=sample_rate)
        out[:, 0] = out[:, 0] + 0.35 * signal.lfilter(b_cav, a_cav, out[:, 0])
        out[:, 1] = out[:, 1] + 0.35 * signal.lfilter(b_cav, a_cav, out[:, 1])

        # Foam pad damping (steep roll-off above 11 kHz)
        sos_foam = signal.butter(2, 11000.0, 'lp', fs=sample_rate, output='sos')
        out[:, 0] = signal.sosfilt(sos_foam, out[:, 0])
        out[:, 1] = signal.sosfilt(sos_foam, out[:, 1])

        # Interaural earcup leakage / crossfeed
        cross_l = out[:, 0] * 0.93 + out[:, 1] * 0.07
        cross_r = out[:, 1] * 0.93 + out[:, 0] * 0.07
        out[:, 0] = cross_l
        out[:, 1] = cross_r

    elif headphone_type == 'bone_conduction':
        # 1. Bone Transmission High-Pass: skull does not conduct sub-bass (< 150 Hz)
        sos_hp = signal.butter(2, 150.0, 'hp', fs=sample_rate, output='sos')
        out[:, 0] = signal.sosfilt(sos_hp, out[:, 0])
        out[:, 1] = signal.sosfilt(sos_hp, out[:, 1])

        # 2. Bone Acoustic Low-Pass: skull attenuates highs sharply (> 4.8 kHz)
        sos_lp = signal.butter(2, 4800.0, 'lp', fs=sample_rate, output='sos')
        out[:, 0] = signal.sosfilt(sos_lp, out[:, 0])
        out[:, 1] = signal.sosfilt(sos_lp, out[:, 1])

        # 3. Speech clarity band peak (1.8 kHz)
        b_peak, a_peak = signal.iirpeak(1800.0, 1.4, fs=sample_rate)
        out[:, 0] = out[:, 0] + 0.70 * signal.lfilter(b_peak, a_peak, out[:, 0])
        out[:, 1] = out[:, 1] + 0.70 * signal.lfilter(b_peak, a_peak, out[:, 1])

        # 4. Transducer Motor Buzz: Low frequencies (80Hz - 220Hz) cause violent mechanical skin vibration
        sos_buzz_lp = signal.butter(2, 220.0, 'lp', fs=sample_rate, output='sos')
        bass_l = signal.sosfilt(sos_buzz_lp, audio[:, 0])
        bass_r = signal.sosfilt(sos_buzz_lp, audio[:, 1])

        buzz_harm_l = np.tanh(bass_l * (3.0 + 5.0 * seal_or_buzz))
        buzz_harm_r = np.tanh(bass_r * (3.0 + 5.0 * seal_or_buzz))

        out[:, 0] = (out[:, 0] * 0.85) + (buzz_harm_l * 0.35 * seal_or_buzz)
        out[:, 1] = (out[:, 1] * 0.85) + (buzz_harm_r * 0.35 * seal_or_buzz)

    return out

def process_bluetooth_degradation(
    audio,
    sample_rate,
    distance=0.45,
    obstacle=0.20,
    interference=0.15,
    codec='sbc',
    bitpool=18,
    jitter_warble=0.60,
    buffer_ms=45.0,
    plc_mode='robot_freeze',
    max_hold_ms=250.0,
    earbud_desync=0.30,
    headphone='flat',
    seal_or_buzz=0.50
):
    num_samples = len(audio)
    packet_size = 128
    output = np.zeros_like(audio)

    rng = np.random.RandomState(42)
    state_good = [True, True]
    history = [np.zeros(2048, dtype=np.float32), np.zeros(2048, dtype=np.float32)]
    hist_idx = [0, 0]
    pitch_period = [int(sample_rate / 150.0), int(sample_rate / 150.0)]
    freeze_phase = [0, 0]

    jit_buf_size = int(0.6 * sample_rate)
    jit_buf = [np.zeros(jit_buf_size, dtype=np.float32), np.zeros(jit_buf_size, dtype=np.float32)]
    write_pos = [0, 0]
    read_pos = [0.0, 0.0]
    playback_speed = [1.0, 1.0]
    initialized = False

    cvsd_accum = [0.0, 0.0]
    cvsd_step = [0.02, 0.02]
    cvsd_history = [0, 0]

    target_fill_samples = (buffer_ms * 0.001) * sample_rate
    max_hold_samples = (max_hold_ms * 0.001) * sample_rate

    for offset in range(0, num_samples, packet_size):
        chunk_len = min(packet_size, num_samples - offset)

        for ch in range(2):
            eff_dist = min(1.0, distance + (0.25 * earbud_desync if ch == 1 else 0.0))
            eff_obst = min(1.0, obstacle + (0.35 * earbud_desync if ch == 1 else 0.0))

            p_GB = 0.003 + (0.28 * (eff_dist ** 2.2)) + (0.45 * eff_obst)
            p_BG = max(0.08, 0.65 - 0.40 * eff_obst)

            interf_val = 0.0
            if interference > 0.01:
                t_now = offset / sample_rate
                wave_val = np.sin(2 * np.pi * 120.0 * t_now)
                if wave_val > 0.3:
                    interf_val = wave_val * interference

            if state_good[ch]:
                if rng.rand() < (p_GB + interf_val * 0.7):
                    state_good[ch] = False
            else:
                if rng.rand() < p_BG:
                    state_good[ch] = True

            loss_prob = 0.88 if not state_good[ch] else 0.005
            if interf_val > 0.5:
                loss_prob = max(loss_prob, 0.95)

            is_dropped = rng.rand() < loss_prob

            if is_dropped and freeze_phase[ch] == 0:
                h = history[ch]
                h_len = len(h)
                min_lag = int(sample_rate / 600.0)
                max_lag = int(sample_rate / 60.0)
                best_lag = min_lag
                max_corr = -1.0
                curr_h = hist_idx[ch]

                for lag in range(min_lag, max_lag, 2):
                    seg_a = h[(curr_h - np.arange(128)) % h_len]
                    seg_b = h[(curr_h - np.arange(128) - lag) % h_len]
                    denom = np.sqrt(np.sum(seg_a**2) * np.sum(seg_b**2)) + 1e-6
                    corr = np.sum(seg_a * seg_b) / denom
                    if corr > max_corr:
                        max_corr = corr
                        best_lag = lag
                pitch_period[ch] = best_lag

            for s in range(chunk_len):
                idx = offset + s
                in_s = audio[idx, ch]
                proc_s = 0.0

                if is_dropped:
                    age = freeze_phase[ch]
                    decay = max(0.0, 1.0 - (age / max_hold_samples))

                    if plc_mode == 'robot_freeze':
                        period = pitch_period[ch]
                        sub_offset = freeze_phase[ch] % period
                        read_i = (hist_idx[ch] - 1 - sub_offset) % len(history[ch])
                        proc_s = history[ch][read_i] * decay
                    elif plc_mode == 'frame_repeat':
                        sub_offset = freeze_phase[ch] % 256
                        read_i = (hist_idx[ch] - 1 - sub_offset) % len(history[ch])
                        proc_s = history[ch][read_i] * (0.88 ** (freeze_phase[ch] / 256.0))
                    else:
                        proc_s = 0.0

                    freeze_phase[ch] += 1
                else:
                    freeze_phase[ch] = 0

                    if codec == 'sbc':
                        bits = max(1, min(16, bitpool // 4))
                        levels = (2 ** bits) - 1.0
                        scale = max(0.01, abs(in_s))
                        norm = np.clip(in_s / scale, -1.0, 1.0)
                        proc_s = (np.round(norm * (levels * 0.5)) / (levels * 0.5)) * scale
                        if bitpool < 20 and s % 2 == 1:
                            proc_s *= 0.5
                    elif codec == 'hfp_cvsd':
                        bit = in_s > cvsd_accum[ch]
                        cvsd_history[ch] = ((cvsd_history[ch] << 1) | (1 if bit else 0)) & 0x0F
                        if cvsd_history[ch] in (0x0F, 0x00):
                            cvsd_step[ch] = min(0.35, cvsd_step[ch] * 1.45)
                        else:
                            cvsd_step[ch] = max(0.005, cvsd_step[ch] * 0.92)

                        if bit:
                            cvsd_accum[ch] += cvsd_step[ch]
                        else:
                            cvsd_accum[ch] -= cvsd_step[ch]
                        cvsd_accum[ch] *= 0.96
                        proc_s = cvsd_accum[ch]
                    else:
                        proc_s = in_s

                    history[ch][hist_idx[ch]] = proc_s
                    hist_idx[ch] = (hist_idx[ch] + 1) % len(history[ch])

                w = write_pos[ch]
                jit_buf[ch][w] = proc_s
                write_pos[ch] = (w + 1) % jit_buf_size

                if not initialized and write_pos[0] > int(0.04 * sample_rate):
                    initialized = True

                fill = write_pos[ch] - read_pos[ch]
                if fill < 0:
                    fill += jit_buf_size

                fill_error = fill - target_fill_samples
                target_speed = 1.0 + (fill_error / target_fill_samples) * (0.09 * jitter_warble)
                target_speed = max(0.88, min(1.15, target_speed))
                playback_speed[ch] += (target_speed - playback_speed[ch]) * 0.002

                i1 = int(np.floor(read_pos[ch]))
                frac = read_pos[ch] - i1
                i0 = (i1 - 1) % jit_buf_size
                i2 = (i1 + 1) % jit_buf_size
                i3 = (i1 + 2) % jit_buf_size

                y0, y1, y2, y3 = jit_buf[ch][i0], jit_buf[ch][i1], jit_buf[ch][i2], jit_buf[ch][i3]
                c0 = y1
                c1 = 0.5 * (y2 - y0)
                c2 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3
                c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2)
                out_sample = ((c3 * frac + c2) * frac + c1) * frac + c0

                read_pos[ch] = (read_pos[ch] + playback_speed[ch]) % jit_buf_size
                output[idx, ch] = out_sample

    # Apply Transducer / Headphone Acoustic Modeling
    final_output = apply_headphone_acoustics(output, sample_rate, headphone, seal_or_buzz)
    return final_output

def main():
    parser = argparse.ArgumentParser(description="BlueToothRot Audio Degradation Prototype")
    parser.add_argument("--input", "-i", type=str, default="", help="Input WAV file path")
    parser.add_argument("--output", "-o", type=str, default="degraded_output.wav", help="Output WAV file path")
    parser.add_argument("--distance", type=float, default=0.55, help="Distance / RSSI loss (0.0 to 1.0)")
    parser.add_argument("--obstacle", type=float, default=0.25, help="Obstacle / wall attenuation (0.0 to 1.0)")
    parser.add_argument("--interference", type=float, default=0.20, help="2.4 GHz microwave clashing (0.0 to 1.0)")
    parser.add_argument("--codec", type=str, choices=["sbc", "hfp_cvsd", "passthrough"], default="sbc")
    parser.add_argument("--bitpool", type=int, default=14, help="SBC Bitpool starvation (2 to 53)")
    parser.add_argument("--warble", type=float, default=0.75, help="Jitter pitch warble intensity (0.0 to 1.0)")
    parser.add_argument("--plc", type=str, choices=["robot_freeze", "frame_repeat", "squelch"], default="robot_freeze")
    parser.add_argument("--desync", type=float, default=0.35, help="True Wireless earbud desync (0.0 to 1.0)")
    parser.add_argument("--headphone", type=str, choices=["flat", "earbuds", "over_ear", "bone_conduction"], default="bone_conduction",
                        help="Headphone type emulation")
    parser.add_argument("--seal", type=float, default=0.65, help="Ear seal loose/tight or bone conduction buzz amount (0.0 to 1.0)")
    args = parser.parse_args()

    if args.input:
        print(f"Loading input file: {args.input}")
        audio, sr = read_wav(args.input)
    else:
        print("No input WAV specified; generating 5-second synth chord demo...")
        audio, sr = generate_test_tone(44100, 5.0)

    print(f"Processing with BlueToothRot DSP (Codec: {args.codec}, Headphone: {args.headphone}, Distance: {args.distance})...")
    processed = process_bluetooth_degradation(
        audio,
        sr,
        distance=args.distance,
        obstacle=args.obstacle,
        interference=args.interference,
        codec=args.codec,
        bitpool=args.bitpool,
        jitter_warble=args.warble,
        plc_mode=args.plc,
        earbud_desync=args.desync,
        headphone=args.headphone,
        seal_or_buzz=args.seal
    )

    write_wav(args.output, processed, sr)
    print(f"Successfully generated degraded audio with {args.headphone} acoustics at: {args.output}")

if __name__ == "__main__":
    main()
