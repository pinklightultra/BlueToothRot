import unittest
import numpy as np
from scripts.test_bluetooth_dsp import (
    generate_test_tone,
    apply_headphone_acoustics,
    process_bluetooth_degradation,
)


class TestBlueToothRotDSP(unittest.TestCase):
    def test_tone_generator(self):
        sr = 44100
        tone, out_sr = generate_test_tone(sample_rate=sr, duration_sec=0.1)
        self.assertEqual(out_sr, sr)
        self.assertEqual(tone.shape, (int(sr * 0.1), 2))
        self.assertTrue(np.all(np.isfinite(tone)))

    def test_headphone_acoustics(self):
        sr = 44100
        sine = 0.5 * np.sin(2 * np.pi * 440 * np.linspace(0, 0.05, int(sr * 0.05))).astype(np.float32)
        stereo = np.column_stack([sine, sine])

        for hp in ["flat", "earbuds", "over_ear", "bone_conduction"]:
            proc = apply_headphone_acoustics(stereo, sample_rate=sr, headphone_type=hp, seal_or_buzz=0.5)
            self.assertEqual(proc.shape, stereo.shape)
            self.assertTrue(np.all(np.isfinite(proc)))

    def test_process_bluetooth_degradation(self):
        sr = 44100
        sine = 0.5 * np.sin(2 * np.pi * 440 * np.linspace(0, 0.05, int(sr * 0.05))).astype(np.float32)
        stereo = np.column_stack([sine, sine])

        for codec in ["sbc", "hfp_cvsd", "passthrough"]:
            proc = process_bluetooth_degradation(
                stereo,
                sample_rate=sr,
                distance=0.3,
                obstacle=0.1,
                interference=0.1,
                codec=codec,
                plc_mode="frame_repeat"
            )
            self.assertEqual(proc.shape, stereo.shape)
            self.assertTrue(np.all(np.isfinite(proc)))


if __name__ == "__main__":
    unittest.main()
