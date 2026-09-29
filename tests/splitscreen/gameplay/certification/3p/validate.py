#!/usr/bin/env python3
"""Strictly validate one current three-player local-certification suite."""

from hashlib import sha256
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from auxiliary import validate_auxiliary_rows  # noqa: E402


REQUIRED_MODES = (
    "ffa-3p",
    "holocron-3p",
    "jedimaster-3p",
    "duel-3p",
    "powerduel-3p",
    "ctf-3p",
    "cty-3p",
    "team-ffa-3p",
    "siege-3p",
)

INDIVIDUAL_MARKERS = (
    "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active",
    "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
    "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
    "SplitNetLifecycleAssert: PASS player=3 expected=SPECTATOR actual=SPECTATOR",
)
MODE_MARKERS = {
    "ffa-3p": INDIVIDUAL_MARKERS,
    "holocron-3p": INDIVIDUAL_MARKERS,
    "jedimaster-3p": INDIVIDUAL_MARKERS,
    "duel-3p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=3 actual=3",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=3 expected=SPECTATOR actual=SPECTATOR",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "GP2-DUEL:COMPLETE",
    ),
    "powerduel-3p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=4 actual=4",
        "GP2-DUEL:POWER-CAPACITY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "GP2-DUEL:COMPLETE",
    ),
    "ctf-3p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
    ),
    "cty-3p": (
        "ObjectiveLifecycle: READY",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10",
        "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION",
        "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
    ),
    "team-ffa-3p": (
        "SplitNetStatAssert: PASS player=1 field=team op=eq expected=1 actual=1",
        "SplitNetStatAssert: PASS player=2 field=team op=eq expected=2 actual=2",
        "SplitNetStatAssert: PASS player=3 field=team op=eq expected=1 actual=1",
        "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "SplitNetStatAssert: PASS player=1 field=score op=ge expected=1",
        "GP2-03:COMPLETE",
    ),
    "siege-3p": (
        "SplitUIAssert: PASS cvar=g_gametype expected=7 actual=7",
        "SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE",
        "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE",
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


def validate_index(path: Path, run_id: str) -> tuple[bool, list[str]]:
    errors: list[str] = []
    if not path.is_file():
        return False, ["missing evidence index"]
    data = fields(path)
    if data.get("format") != [["openjk-cert-local-v1"]]:
        errors.append("invalid evidence format")
    if data.get("run_id") != [[run_id]]:
        errors.append("evidence run_id mismatch")
    artifacts = data.get("artifact", [])
    if not artifacts:
        errors.append("evidence contains no artifacts")
    for row in artifacts:
        if len(row) != 2:
            errors.append("malformed artifact row")
            continue
        artifact = Path(row[0])
        if not artifact.is_file() or not artifact.stat().st_size:
            errors.append(f"missing artifact: {artifact}")
        elif digest(artifact) != row[1]:
            errors.append(f"artifact hash mismatch: {artifact}")
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
    data = fields(manifest)
    if data.get("status") != [["passed"]]:
        errors.append(f"{case_name}: manifest status is not passed")
    if data.get("case") != [[case_name]]:
        errors.append(f"{case_name}: manifest case mismatch")
    if data.get("players") != [["3"]]:
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
    artifacts = {
        row[0]: row[1] for row in data.get("artifact", []) if len(row) == 2
    }
    if artifacts.get(str(log)) != digest(log):
        errors.append(f"{case_name}: authoritative client log is not hash-verified")
    text = log.read_text(encoding="utf-8", errors="replace")
    for marker in MODE_MARKERS[case_name]:
        if marker not in text:
            errors.append(f"{case_name}: missing positive marker: {marker}")
    if re.search(r"Split\w*Assert\w*: FAIL|VM_CreateLegacy on cgame failed|Sys_Error", text):
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

    markers = {
        "fresh_visible_ui": "SplitUIAssert: PASS cvar=ui_splitScreenPlayerCount expected=3 actual=3",
        "p1_keyboard": "SplitUIAssert: PASS cvar=ui_splitScreenP1Input expected=keyboard actual=keyboard",
        "p2_controller1": "SplitUIAssert: PASS cvar=ui_splitScreenP2Input expected=controller1 actual=controller1",
        "p3_controller2": "SplitUIAssert: PASS cvar=ui_splitScreenP3Input expected=controller2 actual=controller2",
        "p1_model": "SplitUIAssert: PASS cvar=ui_splitScreenP1Model expected=kyle/default actual=kyle/default",
        "p2_model": "SplitUIAssert: PASS cvar=ui_splitScreenP2Model expected=desann/default actual=desann/default",
        "p3_model": "SplitUIAssert: PASS cvar=ui_splitScreenP3Model expected=reborn/default actual=reborn/default",
        "p1_force": "SplitUIAssert: PASS cvar=ui_splitScreenP1ForcePowers expected=7-2-012320333000030321",
        "p2_force": "SplitUIAssert: PASS cvar=ui_splitScreenP2ForcePowers expected=7-1-332300000330003230",
        "p3_force": "SplitUIAssert: PASS cvar=ui_splitScreenP3ForcePowers expected=7-1-332300000330003230",
        "three_live_clients": "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE",
        "p2_attack_isolation": "SplitInputSim: button device=controller1 player=2 button=0 pressed=1",
        "p3_attack_isolation": "SplitInputSim: button device=controller2 player=3 button=0 pressed=1",
        "p1_keyboard_attack": "SplitInputSim: key device=keyboard player=1",
        "p2_terminal_death": "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD",
        "intermission": "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION",
    }
    checks = {name: marker in log for name, marker in markers.items()}
    checks["candidate_log_present"] = log_path.is_file() and log_path.stat().st_size > 0
    checks["no_assert_failures"] = bool(log) and re.search(
        r"Split(?:UI|Input|NetLifecycle|NetStat|Profile)Assert: FAIL", log
    ) is None
    checks["suite_id"] = bool(run_id) and env.get("run_id") == run_id
    checks["suite_started"] = started_epoch > 0
    checks["hash_before"] = env.get("before_sha256") == expected
    checks["hash_after"] = env.get("after_sha256") == expected
    checks["clean_exit"] = env.get("process_exit") == "0"
    binary = root / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    checks["hash_current"] = binary.is_file() and digest(binary) == expected

    base_index = Path(env.get("evidence_index", "")) if env.get("evidence_index") else Path()
    base_ok, base_errors = validate_index(base_index, run_id)
    checks["current_base_artifacts"] = base_ok

    required_frames = (
        "routed3_chain_01_main_menu.png",
        "routed3_chain_02_multiplayer.png",
        "routed3_chain_03_split_screen.png",
        "routed3_chain_04_three_players.png",
        "routed3_chain_05_player_setup.png",
        "routed3_selection_p1_kyle_dark.png",
        "routed3_selection_p2_desann.png",
        "routed3_selection_p2_light.png",
        "routed3_selection_p3_reborn.png",
        "routed3_selection_p3_light.png",
        "routed3_selection_profiles_final.png",
        "routed3_gameplay_all_alive.png",
        "routed3_gameplay_p2_attacks_p1.png",
        "routed3_gameplay_p3_attacks_p1.png",
        "routed3_gameplay_kyle_lightning_p2.png",
        "routed3_gameplay_p2_down.png",
        "routed3_gameplay_kyle_lightning_p3.png",
        "routed3_gameplay_p3_down.png",
        "routed3_match_complete.png",
    )
    frames = {}
    for name in required_frames:
        path = journey / "screenshots" / name
        frames[name] = {
            "present": path.is_file() and path.stat().st_size > 0,
            "sha256": digest(path) if path.is_file() else None,
            "bytes": path.stat().st_size if path.is_file() else 0,
        }
    checks["required_frames"] = all(frame["present"] for frame in frames.values())

    aux_ledger = suite / "aux.tsv"
    aux_rows = [
        line.split("\t")
        for line in aux_ledger.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ] if aux_ledger.is_file() else []
    lifecycle_rows = [row for row in aux_rows if len(row) == 3 and row[0] == "lifecycle"]
    lifecycle_errors: list[str] = []
    if len(lifecycle_rows) != 1:
        checks["current_lifecycle_artifacts"] = False
        lifecycle_errors.append("expected exactly one current-suite lifecycle index")
    else:
        lifecycle_index = Path(lifecycle_rows[0][1])
        if not lifecycle_index.is_file() or digest(lifecycle_index) != lifecycle_rows[0][2]:
            checks["current_lifecycle_artifacts"] = False
            lifecycle_errors.append("lifecycle index missing or hash mismatch")
        else:
            checks["current_lifecycle_artifacts"], lifecycle_errors = validate_index(
                lifecycle_index, run_id
            )
    lifecycle_log = suite / "lifecycle" / "candidate.log"
    lifecycle_text = lifecycle_log.read_text(
        encoding="utf-8", errors="replace"
    ) if lifecycle_log.is_file() else ""
    checks["spectate_rejoin_restart_nextmap"] = (
        checks["current_lifecycle_artifacts"]
        and lifecycle_text.count("SplitNetLifecycleAssert: PASS") >= 14
        and "expected=SPECTATOR actual=SPECTATOR" in lifecycle_text
        and "SplitNetLifecycleAssert: FAIL" not in lifecycle_text
        and all(
            (suite / "lifecycle" / "screenshots" / name).is_file()
            and (suite / "lifecycle" / "screenshots" / name).stat().st_size > 0
            for name in (
                "splitqa_join_spectate_flow.png",
                "gp5_3p_restart.png",
                "gp5_3p_next_map.png",
            )
        )
    )

    mode_ledger = suite / "modes.tsv"
    mode_rows = [
        line.split("\t")
        for line in mode_ledger.read_text(encoding="utf-8").splitlines()
        if line.strip()
    ] if mode_ledger.is_file() else []
    by_case = {
        row[0]: row[1:]
        for row in mode_rows
        if len(row) == 3 and row[0] in REQUIRED_MODES
    }
    checks["mode_ledger_exact"] = (
        len(mode_rows) == len(REQUIRED_MODES)
        and set(by_case) == set(REQUIRED_MODES)
    )
    mode_errors: dict[str, list[str]] = {}
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

    aux_checks, aux_errors = validate_auxiliary_rows(
        aux_rows, run_id=run_id, players=3, expected_sha=expected,
        started_epoch=started_epoch,
    )
    checks["auxiliary_ledger_exact"] = (
        aux_checks["auxiliary_ledger_exact"]
        and len(aux_rows) == 4
        and {row[0] for row in aux_rows if row} == {
            "lifecycle",
            "modal_ownership",
            "virtual_controller_restart",
            "asymmetric_network_recovery",
        }
    )
    checks["modal_ownership_current_suite"] = aux_checks["modal_ownership"]
    checks["virtual_controller_restart_current_suite"] = aux_checks[
        "virtual_controller_restart"
    ]
    checks["asymmetric_network_recovery_current_suite"] = aux_checks[
        "asymmetric_network_recovery"
    ]
    checks["physical_controller_hotplug_current_suite"] = False
    checks["whole_party_server_loss_recovery_current_suite"] = False

    failed = [name for name, value in checks.items() if value is not True]
    status = "passed" if not failed else "failed"
    report = {
        "schema_version": 2,
        "ticket": "GP5-02",
        "status": status,
        "run_id": run_id,
        "frozen_sha256": expected,
        "checks": checks,
        "errors": {
            "base_evidence": base_errors,
            "lifecycle": lifecycle_errors,
            "modes": mode_errors,
            "auxiliary": aux_errors,
        },
        "screenshots": frames,
        "certification_gaps": [
            "physical controller disconnect/reconnect and enumeration reorder (SDL remove/add unsupported by the virtual bridge)",
            "whole-party reconnect and authoritative server-loss recovery (known controlled-run failures)",
            "real public server with remote humans",
            "explicit HUD/camera/chat/console/audio/resource oracles",
        ],
    }
    (suite / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": status, "run_id": run_id, "failed": failed}, indent=2))
    return 0 if status == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
