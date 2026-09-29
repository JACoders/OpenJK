#!/usr/bin/env python3
"""Strictly validate one current four-player stock-mode certification suite."""

from __future__ import annotations

from hashlib import sha256
import json
from pathlib import Path
import re
import sys


REQUIRED_MODES = (
    "ffa-4p",
    "holocron-4p",
    "jedimaster-4p",
    "duel-4p",
    "powerduel-4p",
    "ctf-4p",
    "cty-4p",
    "team-ffa-4p",
    "siege-4p",
)

INDIVIDUAL_MARKERS = (
    "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active",
    "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
    "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
    "SplitNetLifecycleAssert: PASS player=4 expected=SPECTATOR actual=SPECTATOR",
)
MODE_MARKERS = {
    "ffa-4p": INDIVIDUAL_MARKERS,
    "holocron-4p": INDIVIDUAL_MARKERS,
    "jedimaster-4p": INDIVIDUAL_MARKERS,
    "duel-4p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=3 actual=3",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=3 expected=SPECTATOR actual=SPECTATOR",
        "SplitNetLifecycleAssert: PASS player=4 expected=SPECTATOR actual=SPECTATOR",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "GP2-DUEL:COMPLETE",
    ),
    "powerduel-4p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=4 actual=4",
        "GP2-DUEL:POWER-CAPACITY",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=4 expected=INTERMISSION actual=INTERMISSION",
        "GP2-DUEL:POWER-UNSUPPORTED-CAPACITY",
        "GP2-DUEL:COMPLETE",
    ),
    "ctf-4p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=4 expected=INTERMISSION actual=INTERMISSION",
    ),
    "cty-4p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=4 expected=INTERMISSION actual=INTERMISSION",
    ),
    "team-ffa-4p": (
        "SplitNetStatAssert: PASS player=1 field=team op=eq expected=1 actual=1",
        "SplitNetStatAssert: PASS player=2 field=team op=eq expected=2 actual=2",
        "SplitNetStatAssert: PASS player=3 field=team op=eq expected=1 actual=1",
        "SplitNetStatAssert: PASS player=4 field=team op=eq expected=2 actual=2",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=4 expected=INTERMISSION actual=INTERMISSION",
        "GP2-03:INTERMISSION-END",
        "GP2-03:COMPLETE",
    ),
    "siege-4p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=7 actual=7",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "GP2-05:COMPLETE",
    ),
}


def digest(path: Path) -> str:
    return sha256(path.read_bytes()).hexdigest()


def fields(path: Path) -> dict[str, list[list[str]]]:
    parsed: dict[str, list[list[str]]] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split("\t")
        if len(parts) >= 2:
            parsed.setdefault(parts[0], []).append(parts[1:])
    return parsed


def validate_mode_manifest(
    case_name: str,
    manifest: Path,
    manifest_sha: str,
    expected_binary_sha: str,
    started_epoch: int,
) -> tuple[bool, list[str]]:
    errors: list[str] = []
    if not manifest.is_file():
        return False, [f"{case_name}: missing manifest"]
    if digest(manifest) != manifest_sha:
        errors.append(f"{case_name}: ledger hash mismatch")
    if int(manifest.stat().st_mtime) < started_epoch:
        errors.append(f"{case_name}: manifest predates suite")
    data = fields(manifest)
    if data.get("status") != [["passed"]]:
        errors.append(f"{case_name}: manifest status is not passed")
    if data.get("case") != [[case_name]]:
        errors.append(f"{case_name}: manifest case mismatch")
    if data.get("players") != [["4"]]:
        errors.append(f"{case_name}: manifest player count mismatch")
    if not any(
        len(row) == 2 and row[1] == expected_binary_sha
        for row in data.get("hash", [])
    ):
        errors.append(f"{case_name}: frozen executable hash absent")
    log_rows = data.get("process.client.log", [])
    if len(log_rows) != 1 or len(log_rows[0]) != 1:
        return False, errors + [f"{case_name}: authoritative client log absent"]
    log = Path(log_rows[0][0])
    if not log.is_file() or not log.stat().st_size:
        return False, errors + [f"{case_name}: authoritative client log missing"]
    artifact_rows = data.get("artifact", [])
    artifacts = {row[0]: row[1] for row in artifact_rows if len(row) == 2}
    for row in artifact_rows:
        if len(row) != 2:
            errors.append(f"{case_name}: malformed artifact row")
            continue
        artifact = Path(row[0])
        if not artifact.is_file() or not artifact.stat().st_size:
            errors.append(f"{case_name}: missing artifact: {artifact}")
        elif digest(artifact) != row[1]:
            errors.append(f"{case_name}: artifact hash mismatch: {artifact}")
    if artifacts.get(str(log)) != digest(log):
        errors.append(f"{case_name}: authoritative client log is not hash-verified")
    text = log.read_text(encoding="utf-8", errors="replace")
    for marker in MODE_MARKERS[case_name]:
        if marker not in text:
            errors.append(f"{case_name}: missing positive marker: {marker}")
    if re.search(
        r"Split\w*Assert\w*: FAIL|SplitNetStagePair: FAIL|"
        r"VM_CreateLegacy on cgame failed|Sys_Error",
        text,
    ):
        errors.append(f"{case_name}: failure marker present")
    screenshots = [
        (Path(path), recorded_sha)
        for path, recorded_sha in artifacts.items()
        if path.endswith(".png")
    ]
    if len(screenshots) < 2 or any(
        not shot.is_file()
        or not shot.stat().st_size
        or digest(shot) != recorded_sha
        for shot, recorded_sha in screenshots
    ):
        errors.append(f"{case_name}: fewer than two hash-recorded screenshots")
    return not errors, errors


def main() -> int:
    if len(sys.argv) != 4:
        raise SystemExit("usage: validate_modes.py RESULTS EXPECTED_SHA RUN_ID")
    results = Path(sys.argv[1]).resolve()
    expected = sys.argv[2]
    run_id = sys.argv[3]
    if not re.fullmatch(r"[A-Za-z0-9._-]+", run_id):
        raise SystemExit("invalid RUN_ID")
    suite = results / "local-suite" / run_id
    env_path = suite / "modes.env"
    env = dict(
        line.split("=", 1)
        for line in env_path.read_text(encoding="utf-8").splitlines()
        if "=" in line
    ) if env_path.is_file() else {}
    started_epoch = int(env.get("started_epoch", "0") or 0)
    checks = {
        "env_format": env.get("format") == "openjk-cert-modes-v1",
        "suite_id": env.get("run_id") == run_id,
        "suite_started": started_epoch > 0,
        "hash_before": env.get("before_sha256") == expected,
        "hash_after": env.get("after_sha256") == expected,
    }
    root = results.parents[5]
    binary = root / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    checks["hash_current"] = binary.is_file() and digest(binary) == expected

    ledger = suite / "modes.tsv"
    rows = [
        line.split("\t")
        for line in ledger.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ] if ledger.is_file() else []
    by_case = {
        row[0]: row[1:]
        for row in rows
        if len(row) == 3 and row[0] in REQUIRED_MODES
    }
    checks["mode_ledger_exact"] = (
        len(rows) == len(REQUIRED_MODES)
        and set(by_case) == set(REQUIRED_MODES)
    )
    errors: dict[str, list[str]] = {}
    for case_name in REQUIRED_MODES:
        row = by_case.get(case_name)
        if row is None:
            checks[f"mode_{case_name}"] = False
            errors[case_name] = ["missing current-suite ledger row"]
            continue
        ok, case_errors = validate_mode_manifest(
            case_name, Path(row[0]), row[1], expected, started_epoch
        )
        checks[f"mode_{case_name}"] = ok
        errors[case_name] = case_errors

    failed = [name for name, passed in checks.items() if passed is not True]
    status = "passed" if not failed else "failed"
    report = {
        "schema_version": 1,
        "ticket": "GP5-03-4P-STOCK-MODES",
        "status": status,
        "run_id": run_id,
        "frozen_sha256": expected,
        "required_modes": list(REQUIRED_MODES),
        "checks": checks,
        "errors": errors,
    }
    suite.mkdir(parents=True, exist_ok=True)
    (suite / "modes-result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": status, "run_id": run_id, "failed": failed}, indent=2))
    return 0 if status == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
