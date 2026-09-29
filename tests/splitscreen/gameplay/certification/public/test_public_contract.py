#!/usr/bin/env python3
from __future__ import annotations

import base64
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


HERE = Path(__file__).resolve().parent
RUNNER = HERE / "run_serialized.sh"
QUERY = HERE / "query_server.py"
TARGET = "135.125.145.49:29070"
FROZEN = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"


class QueryContract(unittest.TestCase):
    def run_query(self, payload: object, players: int) -> tuple[int, dict]:
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "response.json"
            output = Path(temp) / "gate.json"
            source.write_text(json.dumps(payload))
            completed = subprocess.run(
                ["python3", str(QUERY), str(source), str(players), "--output", str(output)],
                check=False,
            )
            return completed.returncode, json.loads(output.read_text())

    def test_ready_requires_capacity_and_a_human(self):
        code, gate = self.run_query(
            [{"address": TARGET, "clients": 7, "maxClients": 32, "bots": 4}], 4
        )
        self.assertEqual(code, 0)
        self.assertEqual(gate["classification"], "READY")
        self.assertEqual(gate["nonlocal_humans"], 3)
        self.assertEqual(gate["free_slots"], 25)

    def test_target_keyed_response_is_supported(self):
        code, gate = self.run_query(
            {"servers": {TARGET: {"playerCount": 5, "maxPlayers": 16, "humanCount": 4}}},
            4,
        )
        self.assertEqual(code, 0)
        self.assertEqual(gate["classification"], "READY")

    def test_explicit_alternate_target_is_supported(self):
        alternate = "51.75.156.192:29070"
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "response.json"
            output = Path(temp) / "gate.json"
            source.write_text(
                json.dumps(
                    [
                        {
                            "address": alternate,
                            "clients": 1,
                            "maxClients": 23,
                            "humanCount": 1,
                        }
                    ]
                )
            )
            completed = subprocess.run(
                [
                    "python3",
                    str(QUERY),
                    str(source),
                    "4",
                    "--target",
                    alternate,
                    "--output",
                    str(output),
                ],
                check=False,
            )
            gate = json.loads(output.read_text())
        self.assertEqual(completed.returncode, 0)
        self.assertEqual(gate["classification"], "READY")
        self.assertEqual(gate["target"], alternate)

    def test_live_backend_base64_status_is_supported_conservatively(self):
        status = (
            b"\xff\xff\xff\xffstatusResponse\n"
            b"\\sv_hostname\\Public\\sv_maxclients\\8\\mapname\\mp/ffa3\n"
            b'0 0 "server message"\n'
            b'1 42 "real human"\n'
            b'0 18 "another human"\n'
        )
        code, gate = self.run_query(
            [
                {
                    "ip": "135.125.145.49",
                    "port": 29070,
                    "info": base64.b64encode(status).decode("ascii"),
                }
            ],
            4,
        )
        self.assertEqual(code, 0)
        self.assertEqual(gate["classification"], "READY")
        self.assertEqual(gate["occupied"], 3)
        self.assertEqual(gate["max_clients"], 8)
        self.assertEqual(gate["bots"], 1)
        self.assertEqual(gate["nonlocal_humans"], 2)
        self.assertEqual(gate["free_slots"], 5)

    def test_empty_server_is_blocked(self):
        code, gate = self.run_query(
            [{"ip": "135.125.145.49", "port": 29070, "players": [], "maxPlayers": 32}],
            2,
        )
        self.assertEqual(code, 78)
        self.assertEqual(gate["classification"], "BLOCKED_NO_NONLOCAL_HUMAN")

    def test_unknown_and_insufficient_capacity_are_blocked(self):
        code, gate = self.run_query([{"address": TARGET, "players": 3}], 3)
        self.assertEqual(code, 78)
        self.assertEqual(gate["classification"], "BLOCKED_CAPACITY_UNKNOWN")
        code, gate = self.run_query(
            [{"endpoint": TARGET, "playerCount": "30", "maxclients": "32", "humanCount": 2}],
            3,
        )
        self.assertEqual(code, 78)
        self.assertEqual(gate["classification"], "BLOCKED_INSUFFICIENT_CAPACITY")


class SerializedRunnerContract(unittest.TestCase):
    def setUp(self):
        lock = Path("/tmp/openjk-public-qa.lock")
        if lock.exists():
            self.skipTest("real public QA lock is active")

    def make_fixture(self, root: Path, humans: int = 2) -> Path:
        fixture = root / "serverlist.json"
        fixture.write_text(
            json.dumps(
                [
                    {
                        "address": TARGET,
                        "clients": humans + 1,
                        "maxClients": 32,
                        "bots": 1,
                    }
                ]
            )
        )
        return fixture

    def test_production_cases_have_a_reconnect_guard_cooldown(self):
        source = RUNNER.read_text()
        self.assertIn(
            'CASE_COOLDOWN_SECONDS="${GP5_PUBLIC_CASE_COOLDOWN_SECONDS:-12}"',
            source,
        )
        self.assertIn(
            'CASE_COOLDOWN_SECONDS="${GP5_PUBLIC_CASE_COOLDOWN_SECONDS:-0}"',
            source,
        )
        self.assertIn('sleep "$CASE_COOLDOWN_SECONDS"', source)

    def make_fake_runner(self, root: Path, reject_four: bool = False) -> Path:
        fake = root / "acceptance.sh"
        rejection = (
            'if [[ "$1" == 4 ]]; then echo "Too Many players with the same IP"; exit 1; fi'
            if reject_four
            else ":"
        )
        fake.write_text(
            "#!/usr/bin/env bash\n"
            "set -euo pipefail\n"
            'printf "%s\\n" "$1" >>"$CALL_LOG"\n'
            f"{rejection}\n"
        )
        fake.chmod(0o755)
        return fake

    def make_fake_verifier(self, root: Path, fail: bool = False) -> Path:
        fake = root / "verify.py"
        fake.write_text(
            "#!/usr/bin/env python3\n"
            "import os, pathlib, sys\n"
            'assert not pathlib.Path("/tmp/openjk-public-qa.lock").exists()\n'
            'pathlib.Path(os.environ["VERIFY_LOG"]).write_text(" ".join(sys.argv[1:]))\n'
            + (
                'print("FROZEN_ARTIFACTS_FAIL stale artifact")\nsys.exit(1)\n'
                if fail
                else 'print("FROZEN_ARTIFACTS_PASS artifacts=10")\n'
            )
        )
        fake.chmod(0o755)
        return fake

    def run_serialized(
        self,
        temp: Path,
        fake: Path,
        fixture: Path,
        allow: str = "1",
        expected: str | None = None,
        verifier_fails: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env.update(
            {
                "GP5_ALLOW_PUBLIC": allow,
                "GP5_PUBLIC_ACCEPTANCE_RUNNER": str(fake),
                "GP5_PUBLIC_QUERY_FIXTURE": str(fixture),
                "GP5_PUBLIC_OFFLINE_CONTRACT": "1",
                "GP5_PUBLIC_FROZEN_VERIFIER": str(
                    self.make_fake_verifier(temp, fail=verifier_fails)
                ),
                "GP5_PUBLIC_EVIDENCE_ROOT": str(temp / "evidence"),
                "CALL_LOG": str(temp / "calls.txt"),
                "VERIFY_LOG": str(temp / "verify.txt"),
            }
        )
        if expected is not None:
            env["GP5_EXPECTED_SHA256"] = expected
        else:
            env.pop("GP5_EXPECTED_SHA256", None)
        return subprocess.run(
            [str(RUNNER)], env=env, text=True, capture_output=True, check=False
        )

    def test_explicit_authorization_prevents_any_call(self):
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp, self.make_fake_runner(temp), self.make_fixture(temp), allow="0"
            )
            self.assertEqual(result.returncode, 77)
            self.assertFalse((temp / "calls.txt").exists())

    def test_runs_two_three_four_in_order_and_preserves_evidence(self):
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp, self.make_fake_runner(temp), self.make_fixture(temp)
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                (temp / "verify.txt").read_text(),
                f"--expected-client-hash {FROZEN}",
            )
            self.assertIn(
                "FROZEN_ARTIFACTS_PASS artifacts=10",
                (temp / "evidence" / "frozen-artifacts.log").read_text(),
            )
            self.assertEqual((temp / "calls.txt").read_text().splitlines(), ["2", "3", "4"])
            summary = (temp / "evidence" / "summary.tsv").read_text()
            self.assertEqual(summary.count("PASS_WITH_NONLOCAL_HUMAN"), 3)
            self.assertTrue((temp / "evidence" / "SHA256SUMS").is_file())
            for players in (2, 3, 4):
                case = temp / "evidence" / f"{players}p"
                self.assertTrue((case / "serverlist.raw.json").is_file())
                self.assertTrue((case / "capacity.json").is_file())
                self.assertTrue((case / "runner.stdout.log").is_file())

    def test_no_human_blocks_without_calling_acceptance(self):
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp,
                self.make_fake_runner(temp),
                self.make_fixture(temp, humans=0),
            )
            self.assertEqual(result.returncode, 78)
            self.assertFalse((temp / "calls.txt").exists())
            self.assertTrue((temp / "verify.txt").is_file())
            summary = (temp / "evidence" / "summary.tsv").read_text()
            self.assertEqual(summary.count("BLOCKED_NO_NONLOCAL_HUMAN"), 3)

    def test_frozen_artifact_failure_precedes_lock_query_and_runner(self):
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp,
                self.make_fake_runner(temp),
                self.make_fixture(temp),
                verifier_fails=True,
            )
            self.assertEqual(result.returncode, 2)
            self.assertIn("frozen artifact gate failed before public lock/query", result.stderr)
            self.assertFalse((temp / "calls.txt").exists())
            self.assertFalse((temp / "evidence" / "2p").exists())
            self.assertFalse(Path("/tmp/openjk-public-qa.lock").exists())

    def test_four_player_same_ip_rejection_is_classified_not_passed(self):
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp,
                self.make_fake_runner(temp, reject_four=True),
                self.make_fixture(temp),
            )
            self.assertEqual(result.returncode, 78)
            self.assertEqual((temp / "calls.txt").read_text().splitlines(), ["2", "3", "4"])
            self.assertEqual(
                (temp / "evidence" / "4p" / "result.txt").read_text().strip(),
                "BLOCKED_SERVER_SAME_IP_LIMIT",
            )
            self.assertNotIn(
                "4\tPASS_WITH_NONLOCAL_HUMAN",
                (temp / "evidence" / "summary.tsv").read_text(),
            )

    def test_hash_override_is_available_only_to_offline_contract(self):
        override = "a" * 64
        with tempfile.TemporaryDirectory() as raw:
            temp = Path(raw)
            result = self.run_serialized(
                temp,
                self.make_fake_runner(temp),
                self.make_fixture(temp),
                expected=override,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(
                (temp / "verify.txt").read_text(),
                f"--expected-client-hash {override}",
            )

        env = os.environ.copy()
        env.update({"GP5_ALLOW_PUBLIC": "1", "GP5_EXPECTED_SHA256": override})
        env.pop("GP5_PUBLIC_OFFLINE_CONTRACT", None)
        result = subprocess.run(
            [str(RUNNER)], env=env, text=True, capture_output=True, check=False
        )
        self.assertEqual(result.returncode, 2)
        self.assertIn("requires frozen client", result.stderr)

    def test_static_safety_contract(self):
        source = RUNNER.read_text()
        self.assertIn('DEFAULT_TARGET="135.125.145.49:29070"', source)
        self.assertIn('TARGET="${GP5_PUBLIC_TARGET:-$DEFAULT_TARGET}"', source)
        self.assertIn('LOCK="/tmp/openjk-public-qa.lock"', source)
        self.assertIn(f'FROZEN_CLIENT_SHA256="{FROZEN}"', source)
        self.assertIn("verify_frozen_artifacts.py", source)
        self.assertIn("GP5_ALLOW_PUBLIC", source)
        self.assertIn("https://jknexus.se/api/serverlist", source)
        self.assertIn("for players in 2 3 4", source)
        self.assertIn('"$RUNNER" "$players"', source)
        acceptance = (HERE.parents[2] / "run_routed_public_acceptance.sh").read_text()
        self.assertIn(
            'sed "s|$CONFIGURED_PUBLIC_SERVER|$PUBLIC_SERVER|g"',
            acceptance,
        )
        for players in (2, 3, 4):
            config = (
                HERE.parents[2]
                / "cfg"
                / f"routed_public_{players}p_acceptance.cfg"
            ).read_text()
            self.assertNotIn("team free", config)
            self.assertIn("cmd team auto", config)


if __name__ == "__main__":
    unittest.main()
