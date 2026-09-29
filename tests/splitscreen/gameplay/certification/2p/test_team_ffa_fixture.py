#!/usr/bin/env python3
"""Offline contract tests for the owned two-player Team FFA fixture."""

import re
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
CFG = HERE / "team_ffa_2p.cfg"
RUNNER = HERE / "run_extended_modes.sh"


class TeamFfaFixtureContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = CFG.read_text()
        cls.lines = [
            line.strip()
            for line in cls.text.splitlines()
            if line.strip() and not line.lstrip().startswith("//")
        ]

    def test_runner_uses_owned_fixture_not_shared_generator(self):
        runner = RUNNER.read_text()
        self.assertIn('team_cfg="$root/tests/splitscreen/gameplay/certification/2p/team_ffa_2p.cfg"', runner)
        self.assertNotIn("modes/team_ffa/generate_cfg.py", runner)

    def test_routed_attack_accepts_nonzero_button_mask(self):
        assertions = [line for line in self.lines if line.startswith("splitinput_assert_cmd 1 ")]
        self.assertEqual(["splitinput_assert_cmd 1 -999 -999 -999 -998"], assertions)
        self.assertNotIn("splitinput_assert_cmd 1 -999 -999 -999 1", self.text)

    def test_combat_is_staged_and_repeats_attack_edges(self):
        combat = self.text.split("GP5-01:ROUTED-COMBAT-BEGIN", 1)[1].split(
            "GP5-01:ROUTED-COMBAT-END", 1
        )[0]
        self.assertIn("cmd give weaponnum 3", combat)
        self.assertIn("weapon 1", combat)
        self.assertIn("set g_saberDamageScale 20", self.text)
        self.assertGreaterEqual(combat.count("splitnet_stage_pair 1 2 48"), 3)
        self.assertGreaterEqual(
            combat.count("splitinput_device_key keyboard MOUSE1 1"), 3
        )
        self.assertIn("splitnet_assert_lifecycle 2 DEAD", combat)
        self.assertIn("splitnet_assert_stat 1 score ge 1", combat)

    def test_respawns_use_attack_edges_after_dead_assertions(self):
        p2 = self.text.split("GP5-01:P2-RESPAWN-BEGIN", 1)[1].split(
            "GP5-01:P2-RESPAWN-END", 1
        )[0]
        self.assertRegex(
            p2,
            r"wait 120\s+splitinput_device_button controller1 0 1"
            r"\s+wait 60\s+splitinput_device_button controller1 0 0"
            r"\s+wait 420\s+splitnet_assert_lifecycle 2 ALIVE",
        )
        p1 = self.text.split("GP5-01:P1-SUICIDE-RESPAWN-BEGIN", 1)[1].split(
            "GP5-01:P1-SUICIDE-RESPAWN-END", 1
        )[0]
        self.assertRegex(
            p1,
            re.compile(
            r"splitnet_assert_lifecycle 1 DEAD.*?"
            r"splitinput_device_key keyboard MOUSE1 1\s+wait 60\s+"
            r"splitinput_device_key keyboard MOUSE1 0\s+wait 420\s+"
            r"splitnet_assert_lifecycle 1 ALIVE",
                re.DOTALL,
            ),
        )

    def test_every_p2_team_transition_observes_cooldown_at_high_fps(self):
        transitions = [
            index
            for index, line in enumerate(self.lines)
            if line.startswith("splitnet_cmd 2 team ")
        ]
        # Initial assignment and post-map assignment are fresh map contexts.
        lifecycle_transitions = transitions[1:5]
        self.assertEqual(4, len(lifecycle_transitions))
        for index in lifecycle_transitions:
            self.assertEqual("wait 1500", self.lines[index + 1])
        self.assertNotIn("wait 1000", self.text)

    def test_second_map_preserves_cheats_for_terminal_staging(self):
        self.assertIn("devmap mp/ffa2", self.lines)
        self.assertNotIn("map mp/ffa2", self.lines)
        terminal = self.text.split("GP5-01:INTERMISSION-BEGIN", 1)[1]
        self.assertIn("splitnet_stage_pair 1 2 48", terminal)
        self.assertIn("cmd giveother 1 health 1", terminal)
        self.assertIn("cmd give weaponnum 3", terminal)
        self.assertIn("weapon 1", terminal)
        self.assertIn("splitnet_assert_lifecycle 1 INTERMISSION", terminal)
        self.assertIn("splitnet_assert_lifecycle 2 INTERMISSION", terminal)

    def test_fixture_never_uses_unsupported_ready_command(self):
        self.assertIsNone(re.search(r"(?m)^\s*(?:cmd |splitnet_cmd \d+ )?ready\s*$", self.text))


if __name__ == "__main__":
    unittest.main()
