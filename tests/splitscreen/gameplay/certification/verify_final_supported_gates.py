#!/usr/bin/env python3
"""Fail-closed aggregate verifier for the frozen split-screen runtime gates."""

from __future__ import annotations

import csv
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[4]
EXPECTED = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
RESULTS = ROOT / "tests/splitscreen/gameplay/results/certification"
OUTPUT = RESULTS / "final/final-supported-gates.json"
KNOWN_EXACT_SUITE_DELEGATIONS = {
    "physical_controller_hotplug_current_suite",
    "whole_party_server_loss_recovery_current_suite",
}


def digest(path: Path) -> str:
    return sha256(path.read_bytes()).hexdigest()


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def load_json(path: Path, failures: list[str]) -> dict:
    if not path.is_file():
        failures.append(f"missing JSON evidence: {path}")
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"invalid JSON evidence: {path}: {exc}")
        return {}


def verify_frozen_artifacts(failures: list[str]) -> dict:
    path = (
        ROOT
        / "docs/splitscreen-gameplay-hardening/phase-5-frozen-certification"
        / "frozen-build.json"
    )
    data = load_json(path, failures)
    artifacts = data.get("artifacts", {})
    require(len(artifacts) == 10, "frozen manifest must contain ten artifacts", failures)
    for relative, expected in artifacts.items():
        artifact = ROOT / relative
        require(artifact.is_file(), f"missing frozen artifact: {relative}", failures)
        if artifact.is_file():
            require(
                digest(artifact) == expected,
                f"frozen artifact hash mismatch: {relative}",
                failures,
            )
    client = "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    require(artifacts.get(client) == EXPECTED, "wrong frozen client hash", failures)
    return {"manifest": str(path), "artifacts": len(artifacts)}


def verify_exact_suite(players: int, failures: list[str]) -> dict:
    run_id = f"final-737f-{players}p"
    path = RESULTS / f"{players}p/local-suite/{run_id}/result.json"
    data = load_json(path, failures)
    checks = data.get("checks", {})
    require(data.get("run_id") == run_id, f"{players}p suite ID mismatch", failures)
    require(
        data.get("frozen_sha256") == EXPECTED,
        f"{players}p suite frozen hash mismatch",
        failures,
    )
    false_checks = {name for name, value in checks.items() if value is not True}
    require(
        false_checks == KNOWN_EXACT_SUITE_DELEGATIONS,
        f"{players}p unexpected failed checks: {sorted(false_checks)}",
        failures,
    )
    mode_errors = data.get("errors", {}).get("modes", {})
    require(
        mode_errors and all(not errors for errors in mode_errors.values()),
        f"{players}p mode errors are not empty",
        failures,
    )
    return {
        "result": str(path),
        "supported_checks": len(checks) - len(false_checks),
        "delegated_checks": sorted(false_checks),
    }


def verify_four_player(failures: list[str]) -> dict:
    matrix_path = RESULTS / "4p/matrix.json"
    matrix = load_json(matrix_path, failures)
    require(
        matrix.get("frozen_binary_sha256") == EXPECTED,
        "4p aggregate frozen hash mismatch",
        failures,
    )
    require(
        matrix.get("summary") == {"BLOCKED_EXTERNAL": 1, "COVERED_PASS": 7},
        "4p aggregate summary mismatch",
        failures,
    )
    require(
        matrix.get("local_certification_passed") is True,
        "4p local aggregate did not pass",
        failures,
    )
    modes_path = RESULTS / "4p/local-suite/final-737f-4p/modes-result.json"
    modes = load_json(modes_path, failures)
    require(modes.get("status") == "passed", "4p nine-mode matrix did not pass", failures)
    require(
        modes.get("frozen_sha256") == EXPECTED,
        "4p mode matrix frozen hash mismatch",
        failures,
    )
    return {"aggregate": str(matrix_path), "modes": str(modes_path)}


def verify_mouse(failures: list[str]) -> dict:
    path = RESULTS / "final/mouse-focus-737f/matrix.json"
    data = load_json(path, failures)
    expected = {"COVERED_PASS": 76, "DISCOVERED_FAIL": 0, "BLOCKED": 15}
    require(data.get("summary") == expected, "mouse-focus summary mismatch", failures)
    return {"matrix": str(path), "summary": data.get("summary")}


def verify_public(failures: list[str]) -> dict:
    root = RESULTS / "final/public-active-vanilla"
    checksums = root / "SHA256SUMS"
    if checksums.is_file():
        proc = subprocess.run(
            ["shasum", "-a", "256", "-c", checksums.name],
            cwd=root,
            check=False,
            capture_output=True,
            text=True,
        )
        require(proc.returncode == 0, "public aggregate checksum verification failed", failures)
    else:
        failures.append(f"missing public checksum index: {checksums}")
    summary = root / "summary.tsv"
    rows = []
    if summary.is_file():
        with summary.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.DictReader(stream, delimiter="\t"))
    require(
        {row.get("players") for row in rows} == {"2", "3", "4"},
        "public aggregate does not contain 2p/3p/4p",
        failures,
    )
    require(
        rows
        and all(
            row.get("classification") == "PASS_WITH_NONLOCAL_HUMAN"
            for row in rows
        ),
        "public aggregate lacks a nonlocal-human pass",
        failures,
    )
    return {"summary": str(summary), "players": [row.get("players") for row in rows]}


def verify_recovery(failures: list[str]) -> dict:
    path = ROOT / "tests/splitscreen/gameplay/results/network/recovery/matrix.json"
    data = load_json(path, failures)
    require(
        data.get("frozen_binary_sha256") == EXPECTED,
        "recovery matrix frozen hash mismatch",
        failures,
    )
    require(
        data.get("runtime_acceptance_met") is True,
        "supported recovery runtime phases did not all pass",
        failures,
    )
    summary = data.get("summary", {})
    require(summary.get("COVERED_PASS") == 21, "recovery must pass 21 runtime cells", failures)
    require(
        summary.get("DISCOVERED_FAIL", 0) == 0
        and summary.get("BLOCKED_PREREQUISITE", 0) == 0,
        "recovery matrix contains a runtime failure or blocked prerequisite",
        failures,
    )
    require(
        summary.get("BLOCKED_NOT_COVERED") == 27,
        "recovery unsupported-cell accounting mismatch",
        failures,
    )
    return {"matrix": str(path), "summary": summary}


def main() -> int:
    failures: list[str] = []
    evidence = {
        "frozen": verify_frozen_artifacts(failures),
        "two_player": verify_exact_suite(2, failures),
        "three_player": verify_exact_suite(3, failures),
        "four_player": verify_four_player(failures),
        "mouse_focus": verify_mouse(failures),
        "public_server": verify_public(failures),
        "recovery": verify_recovery(failures),
    }
    report = {
        "schema_version": 1,
        "frozen_client_sha256": EXPECTED,
        "supported_runtime_gates_passed": not failures,
        "evidence": evidence,
        "cross_suite_closures": {
            "whole_party_server_loss_recovery_current_suite": (
                "closed by the shared 2p/3p/4p GP3-03 recovery matrix"
            )
        },
        "explicitly_unsupported": [
            "physical SDL hotplug, enumeration reorder, and persistent remap",
            "credential/capacity/protocol/pure/admin/packet-loss rejection fixtures",
            "physical audio-device removal and audible attribution",
            "long soak, sanitizer, performance, sleep/wake, and packaged-build gates",
        ],
        "failures": failures,
    }
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"FINAL_SUPPORTED_GATE_FAIL: {failure}", file=sys.stderr)
        return 1
    print(
        "FINAL_SUPPORTED_GATES_PASS "
        f"client={EXPECTED} public=2p,3p,4p recovery=21/21 mouse=76/76"
    )
    print(OUTPUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
