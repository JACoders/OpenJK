from pathlib import Path
import subprocess
import unittest


HERE = Path(__file__).resolve().parent
ARTIFACTS = HERE / "artifacts"


class FrozenLifecycleEvidence(unittest.TestCase):
    def text(self, case):
        return (ARTIFACTS / f"{case}.stdout.txt").read_text(errors="replace")

    def test_respawn_all_four(self):
        log = self.text("respawn_flow")
        for player in range(1, 5):
            self.assertIn(f"PASS player={player} expected=DEAD actual=DEAD", log)
            self.assertIn(f"PASS player={player} expected=ALIVE actual=ALIVE", log)

    def test_held_attack_requires_a_new_respawn_edge(self):
        log = self.text("respawn_edge_matrix")
        for player in (2, 3, 4):
            self.assertIn(f"PASS player={player} expected=DEAD actual=DEAD", log)
            self.assertIn(f"PASS player={player} expected=ALIVE actual=ALIVE", log)

    def test_spectate_and_rejoin_secondary_viewports(self):
        log = self.text("join_spectate_flow")
        for player in (2, 3, 4):
            self.assertIn(f"PASS player={player} expected=SPECTATOR actual=SPECTATOR", log)
            self.assertIn(f"PASS player={player} expected=ALIVE actual=ALIVE", log)

    def test_team_assignment_survives_map_restart(self):
        log = self.text("map_team_matrix")
        self.assertGreaterEqual(log.count("expected=ALIVE actual=ALIVE"), 8)
        for player, team in ((1, 1), (2, 2), (3, 1), (4, 2)):
            marker = f"PASS player={player} field=team op=eq expected={team} actual={team}"
            self.assertGreaterEqual(log.count(marker), 2)

    def test_no_assertion_or_engine_error_markers(self):
        for case in ("respawn_flow", "join_spectate_flow", "map_team_matrix"):
            log = self.text(case)
            self.assertNotIn("Assert: FAIL", log)
            self.assertNotIn("ERROR:", log)

    def test_captured_four_viewport_images_pass_content_oracle(self):
        oracle = HERE.parents[2] / "assert_screenshot.py"
        for name in (
            "splitqa_respawn_flow_after.png",
            "splitqa_join_spectate_flow.png",
            "cert_lifecycle_team_before_restart.png",
            "cert_lifecycle_team_after_restart.png",
        ):
            result = subprocess.run(
                ["python3", str(oracle), str(ARTIFACTS / name), "4"],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
