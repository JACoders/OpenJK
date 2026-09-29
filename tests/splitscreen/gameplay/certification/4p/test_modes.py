#!/usr/bin/env python3
"""Offline contracts for strict current-suite four-player stock-mode certification."""

import importlib.util
from pathlib import Path
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "gp503_4p_modes_validate", HERE / "validate_modes.py"
)
VALIDATE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VALIDATE)


class FourPlayerModesContract(unittest.TestCase):
    def test_exactly_nine_stock_modes_are_required(self) -> None:
        self.assertEqual(
            {
                "ffa-4p", "holocron-4p", "jedimaster-4p", "duel-4p",
                "powerduel-4p", "ctf-4p", "cty-4p", "team-ffa-4p",
                "siege-4p",
            },
            set(VALIDATE.REQUIRED_MODES),
        )
        self.assertEqual(9, len(VALIDATE.REQUIRED_MODES))
        self.assertEqual(set(VALIDATE.REQUIRED_MODES), set(VALIDATE.MODE_MARKERS))

    def test_missing_manifest_is_a_hard_failure(self) -> None:
        ok, errors = VALIDATE.validate_mode_manifest(
            "ffa-4p", Path("/definitely/missing/manifest.tsv"), "0" * 64,
            "1" * 64, 1,
        )
        self.assertFalse(ok)
        self.assertTrue(errors)

    def test_absence_only_manifest_and_missing_screenshots_cannot_pass(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log = root / "client.log"
            log.write_text("\n".join(VALIDATE.INDIVIDUAL_MARKERS) + "\n")
            manifest = root / "manifest.tsv"
            manifest.write_text(
                "format\topenjk-e2e-v1\n"
                "status\tpassed\n"
                "case\tffa-4p\n"
                "players\t4\n"
                f"hash\t/bin/game\t{'1' * 64}\n"
                f"process.client.log\t{log}\n"
                f"artifact\t{log}\t{VALIDATE.digest(log)}\n"
            )
            ok, errors = VALIDATE.validate_mode_manifest(
                "ffa-4p", manifest, VALIDATE.digest(manifest), "1" * 64, 0
            )
            self.assertFalse(ok)
            self.assertTrue(any("screenshots" in error for error in errors))

    def test_every_mode_accepts_only_complete_hash_backed_positive_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for index, case_name in enumerate(VALIDATE.REQUIRED_MODES):
                case = root / case_name
                case.mkdir()
                log = case / "client.log"
                log.write_text("\n".join(VALIDATE.MODE_MARKERS[case_name]) + "\n")
                first = case / "first.png"
                second = case / "second.png"
                first.write_bytes(b"first")
                second.write_bytes(b"second")
                manifest = case / "manifest.tsv"
                manifest.write_text(
                    "format\topenjk-e2e-v1\n"
                    "status\tpassed\n"
                    f"case\t{case_name}\n"
                    "players\t4\n"
                    f"hash\t/bin/game\t{'1' * 64}\n"
                    f"process.client.log\t{log}\n"
                    f"artifact\t{log}\t{VALIDATE.digest(log)}\n"
                    f"artifact\t{first}\t{VALIDATE.digest(first)}\n"
                    f"artifact\t{second}\t{VALIDATE.digest(second)}\n"
                )
                ok, errors = VALIDATE.validate_mode_manifest(
                    case_name, manifest, VALIDATE.digest(manifest), "1" * 64, 0
                )
                self.assertTrue(ok, f"{index}: {case_name}: {errors}")

                old_log_digest = VALIDATE.digest(log)
                log.write_text(log.read_text() + "SplitInputAssertCmd: FAIL\n")
                manifest.write_text(
                    manifest.read_text().replace(
                        old_log_digest,
                        VALIDATE.digest(log),
                        1,
                    )
                )
                ok, errors = VALIDATE.validate_mode_manifest(
                    case_name, manifest, VALIDATE.digest(manifest), "1" * 64, 0
                )
                self.assertFalse(ok)
                self.assertTrue(any("failure marker" in error for error in errors))

                input_fail_digest = VALIDATE.digest(log)
                log.write_text(
                    "\n".join(VALIDATE.MODE_MARKERS[case_name])
                    + "\nSplitNetStagePair: FAIL attacker=1 victim=2\n"
                )
                manifest.write_text(
                    manifest.read_text().replace(
                        input_fail_digest,
                        VALIDATE.digest(log),
                        1,
                    )
                )
                ok, errors = VALIDATE.validate_mode_manifest(
                    case_name, manifest, VALIDATE.digest(manifest), "1" * 64, 0
                )
                self.assertFalse(ok)
                self.assertTrue(any("failure marker" in error for error in errors))

    def test_runner_is_hash_pinned_resumable_and_exact_cell_capable(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        self.assertIn(
            "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
            runner,
        )
        self.assertIn("--resume", runner)
        self.assertIn("selected+=(\"$1\")", runner)
        self.assertIn("already_recorded", runner)
        self.assertIn('--validate-only "$manifest"', runner)
        self.assertIn("validate_complete_suite", runner)
        self.assertIn("validate_modes.py", runner)

    def test_runner_captures_screenshots_and_upserts_ledger_cells(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        self.assertGreaterEqual(runner.count("OPENJK_E2E_SCREENSHOTS"), 2)
        self.assertIn("$1 != target", runner)
        self.assertIn("mv \"$ledger_tmp\" \"$ledger\"", runner)

    def test_runner_dispatches_every_stock_mode_once(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        self.assertIn(
            "all_modes=(ffa holocron jedimaster duel powerduel ctf cty team-ffa siege)",
            runner,
        )
        self.assertIn("individual/client_case.sh' '$mode' 4", runner)
        self.assertIn("duel_4p.cfg", runner)
        self.assertIn("powerduel_4p.cfg", runner)
        self.assertIn("ctf_cty/launch_game.sh' 4 '$mode'", runner)
        self.assertIn("team_ffa/generate_cfg.py\" 4", runner)
        self.assertIn("siege/generate_cfg.py\" 4", runner)

    def test_individual_fixture_waits_for_party_before_team_commands(self) -> None:
        fixture = (
            HERE.parents[1] / "modes" / "individual" / "cfg" / "lifecycle_4p.cfg"
        ).read_text()
        self.assertIn("wait 2400\nclosemenu", fixture)
        self.assertLess(
            fixture.index("splitui_assert ui_splitScreenPartyState active"),
            fixture.index("splitnet_cmd 2 team free"),
        )

    def test_four_player_duel_fixtures_wait_for_party_and_prove_capacity(self) -> None:
        duel_dir = HERE.parents[1] / "modes" / "duel"
        duel = (duel_dir / "duel_4p.cfg").read_text()
        powerduel = (duel_dir / "powerduel_4p.cfg").read_text()
        for fixture in (duel, powerduel):
            self.assertIn("wait 2400\nclosemenu", fixture)
            self.assertLess(
                fixture.index("splitui_assert ui_splitScreenPartyState active"),
                fixture.index("splitnet_cmd 2 team free"),
            )
        for player in (1, 2, 3, 4):
            self.assertIn(
                f"splitnet_assert_lifecycle {player} INTERMISSION", powerduel
            )


if __name__ == "__main__":
    unittest.main()
