#!/usr/bin/env python3
"""Offline contracts for strict current-suite two-player certification."""

import importlib.util
from pathlib import Path
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("gp501_validate", HERE / "validate.py")
VALIDATE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(VALIDATE)


class StrictValidatorContract(unittest.TestCase):
    def test_all_stock_modes_are_required(self) -> None:
        self.assertEqual(
            {
                "ffa-2p", "holocron-2p", "jedimaster-2p", "duel-2p",
                "powerduel-2p", "ctf-2p", "cty-2p", "team-ffa-2p",
                "siege-2p",
            },
            set(VALIDATE.REQUIRED_MODES),
        )

    def test_missing_manifest_is_a_hard_failure(self) -> None:
        ok, errors = VALIDATE.validate_mode_manifest(
            "ffa-2p", Path("/definitely/missing/manifest.tsv"), "0" * 64,
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
                "case\tffa-2p\n"
                "players\t2\n"
                f"hash\t/bin/game\t{'1' * 64}\n"
                f"process.client.log\t{log}\n"
                f"artifact\t{log}\t{VALIDATE.digest(log)}\n"
            )
            ok, errors = VALIDATE.validate_mode_manifest(
                "ffa-2p", manifest, VALIDATE.digest(manifest), "1" * 64, 0
            )
            self.assertFalse(ok)
            self.assertTrue(any("missing positive marker" in error for error in errors))

    def test_index_rejects_missing_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            index = Path(directory) / "base.tsv"
            index.write_text(
                "format\topenjk-cert-local-v1\n"
                "run_id\trun-1\n"
                f"artifact\t{Path(directory) / 'missing.log'}\t{'0' * 64}\n"
            )
            ok, errors = VALIDATE.validate_index(index, "run-1")
            self.assertFalse(ok)
            self.assertTrue(errors)

    def test_orchestrator_uses_one_explicit_suite_id(self) -> None:
        runner = (HERE / "run_local.sh").read_text()
        journey_runner = (HERE / "run.sh").read_text()
        self.assertIn('OPENJK_CERT_RUN_ID="$run_id"', runner)
        self.assertIn('GP501_DEFER_VALIDATE=1', runner)
        self.assertIn('"$here/run_auxiliary.sh"', runner)
        self.assertIn('touch "$suite_dir/modes.tsv" "$suite_dir/aux.tsv"', runner)
        self.assertIn('"$here/run_modes.sh"', runner)
        self.assertIn('"$here/run_extended_modes.sh"', runner)
        self.assertIn("verify_frozen_artifacts.py", runner)
        self.assertIn("verify_frozen_artifacts.py", journey_runner)
        self.assertIn('suite_env="$results/local-suite/$run_id/suite.env"', journey_runner)
        self.assertIn('started_epoch="$(awk -F =', journey_runner)
        self.assertIn("run_if_missing modal_ownership", (HERE / "run_auxiliary.sh").read_text())

    def test_extended_modes_support_exact_cell_retries(self) -> None:
        runner = (HERE / "run_extended_modes.sh").read_text()
        for cell in (
            "duel", "powerduel", "ctf", "cty", "team-ffa", "siege"
        ):
            self.assertIn(cell, runner)
        self.assertIn("unknown GP501_EXTENDED_ONLY cell", runner)
        self.assertIn('$only" == siege', runner)
        self.assertIn("OPENJK_E2E_SCREENSHOTS", runner)
        self.assertIn("$1 != target", runner)

    def test_auxiliary_gates_are_evidence_backed_and_broader_gaps_stay_hard(self) -> None:
        validator = (HERE / "validate.py").read_text()
        auxiliary = (HERE.parent / "auxiliary.py").read_text()
        self.assertIn("validate_auxiliary_rows", validator)
        self.assertNotIn('checks["modal_ownership_current_suite"] = False', validator)
        self.assertNotIn('checks["asymmetric_network_recovery_current_suite"] = False', validator)
        self.assertIn('checks["physical_controller_hotplug_current_suite"] = False', validator)
        self.assertIn(
            'checks["whole_party_server_loss_recovery_current_suite"] = False',
            validator,
        )
        self.assertIn("physical_hotplug_covered", auxiliary)
        self.assertIn("server_loss_recovery_covered", auxiliary)


if __name__ == "__main__":
    unittest.main()
