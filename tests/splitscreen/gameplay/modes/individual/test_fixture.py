#!/usr/bin/env python3
from pathlib import Path
import unittest


HERE = Path(__file__).resolve().parent


class IndividualModeFixtureContract(unittest.TestCase):
    def test_every_player_count_uses_deterministic_saber_combat(self):
        for players in (2, 3, 4):
            source = (HERE / f"cfg/lifecycle_{players}p.cfg").read_text()
            self.assertIn("cmd give weaponnum 3\nweapon 1\nwait 180", source)
            expected_edges = 2 if players == 4 else 3
            self.assertEqual(
                source.count("splitnet_stage_pair 1 2 44"), expected_edges
            )
            self.assertEqual(
                source.count("splitinput_device_key keyboard MOUSE1 1"),
                expected_edges,
            )
            self.assertIn("splitnet_assert_lifecycle 2 DEAD", source)
            self.assertIn("vstr gp201_score_assert", source)
            self.assertIn("splitnet_assert_stat 2 deaths ge 1", source)

    def test_jedi_master_does_not_claim_nondeterministic_master_scoring(self):
        launcher = (HERE / "client_case.sh").read_text()
        self.assertIn(
            'jedimaster) gametype=2; score_assert="splitnet_assert_stat 2 deaths ge 1"',
            launcher,
        )
        for mode in ("ffa", "holocron"):
            self.assertIn(
                f'{mode}) gametype='
                + ("0" if mode == "ffa" else "1")
                + '; score_assert="splitnet_assert_stat 1 score ge 1"',
                launcher,
            )

    def test_spectator_rejoin_observes_stock_team_change_cooldown(self):
        owner = {2: 2, 3: 3, 4: 4}
        for players, target in owner.items():
            source = (HERE / f"cfg/lifecycle_{players}p.cfg").read_text()
            expected = (
                f"splitnet_cmd {target} team spectator\n"
                "wait 1000\n"
                f"splitnet_assert_lifecycle {target} SPECTATOR\n"
                f"splitnet_cmd {target} team free\n"
                "wait 1000\n"
                f"splitnet_assert_lifecycle {target} ALIVE"
            )
            self.assertIn(expected, source)


if __name__ == "__main__":
    unittest.main()
