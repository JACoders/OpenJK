#!/usr/bin/env python3
"""Offline honesty contracts for same-suite auxiliary evidence."""

import importlib.util
from pathlib import Path
import tempfile
import unittest


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("cert_auxiliary", HERE / "auxiliary.py")
AUX = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(AUX)


class AuxiliaryEvidenceContract(unittest.TestCase):
    def test_missing_rows_fail_all_claims(self) -> None:
        checks, errors = AUX.validate_auxiliary_rows(
            [], run_id="suite", players=2, expected_sha="1" * 64,
            started_epoch=1,
        )
        self.assertFalse(checks["auxiliary_ledger_exact"])
        for kind in AUX.AUXILIARY_KINDS:
            self.assertFalse(checks[kind])
            self.assertTrue(errors[kind])

    def test_virtual_restart_does_not_imply_physical_hotplug(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "client.log"
            log.write_text(
                "\n".join(
                    (
                        "CERT-CONTROLLER:PRE-READY",
                        "CERT-CONTROLLER:PRE-CHECKED",
                        "CERT-CONTROLLER:POST-READY",
                        "CERT-CONTROLLER:POST-CHECKED",
                        "CERT-CONTROLLER:COMPLETE",
                    )
                )
            )
            report = {
                "schema_version": 1,
                "kind": "virtual_controller_restart",
                "players": 2,
                "run_id": "suite",
                "frozen_sha256": "1" * 64,
                "passed": True,
                "input_assert_passes": 4,
                "screenshot_present": True,
                "physical_hotplug_covered": True,
                "physical_hotplug_status": "COVERED_PASS",
            }
            errors = AUX.validate_semantics(
                "virtual_controller_restart", report, log, players=2,
                run_id="suite", expected_sha="1" * 64,
            )
            self.assertTrue(any("physical-hotplug" in error for error in errors))

    def test_asymmetric_report_cannot_claim_server_loss(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            log = Path(directory) / "client.log"
            log.write_text(
                "\n".join(
                    (
                        "CERT-ASYMMETRIC:BASELINE-BEGIN",
                        "CERT-ASYMMETRIC:DISCONNECT-BEGIN",
                        "CERT-ASYMMETRIC:REJOIN-BEGIN",
                        "CERT-ASYMMETRIC:COMPLETE",
                    )
                )
            )
            report = {
                "schema_version": 1,
                "kind": "asymmetric_network_recovery",
                "players": 2,
                "run_id": "suite",
                "frozen_sha256": "1" * 64,
                "passed": True,
                "phases": {"BASELINE": True, "DISCONNECT": True, "REJOIN": True},
                "healthy_identity_preserved": True,
                "ordered_rejoin": True,
                "screenshots_present": True,
                "whole_party_reconnect_covered": False,
                "server_loss_recovery_covered": True,
            }
            errors = AUX.validate_semantics(
                "asymmetric_network_recovery", report, log, players=2,
                run_id="suite", expected_sha="1" * 64,
            )
            self.assertTrue(any("broader recovery" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
