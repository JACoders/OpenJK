from pathlib import Path
import unittest


HERE = Path(__file__).resolve().parent
LOG = HERE / "artifacts" / "gameplay.stdout.txt"


class GameplayEvidenceTests(unittest.TestCase):
    def test_lightning_duel_has_complete_passing_evidence(self):
        text = LOG.read_text()
        required = (
            "SplitProfileAssert: PASS player=2 key=model expected=reborn/default",
            "SplitProfileAssert: PASS player=2 key=forcepowers",
            "SplitInputSim: button device=controller1 player=2 button=0 pressed=1",
            "SplitInputAssertCmd: PASS player=2",
            "SplitNetStatAssert: PASS player=1 field=health op=lt expected=100",
            "SplitInputSim: key device=keyboard player=1 key=76 down=1",
            "SplitInputAssertCmd: PASS player=1",
            "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
            "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
            "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1 actual=1",
        )
        for marker in required:
            self.assertIn(marker, text)
        self.assertNotIn("Assert: FAIL", text)

    def test_visual_evidence_exists(self):
        for name in (
            "acceptance_p2_attacks_kyle.png",
            "acceptance_kyle_lightning.png",
            "acceptance_kyle_lightning_kill.png",
        ):
            self.assertGreater((HERE / "artifacts" / name).stat().st_size, 100_000)


if __name__ == "__main__":
    unittest.main()
