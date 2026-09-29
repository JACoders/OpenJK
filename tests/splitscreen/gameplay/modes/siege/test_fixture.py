#!/usr/bin/env python3
"""Fail-closed contracts for the generated Siege fixtures."""

from __future__ import annotations

import re
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


class SiegeGeneratedFixtureContract(unittest.TestCase):
    def test_multiword_classes_are_quoted_and_have_authoritative_team_outcomes(self):
        classes = (
            "Imperial Snowtrooper",
            "Rebel Infantry",
            "Rocket Trooper",
            "Jedi Guardian",
        )
        for players in (2, 3, 4):
            with self.subTest(players=players):
                text = generated(players)
                self.assertIn(f'siegeclass "{classes[0]}"', text)
                for player in range(2, players + 1):
                    self.assertIn(
                        f'splitnet_cmd {player} siegeclass "{classes[player - 1]}"',
                        text,
                    )
                for player in range(1, players + 1):
                    team = 2 if player % 2 == 0 else 1
                    self.assertGreaterEqual(
                        text.count(f"splitnet_assert_stat {player} team eq {team}"),
                        2,
                    )
                    self.assertGreaterEqual(
                        text.count(f"splitnet_assert_lifecycle {player} ALIVE"),
                        3,
                    )

    def test_four_player_class_commands_wait_for_active_party(self):
        text = generated(4)
        self.assertIn("wait 2400\nclosemenu", text)
        party_assert = text.index(
            "splitui_assert ui_splitScreenPartyState active"
        )
        self.assertLess(party_assert, text.index('siegeclass "Imperial Snowtrooper"'))
        self.assertLess(
            party_assert,
            text.index('splitnet_cmd 4 siegeclass "Jedi Guardian"'),
        )

    def test_use_probe_asserts_bound_use_plus_any_key_mask(self):
        for players in (2, 3, 4):
            with self.subTest(players=players):
                text = generated(players)
                use = text.split("GP2-05:USE-BEGIN", 1)[1].split(
                    "GP2-05:USE-END", 1
                )[0]
                self.assertIn("bind e +use", text)
                self.assertIn("splitinput_device_key keyboard e 1", use)
                self.assertIn("splitinput_assert_cmd 1 -999 -999 -999 288", use)
                self.assertNotIn("splitinput_assert_cmd 1 -999 -999 -999 32", text)

    def test_death_wave_and_class_change_require_ordered_dead_to_alive_transitions(self):
        for players in (2, 3, 4):
            with self.subTest(players=players):
                text = generated(players)
                death_wave = text.split("GP2-05:DEATH-WAVE-BEGIN", 1)[1].split(
                    "GP2-05:DEATH-WAVE-END", 1
                )[0]
                self.assertRegex(
                    death_wave,
                    re.compile(
                        r"set g_siegeRespawn 0\s+splitnet_cmd 2 kill\s+wait 120\s+"
                        r"splitnet_assert_lifecycle 2 DEAD.*?"
                        r"set g_siegeRespawn 1\s+wait 500\s+"
                        r"splitnet_assert_lifecycle 2 ALIVE",
                        re.DOTALL,
                    ),
                )
                class_change = text.split(
                    "GP2-05:CLASS-CHANGE-BEGIN", 1
                )[1].split("GP2-05:CLASS-CHANGE-END", 1)[0]
                self.assertIn('splitnet_cmd 2 siegeclass "Rebel Sniper"', class_change)
                self.assertLess(
                    class_change.index("splitnet_assert_lifecycle 2 DEAD"),
                    class_change.index("splitnet_assert_lifecycle 2 ALIVE"),
                )
                self.assertIn("splitnet_assert_stat 2 team eq 2", class_change)
                self.assertIn("splitnet_assert_stat 2 weapon eq 6", class_change)


if __name__ == "__main__":
    unittest.main()
