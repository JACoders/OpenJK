#!/usr/bin/env python3
"""Fail-closed contracts for the generated Team FFA fixtures."""

from __future__ import annotations

import subprocess
import sys
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
GENERATOR = HERE / "generate_cfg.py"


def generated(players: int) -> str:
    return subprocess.run(
        [sys.executable, str(GENERATOR), str(players)],
        check=True,
        capture_output=True,
        text=True,
    ).stdout


class TeamFfaGeneratedFixtureContract(unittest.TestCase):
    def test_all_player_counts_have_positive_enemy_kill_and_terminal_oracles(self):
        for players in (2, 3, 4):
            with self.subTest(players=players):
                text = generated(players)
                enemy = text.split("GP2-03:ENEMY-KILL-BEGIN", 1)[1].split(
                    "GP2-03:ENEMY-KILL-END", 1
                )[0]
                self.assertIn("cmd give weaponnum 3", enemy)
                self.assertIn("weapon 1", enemy)
                self.assertGreaterEqual(enemy.count("splitnet_stage_pair 1 2 48"), 3)
                self.assertGreaterEqual(
                    enemy.count("splitinput_device_key keyboard MOUSE1 1"), 3
                )
                self.assertIn("splitinput_assert_cmd 1 -999 -999 -999 -998", enemy)
                self.assertIn("splitnet_assert_lifecycle 2 DEAD", enemy)
                self.assertIn("splitnet_assert_stat 1 score ge 1", enemy)
                terminal = text.split("GP2-03:INTERMISSION-BEGIN", 1)[1]
                self.assertGreaterEqual(terminal.count("splitnet_stage_pair 1 2 48"), 2)
                for player in range(1, players + 1):
                    self.assertIn(
                        f"splitnet_assert_lifecycle {player} INTERMISSION", terminal
                    )

    def test_stock_team_switch_cooldown_and_cheat_context_are_preserved(self):
        for players in (2, 3, 4):
            with self.subTest(players=players):
                text = generated(players)
                switch = text.split("GP2-03:TEAM-SWITCH-BEGIN", 1)[1].split(
                    "GP2-03:TEAM-SWITCH-END", 1
                )[0]
                spectator = text.split("GP2-03:SPECTATE-BEGIN", 1)[1].split(
                    "GP2-03:SPECTATE-END", 1
                )[0]
                self.assertEqual(2, switch.count("wait 1500"))
                self.assertEqual(2, spectator.count("wait 1500"))
                self.assertIn("devmap mp/ffa2", text)
                self.assertNotIn("\nmap mp/ffa2", text)
                self.assertNotRegex(
                    text, r"(?m)^\s*(?:cmd |splitnet_cmd \d+ )?ready\s*$"
                )

    def test_same_team_probe_exists_only_when_a_same_team_victim_exists(self):
        self.assertNotIn("GP2-03:TEAMKILL-OFF-BEGIN", generated(2))
        for players in (3, 4):
            with self.subTest(players=players):
                text = generated(players)
                self.assertIn("splitnet_assert_stat 3 health eq 1", text)
                self.assertIn("splitnet_assert_lifecycle 3 DEAD", text)
                self.assertGreaterEqual(text.count("splitnet_stage_pair 1 3 48"), 3)


if __name__ == "__main__":
    unittest.main()
