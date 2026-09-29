#!/usr/bin/env python3
"""Guard recovery fixture timing and same-IP server configuration."""

import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent


class RecoveryFixtureContract(unittest.TestCase):
    def test_controlled_server_disables_mod_same_ip_admission_limit(self):
        server = (HERE / "server_driver.sh").read_text()
        self.assertIn("+set g_antiFakePlayer 0", server)

    def test_whole_party_windows_cover_serialized_secondary_handshakes(self):
        generator = (HERE / "generate_cfg.py").read_text()
        self.assertGreaterEqual(generator.count('"wait 4200"'), 2)
        self.assertIn('"wait 4800"', generator)
        self.assertIn('if p == 4:', generator)
        self.assertIn(
            'lines += [f"splitnet_cmd {target} forcechanged free", "wait 600"]',
            generator,
        )

    def test_analysis_and_validation_do_not_pin_known_failures(self):
        analyzer = (HERE / "analyze.py").read_text()
        validator = (HERE / "validate.py").read_text()
        self.assertIn('"runtime_acceptance_met": runtime_acceptance_met', analyzer)
        self.assertNotIn("EXPECTED_SUMMARY", validator)
        self.assertNotIn(
            'assert cells["p1_party_disconnect"]["status"] == "DISCOVERED_FAIL"',
            validator,
        )

    def test_server_loss_oracle_uses_controlled_server_lifecycle(self):
        analyzer = (HERE / "analyze.py").read_text()
        self.assertIn('"process.server.log"', analyzer)
        self.assertIn('server_text.count("------ Server Initialization ------") >= 2', analyzer)
        self.assertIn(
            "authoritative server-loss observation from controlled-server",
            analyzer,
        )

    def test_runner_supports_duplicate_safe_exact_player_retries(self):
        runner = (HERE / "run.sh").read_text()
        self.assertIn("players_to_run=(2 3 4)", runner)
        self.assertIn('usage: run.sh [2] [3] [4]', runner)
        self.assertIn("$1 != player", runner)


if __name__ == "__main__":
    unittest.main()
