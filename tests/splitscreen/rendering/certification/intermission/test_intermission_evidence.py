from pathlib import Path
import subprocess
import sys
import unittest


HERE = Path(__file__).resolve().parent
ARTIFACTS = HERE / "artifacts"


class IntermissionEvidence(unittest.TestCase):
    def test_every_player_reached_intermission(self):
        log = (ARTIFACTS / "intermission.stdout.txt").read_text(
            encoding="utf-8", errors="replace")
        self.assertIn("SplitNetStagePair: PASS", log)
        self.assertRegex(log, r"hit the kill limit")
        for player in range(1, 5):
            self.assertIn(
                f"SplitNetLifecycleAssert: PASS player={player} "
                "expected=INTERMISSION actual=INTERMISSION",
                log,
            )

    def test_final_capture_passes_overlay_oracle(self):
        result = subprocess.run(
            [sys.executable, str(HERE / "assert_intermission.py"),
             str(ARTIFACTS / "cert_intermission_before.png"),
             str(ARTIFACTS / "cert_intermission_4p.png")],
            text=True, capture_output=True, check=False,
        )
        self.assertEqual(0, result.returncode, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
