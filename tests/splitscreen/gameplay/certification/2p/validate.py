#!/usr/bin/env python3
"""Strictly validate one current two-player local-certification suite."""

from hashlib import sha256
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from auxiliary import validate_auxiliary_rows  # noqa: E402


REQUIRED_MODES = (
    "ffa-2p",
    "holocron-2p",
    "jedimaster-2p",
    "duel-2p",
    "powerduel-2p",
    "ctf-2p",
    "cty-2p",
    "team-ffa-2p",
    "siege-2p",
)

MODE_MARKERS = {
    "ffa-2p": (
        "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetLifecycleAssert: PASS player=2 expected=SPECTATOR actual=SPECTATOR",
    ),
    "holocron-2p": (
        "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetLifecycleAssert: PASS player=2 expected=SPECTATOR actual=SPECTATOR",
    ),
    "jedimaster-2p": (
        "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetLifecycleAssert: PASS player=2 expected=SPECTATOR actual=SPECTATOR",
    ),
    "duel-2p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=3 actual=3",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "GP2-DUEL:COMPLETE",
    ),
    "powerduel-2p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=4 actual=4",
        "GP2-DUEL:POWER-CAPACITY",
        "GP2-DUEL:COMPLETE",
    ),
    "ctf-2p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
    ),
    "cty-2p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
    ),
    "team-ffa-2p": (
        "SplitNetStatAssert: PASS player=1 field=team op=eq expected=1 actual=1",
        "SplitNetStatAssert: PASS player=2 field=team op=eq expected=2 actual=2",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "GP5-01:INTERMISSION-END",
    ),
    "siege-2p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=7 actual=7",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "GP2-05:COMPLETE",
    ),
}


def digest(path: Path) -> str:
    return sha256(path.read_bytes()).hexdigest()


def key_values(path: Path) -> dict[str, list[list[str]]]:
    fields: dict[str, list[list[str]]] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split("\t")
        if len(parts) >= 2:
            fields.setdefault(parts[0], []).append(parts[1:])
    return fields


def validate_index(path: Path, run_id: str) -> tuple[bool, list[str]]:
    errors: list[str] = []
    if not path.is_file():
        return False, ["missing base evidence index"]
    fields = key_values(path)
    if fields.get("format") != [["openjk-cert-local-v1"]]:
        errors.append("invalid base evidence format")
    if fields.get("run_id") != [[run_id]]:
        errors.append("base evidence run_id mismatch")
    artifacts = fields.get("artifact", [])
    if not artifacts:
        errors.append("base evidence contains no artifacts")
    for row in artifacts:
        if len(row) != 2:
            errors.append("malformed base artifact row")
            continue
        artifact = Path(row[0])
        if not artifact.is_file() or not artifact.stat().st_size:
            errors.append(f"missing base artifact: {artifact}")
        elif digest(artifact) != row[1]:
            errors.append(f"base artifact hash mismatch: {artifact}")
    return not errors, errors


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
    fields = key_values(manifest)
    if fields.get("status") != [["passed"]]:
        errors.append(f"{case_name}: manifest status is not passed")
    if fields.get("case") != [[case_name]]:
        errors.append(f"{case_name}: manifest case mismatch")
    if fields.get("players") != [["2"]]:
        errors.append(f"{case_name}: manifest player count mismatch")
    hashes = fields.get("hash", [])
    if not any(len(row) == 2 and row[1] == expected_binary_sha for row in hashes):
        errors.append(f"{case_name}: frozen executable hash absent")
    log_rows = fields.get("process.client.log", [])
    if len(log_rows) != 1 or len(log_rows[0]) != 1:
        return False, errors + [f"{case_name}: authoritative client log absent"]
    log_path = Path(log_rows[0][0])
    if not log_path.is_file() or not log_path.stat().st_size:
        return False, errors + [f"{case_name}: authoritative client log missing"]
    artifact_rows = fields.get("artifact", [])
    recorded = {
        row[0]: row[1] for row in artifact_rows if len(row) == 2
    }
    if recorded.get(str(log_path)) != digest(log_path):
        errors.append(f"{case_name}: authoritative client log is not hash-verified")
    text = log_path.read_text(encoding="utf-8", errors="replace")
    for marker in MODE_MARKERS[case_name]:
        if marker not in text:
            errors.append(f"{case_name}: missing positive marker: {marker}")
    if re.search(r"Split\w*Assert\w*: FAIL|VM_CreateLegacy on cgame failed|Sys_Error", text):
        errors.append(f"{case_name}: failure marker present")
    screenshots = [
        (Path(row[0]), row[1])
        for row in artifact_rows
        if len(row) == 2 and row[0].endswith(".png")
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
    if len(sys.argv) not in (3, 4):
        raise SystemExit("usage: validate.py RESULTS EXPECTED_SHA [RUN_ID]")
    results = Path(sys.argv[1]).resolve()
    expected = sys.argv[2]
    root = results.parents[5]
    requested_run_id = sys.argv[3] if len(sys.argv) == 4 else ""
    if not requested_run_id:
        raise SystemExit("RUN_ID is required for strict current-suite validation")
    suite = results / "local-suite" / requested_run_id
    journey = suite / "journey"
    log_path = journey / "candidate.log"
    env_path = journey / "run.env"
    log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
    env = dict(
        line.split("=", 1)
        for line in env_path.read_text(encoding="utf-8").splitlines()
        if "=" in line
    ) if env_path.is_file() else {}
    run_id = requested_run_id
    started_epoch = int(env.get("started_epoch", "0") or 0)

    required_log = {
        "fresh_visible_ui": "SplitUIAssert: PASS cvar=ui_splitScreenPlayerCount expected=2 actual=2",
        "p1_keyboard_route": "SplitInputSim: key device=keyboard player=1",
        "p2_controller_route": "SplitInputSim: button device=controller1 player=2 button=0 pressed=1",
        "p1_kyle": "SplitUIAssert: PASS cvar=ui_splitScreenP1Model expected=kyle/default actual=kyle/default",
        "p1_dark": "SplitUIAssert: PASS cvar=ui_splitScreenP1ForcePowers expected=7-2-012320333000030321",
        "p2_sith": "SplitUIAssert: PASS cvar=ui_splitScreenP2Model expected=desann/default actual=desann/default",
        "p2_light": "SplitUIAssert: PASS cvar=ui_splitScreenP2ForcePowers expected=7-1-332300000330003230",
        "both_alive": "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "p2_attack": "SplitInputAssertCmd: PASS player=2",
        "p1_attack": "SplitInputAssertCmd: PASS player=1",
        "terminal_death": "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
    }
    checks = {name: marker in log for name, marker in required_log.items()}
    checks["candidate_log_present"] = log_path.is_file() and log_path.stat().st_size > 0
    checks["no_assert_failures"] = bool(log) and re.search(
        r"Split\w*Assert\w*: FAIL", log
    ) is None
    checks["suite_id"] = bool(run_id) and env.get("run_id") == run_id
    checks["suite_started"] = started_epoch > 0
    checks["hash_before"] = env.get("before_sha256") == expected
    checks["hash_after"] = env.get("after_sha256") == expected
    checks["clean_exit"] = env.get("process_exit") == "0"
    binary = root / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    checks["hash_current"] = binary.is_file() and digest(binary) == expected

    evidence_path = Path(env.get("evidence_index", "")) if env.get("evidence_index") else Path()
    evidence_ok, evidence_errors = validate_index(evidence_path, run_id)
    checks["current_base_artifacts"] = evidence_ok

    required_frames = (
        "routed_chain_01_main_menu.png",
        "routed_chain_02_multiplayer.png",
        "routed_chain_03_split_screen.png",
        "routed_chain_04_two_players.png",
        "routed_chain_05_player_setup.png",
        "routed_selection_p1_dark.png",
        "routed_selection_p2_sith.png",
        "routed_selection_p2_light.png",
        "routed_selection_profiles_final.png",
        "routed_selection_p2_attacks_kyle.png",
        "routed_selection_kyle_lightning.png",
        "routed_selection_kyle_lightning_kill.png",
    )
    frames = {}
    for name in required_frames:
        path = journey / "screenshots" / name
        frames[name] = {
            "present": path.is_file() and path.stat().st_size > 0,
            "sha256": digest(path) if path.is_file() else None,
            "bytes": path.stat().st_size if path.is_file() else 0,
        }
    checks["required_frames"] = all(item["present"] for item in frames.values())

    ledger = suite / "modes.tsv"
    ledger_rows = []
    if ledger.is_file():
        ledger_rows = [
            line.split("\t")
            for line in ledger.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
    by_case = {
        row[0]: row[1:]
        for row in ledger_rows
        if len(row) == 3 and row[0] in REQUIRED_MODES
    }
    mode_errors: dict[str, list[str]] = {}
    checks["mode_ledger_exact"] = (
        len(ledger_rows) == len(REQUIRED_MODES)
        and set(by_case) == set(REQUIRED_MODES)
    )
    for case_name in REQUIRED_MODES:
        row = by_case.get(case_name)
        if row is None:
            checks[f"mode_{case_name}"] = False
            mode_errors[case_name] = ["missing current-suite ledger row"]
            continue
        ok, errors = validate_mode_manifest(
            case_name, Path(row[0]), row[1], expected, started_epoch
        )
        checks[f"mode_{case_name}"] = ok
        mode_errors[case_name] = errors

    aux_ledger = suite / "aux.tsv"
    aux_rows = [
        line.split("\t")
        for line in aux_ledger.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ] if aux_ledger.is_file() else []
    aux_checks, aux_errors = validate_auxiliary_rows(
        aux_rows, run_id=run_id, players=2, expected_sha=expected,
        started_epoch=started_epoch,
    )
    checks["auxiliary_ledger_exact"] = aux_checks["auxiliary_ledger_exact"]
    checks["modal_ownership_current_suite"] = aux_checks["modal_ownership"]
    checks["virtual_controller_restart_current_suite"] = aux_checks[
        "virtual_controller_restart"
    ]
    checks["asymmetric_network_recovery_current_suite"] = aux_checks[
        "asymmetric_network_recovery"
    ]
    # The virtual bridge cannot emit SDL remove/add events. Keep physical
    # detach/reorder separate and hard-failed instead of laundering an
    # in_restart pass into physical-hotplug coverage.
    checks["physical_controller_hotplug_current_suite"] = False
    # The asymmetric probe intentionally ends after the secondary slot rejoins.
    # Historical full-party/server-loss runs contain real failures, so those
    # broader claims remain hard failures.
    checks["whole_party_server_loss_recovery_current_suite"] = False

    failed = [name for name, value in checks.items() if value is not True]
    status = "passed" if not failed else "failed"
    report = {
        "schema_version": 2,
        "ticket": "GP5-01",
        "status": status,
        "run_id": run_id,
        "frozen_sha256": expected,
        "checks": checks,
        "errors": {
            "base_evidence": evidence_errors,
            "modes": mode_errors,
            "auxiliary": aux_errors,
        },
        "screenshots": frames,
        "certification_gaps": [
            "physical controller disconnect/reconnect and enumeration reorder (SDL remove/add unsupported by the virtual bridge)",
            "whole-party reconnect and authoritative server-loss recovery (known controlled-run failures)",
            "real public server with remote humans",
            "resource/audio oracle",
        ],
    }
    (suite / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": status, "run_id": run_id, "failed": failed}, indent=2))
    return 0 if status == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
