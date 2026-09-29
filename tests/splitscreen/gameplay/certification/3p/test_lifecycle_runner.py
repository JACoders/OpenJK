#!/usr/bin/env python3
"""Guard the stock five-second team-switch cooldown in the 3p lifecycle."""

from pathlib import Path
import unittest


HERE = Path(__file__).resolve().parent


class LifecycleRunnerContract(unittest.TestCase):
    def test_runner_does_not_shorten_source_cooldowns(self):
        runner = (HERE / "run_lifecycle.sh").read_text()
        self.assertNotIn("s/wait 1000/wait 360/g", runner)

    def test_source_fixture_has_full_rejoin_cooldowns(self):
        root = HERE.parents[4]
        fixture = (root / "tests/splitscreen/cfg/join_spectate_flow.cfg").read_text()
        self.assertGreaterEqual(fixture.count("wait 1000"), 6)
        self.assertIn("wait 4200\nclosemenu", fixture)


if __name__ == "__main__":
    unittest.main()
