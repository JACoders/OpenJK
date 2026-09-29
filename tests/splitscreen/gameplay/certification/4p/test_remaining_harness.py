#!/usr/bin/env python3
from pathlib import Path
import json
import subprocess
import tempfile
import unittest

HERE = Path(__file__).resolve().parent


class RemainingHarnessContract(unittest.TestCase):
    def test_every_wrapper_has_hash_and_strict_scan_gates(self):
        runner = (HERE / "run_remaining.sh").read_text()
        self.assertIn("GP5_EXPECTED_SHA256", runner)
        self.assertIn("strict_scan.sh", runner)
        for cell in ("siege", "lifecycle", "slot-churn", "fifth", "modal", "public"):
            self.assertIn(cell, runner)

    def test_modal_churn_and_fifth_are_owned_runnable_wrappers(self):
        runner = (HERE / "run_remaining.sh").read_text()
        self.assertNotIn("GP5_MODAL_RUNNER", runner)
        self.assertNotIn("GP5_FIFTH_RUNNER", runner)
        self.assertIn('"$HERE/run_modal.sh"', runner)
        self.assertIn('"$HERE/run_slot_churn.sh"', runner)
        self.assertIn('"$HERE/run_controlled_fifth.sh"', runner)
        modal = (HERE / "run_modal.sh").read_text()
        self.assertIn('"$GENERIC/run.sh" 4', modal)
        for name in ("run_modal.sh", "run_slot_churn.sh", "run_controlled_fifth.sh"):
            self.assertTrue((HERE / name).stat().st_mode & 0o111, name)

    def test_public_is_serialized_authorized_and_capacity_gated(self):
        public = (HERE / "public_serialized.sh").read_text()
        self.assertIn("/tmp/openjk-public-qa.lock", public)
        self.assertIn("GP5_ALLOW_PUBLIC", public)
        self.assertIn("GP5_PUBLIC_FREE_SLOTS", public)
        self.assertIn("135.125.145.49:29070", public)

    def test_fifth_is_independent_and_four_slots_are_asserted(self):
        host = (HERE / "controlled_fifth_host.cfg").read_text()
        fifth = (HERE / "controlled_fifth_upstream.cfg").read_text()
        self.assertIn("GP5_Independent_Fifth", fifth)
        self.assertIn("splitnet_assert_lifecycle 1 ALIVE", fifth)
        self.assertIn("GP5-03:FIFTH-CONNECTED", fifth)
        for player in range(1, 5):
            self.assertIn(f"splitnet_assert_lifecycle {player} ALIVE", host)
        launcher = (HERE / "launch_fifth_client.sh").read_text()
        self.assertIn("GP5-03:FIFTH-READY", launcher)
        self.assertIn("OPENJK_E2E_REMOTE_PORT", launcher)

    def test_varied_slot_churn_disconnects_and_rejoins_every_aux_player(self):
        churn = (HERE / "slot_churn.cfg").read_text()
        for player in (2, 3, 4):
            self.assertIn(f"splitnet_disconnect {player}", churn)
            self.assertIn(f"splitnet_rejoin {player}", churn)

    def test_simultaneous_controller_probe_allows_the_last_packet_to_sample(self):
        fixture = (HERE / "local_max.cfg").read_text()
        runner = (HERE / "run.sh").read_text()
        marker = fixture.index("echo GP5-03:FORCE-ALL")
        assertions = fixture.index(
            "splitui_assert cl_splitScreenP2LastGenericCmd 5", marker
        )
        self.assertIn("wait 300", fixture[marker:assertions])
        for player in (2, 3, 4):
            self.assertIn(
                f"send_once GP5-03:FORCE-P{player} gamepad {player - 1} "
                "button 10 down wait 4000",
                runner,
            )

    def test_controlled_fixture_waits_for_all_serialized_handshakes(self):
        controlled = (HERE / "controlled.cfg").read_text()
        self.assertIn("connect 127.0.0.1:29953\n", controlled)
        self.assertIn("wait 4200\nclosemenu", controlled)
        for name in ("slot_churn.cfg", "controlled_fifth_host.cfg"):
            fixture = (HERE / name).read_text()
            self.assertIn("connect 127.0.0.1:29973\n", fixture)
            self.assertIn("wait 4200\ncmd team free", fixture)

    def test_x86_delegation_preserves_all_split_cgame_modules(self):
        runner = (HERE / "run_remaining.sh").read_text()
        self.assertIn("cgame*x86_64.dylib", runner)
        self.assertIn('"$compat/codemp/cgame/"', runner)
        self.assertNotIn('cgamearm64.dylib"', runner)

    def test_main_runner_does_not_turn_failed_or_skipped_runs_into_passes(self):
        runner = (HERE / "run.sh").read_text()
        self.assertNotIn("|| true", runner)
        self.assertIn("public_status=skipped", runner)
        self.assertIn('write_manifest gp5-03-public-attempt "$public_status"', runner)

    def test_analyzer_and_validator_require_positive_completeness(self):
        analyzer = (HERE / "analyze.py").read_text()
        validator = (HERE / "validate.py").read_text()
        for marker in (
            'local.count("SplitProfileAssert: PASS") >= 12',
            'local.count("SplitInputAssertCmd: PASS") >= 11',
            'controlled.count("SplitNetStatAssert: PASS") >= 5',
            '"Four-player routed selection acceptance passed" in visible',
            'report["local_certification_passed"]',
        ):
            self.assertIn(marker, analyzer + validator)
        self.assertIn('all(cell["status"] == "COVERED_PASS" for cell in local_cells)', validator)
        self.assertIn('assert report["certification_passed"] is True', validator)
        self.assertNotIn('assert report["certification_passed"] is False', validator)

    def test_analyzer_fails_closed_when_positive_evidence_is_absent(self):
        with tempfile.TemporaryDirectory() as directory:
            run_root = Path(directory)
            output = run_root / "matrix.json"
            proc = subprocess.run(
                [
                    "python3",
                    str(HERE / "analyze.py"),
                    str(run_root),
                    str(output),
                    "0" * 64,
                ],
                check=False,
                capture_output=True,
                text=True,
            )
            self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
            report = json.loads(output.read_text())
            self.assertFalse(report["local_certification_passed"])
            self.assertFalse(report["certification_passed"])
            local = [
                cell
                for cell in report["cells"]
                if cell["cell"] != "safe_public_attempt"
            ]
            self.assertTrue(local)
            self.assertTrue(
                all(cell["status"] == "DISCOVERED_FAIL" for cell in local)
            )

    def test_owned_network_wrappers_use_dynamic_isolated_ports_and_e2e_manifests(self):
        churn = (HERE / "run_slot_churn.sh").read_text()
        fifth = (HERE / "run_controlled_fifth.sh").read_text()
        for runner in (churn, fifth):
            self.assertIn("run_e2e.sh", runner)
            self.assertIn("--validate-only", runner)
            self.assertIn("GP5_EXPECTED_SHA256", runner)
            self.assertIn("strict_scan.sh", runner)
        for launcher in (
            "launch_controlled_server.sh",
            "launch_slot_churn.sh",
            "launch_fifth_host.sh",
            "launch_fifth_client.sh",
        ):
            self.assertIn("OPENJK_E2E_", (HERE / launcher).read_text())

    def test_every_top_level_local_entry_verifies_all_frozen_artifacts(self):
        expected = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
        for name in (
            "run.sh",
            "run_remaining.sh",
            "run_slot_churn.sh",
            "run_controlled_fifth.sh",
            "run_modal.sh",
        ):
            runner = (HERE / name).read_text()
            self.assertIn(expected, runner, name)
            self.assertIn("verify_frozen_artifacts.py", runner, name)
            self.assertIn("--expected-client-hash", runner, name)

    def test_siege_fixture_prevents_explosive_classes_from_self_killing(self):
        runner = (HERE.parents[2] / "phase5/four-player/run_siege.sh").read_text()
        fixture = (HERE.parents[2] / "phase5/four-player/siege.cfg").read_text()
        self.assertIn("god\n", fixture)
        for player in (2, 3, 4):
            self.assertIn(f"splitnet_cmd {player} god", fixture)
        self.assertIn('rg -c "godmode ON$"', runner)


if __name__ == "__main__":
    unittest.main()
