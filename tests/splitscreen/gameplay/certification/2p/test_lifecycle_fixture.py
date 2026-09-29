#!/usr/bin/env python3
from pathlib import Path
import re
import unittest


CFG = Path(__file__).with_name("lifecycle_2p.cfg").read_text(encoding="utf-8")


class LifecycleFixtureContract(unittest.TestCase):
    def test_team_commands_wait_for_all_party_slots_to_be_active(self) -> None:
        party = CFG.index("splitui_assert ui_splitScreenPartyState active")
        team = CFG.index("splitnet_cmd 2 team free")
        self.assertLess(party, team)
        self.assertIn("wait 1440\nclosemenu", CFG[:party])

    def test_generic_mode_does_not_claim_unprovisioned_combat(self) -> None:
        self.assertIn("cmd killother 1", CFG)
        self.assertNotIn("splitnet_assert_stat 1 score", CFG)
        self.assertNotIn("+force_lightning", CFG)
        self.assertNotIn("splitnet_stage_pair", CFG)
        self.assertNotIn("giveother 1 health 1", CFG)

    def test_rejoin_respects_stock_team_switch_cooldown(self) -> None:
        transition = re.search(
            r"splitnet_cmd 2 team spectator(?P<body>.*?)"
            r"splitnet_cmd 2 team free",
            CFG,
            re.S,
        )
        self.assertIsNotNone(transition)
        waits = [int(value) for value in re.findall(r"\bwait\s+(\d+)", transition.group("body"))]
        self.assertGreaterEqual(sum(waits), 720)


if __name__ == "__main__":
    unittest.main()
