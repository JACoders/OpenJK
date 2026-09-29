#!/usr/bin/env python3
from pathlib import Path
import unittest

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]


class DiagnosisContract(unittest.TestCase):
    def test_button_bits_disprove_lightning_label(self):
        shared = (ROOT / "codemp/qcommon/q_shared.h").read_text()
        self.assertIn("#define\tBUTTON_ATTACK\t\t\t1", shared)
        self.assertIn("#define BUTTON_FORCE_LIGHTNING\t1024", shared)
        self.assertEqual(257, 1 | 256)
        self.assertNotEqual(257, 1024)

    def test_corrected_bridge_and_force_probes_are_separate_then_combined(self):
        cfg = (HERE / "local_max.cfg").read_text()
        bridge = [cfg.index(f"echo GP5-03:BRIDGE-P{player}") for player in (2, 3, 4)]
        bridge_all = cfg.index("echo GP5-03:BRIDGE-ALL")
        force = [cfg.index(f"echo GP5-03:FORCE-P{player}") for player in (2, 3, 4)]
        force_all = cfg.index("echo GP5-03:FORCE-ALL")
        self.assertEqual(bridge, sorted(bridge))
        self.assertTrue(all(item < bridge_all for item in bridge))
        self.assertEqual(force, sorted(force))
        self.assertTrue(all(item < force_all for item in force))


if __name__ == "__main__":
    unittest.main()
