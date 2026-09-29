#!/usr/bin/env python3
"""Lock the evidence-based GP4-04 routing decisions."""

from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/modes"


class FindingReclassification(unittest.TestCase):
    def test_team_join_failure_is_malformed_routed_command_not_setteam_rule(self) -> None:
        pointer = RESULTS / "ctf_cty/ctf-3p-manifest-path.txt"
        manifest = Path(pointer.read_text().strip())
        fields = {
            parts[0]: parts[1]
            for line in manifest.read_text().splitlines()
            if len(parts := line.split("\t")) >= 2
        }
        text = Path(fields["process.client.log"]).read_text(errors="replace")
        self.assertIn("clientCommand: Objective_P3 : 2 : 3 team red", text)
        self.assertNotIn("clientCommand: Objective_P3 : 2 : team red", text)

    def test_team_ffa_combat_probe_records_input_mismatch(self) -> None:
        text = (RESULTS / "team_ffa/README.md").read_text()
        self.assertIn("usercmd carried buttons `2` while the probe expected `1`", text)

    def test_duel_report_disclaims_queue_order_oracle(self) -> None:
        text = (RESULTS / "duel/findings.md").read_text()
        self.assertIn("not queue order", text)
        self.assertIn("No production diagnostic exposes numeric duel roles", text)


if __name__ == "__main__":
    unittest.main()
