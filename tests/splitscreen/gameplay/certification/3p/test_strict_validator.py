#!/usr/bin/env python3
"""Offline contracts for strict current-suite three-player certification."""

import importlib.util
from pathlib import Path
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("gp502_validate", HERE / "validate.py")
VALIDATE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VALIDATE)


class StrictValidatorContract(unittest.TestCase):
    def test_all_stock_modes_are_required(self) -> None:
        self.assertEqual(
            {
                "ffa-3p", "holocron-3p", "jedimaster-3p", "duel-3p",
                "powerduel-3p", "ctf-3p", "cty-3p", "team-ffa-3p",
                "siege-3p",
            },
            set(VALIDATE.REQUIRED_MODES),
        )

    def test_missing_manifest_is_a_hard_failure(self) -> None:
        ok, errors = VALIDATE.validate_mode_manifest(
            "ffa-3p", Path("/definitely/missing/manifest.tsv"), "0" * 64,
            "1" * 64, 1,
        )
        self.assertFalse(ok)
        self.assertTrue(errors)

    def test_absence_only_manifest_cannot_pass(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            log = root / "client.log"
            log.write_text("nothing failed\n")
            manifest = root / "manifest.tsv"
            manifest.write_text(
                "format\topenjk-e2e-v1\n"
                "status\tpassed\n"
                "case\tffa-3p\n"
                "players\t3\n"
                f"hash\t/bin/game\t{'1' * 64}\n"
                f"process.client.log\t{log}\n"
                f"artifact\t{log}\t{VALIDATE.digest(log)}\n"
            )
            ok, errors = VALIDATE.validate_mode_manifest(
                "ffa-3p", manifest, VALIDATE.digest(manifest), "1" * 64, 0
            )
            self.assertFalse(ok)
            self.assertTrue(any("missing positive marker" in error for error in errors))

    def test_index_rejects_wrong_suite(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            artifact = Path(directory) / "log"
            artifact.write_text("positive")
            index = Path(directory) / "index.tsv"
            index.write_text(
                "format\topenjk-cert-local-v1\n"
                "run_id\told-run\n"
                f"artifact\t{artifact}\t{VALIDATE.digest(artifact)}\n"
            )
            ok, errors = VALIDATE.validate_index(index, "new-run")
            self.assertFalse(ok)
            self.assertTrue(errors)

    def test_orchestrator_runs_lifecycle_and_modes_under_one_id(self) -> None:
        runner = (HERE / "run_local.sh").read_text()
        journey_runner = (HERE / "run.sh").read_text()
        self.assertIn('GP502_DEFER_VALIDATE=1', runner)
        self.assertIn('"$here/run_lifecycle.sh"', runner)
        self.assertIn('"$here/run_auxiliary.sh"', runner)
        self.assertIn('touch "$suite_dir/modes.tsv" "$suite_dir/aux.tsv"', runner)
        self.assertIn('"$here/run_modes.sh"', runner)
        self.assertGreaterEqual(runner.count('OPENJK_CERT_RUN_ID="$run_id"'), 4)
        self.assertIn("verify_frozen_artifacts.py", runner)
        self.assertIn("verify_frozen_artifacts.py", journey_runner)
        self.assertIn('suite_env="$results/local-suite/$run_id/suite.env"', journey_runner)
        self.assertIn('started_epoch="$(awk -F =', journey_runner)
        self.assertIn("run_if_missing modal_ownership", (HERE / "run_auxiliary.sh").read_text())

    def test_individual_modes_use_owned_cooldown_hardening(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        client = (HERE / "mode_client.sh").read_text()
        fixture = (
            HERE.parents[1] / "modes" / "individual" / "cfg" / "lifecycle_3p.cfg"
        ).read_text()
        self.assertIn("certification/3p/mode_client.sh", runner)
        self.assertIn("wait 1000", client)
        self.assertNotIn("modes/individual/client_case.sh", runner)
        self.assertLess(
            fixture.index("splitui_assert ui_splitScreenPartyState active"),
            fixture.index("splitnet_cmd 2 team free"),
        )
        self.assertIn("wait 1440\nclosemenu", fixture)

    def test_team_mode_uses_the_already_hardened_shared_fixture(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        generator = (
            HERE.parents[1] / "modes" / "team_ffa" / "generate_cfg.py"
        ).read_text()
        self.assertIn("give weaponnum 3", generator)
        self.assertIn("-999 -999 -999 -998", generator)
        self.assertIn("wait 1500", generator)
        self.assertIn("devmap mp/ffa2", generator)
        self.assertNotIn("perl -0pi", runner)

    def test_mode_runner_records_screenshots_and_upserts_ledger_cells(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        self.assertIn("OPENJK_E2E_SCREENSHOTS", runner)
        self.assertIn("$1 != target", runner)

    def test_mode_runner_supports_exact_cell_retries(self) -> None:
        runner = (HERE / "run_modes.sh").read_text()
        self.assertIn(
            "canonical_modes=(ffa holocron jedimaster duel powerduel ctf cty "
            "team-ffa siege)",
            runner,
        )
        self.assertIn('only="${GP502_MODES_ONLY:-all}"', runner)
        self.assertIn('all) selected_modes=("${canonical_modes[@]}")', runner)
        self.assertIn('selected_modes=("$only")', runner)
        self.assertIn("unknown GP502_MODES_ONLY cell", runner)

    def test_auxiliary_gates_are_evidence_backed_and_broader_gaps_stay_hard(self) -> None:
        validator = (HERE / "validate.py").read_text()
        self.assertIn("validate_auxiliary_rows", validator)
        self.assertNotIn('checks["modal_ownership_current_suite"] = False', validator)
        self.assertNotIn('checks["asymmetric_network_recovery_current_suite"] = False', validator)
        self.assertIn('checks["physical_controller_hotplug_current_suite"] = False', validator)
        self.assertIn(
            'checks["whole_party_server_loss_recovery_current_suite"] = False',
            validator,
        )


if __name__ == "__main__":
    unittest.main()
