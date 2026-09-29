#!/usr/bin/env python3
"""Manifest-driven split-screen state, media, audio, and resource oracle."""

from __future__ import annotations

import argparse
import hashlib
import json
import statistics
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

SCHEMA = 1


@dataclass
class Finding:
    code: str
    player: int | str
    phase: str
    expected: object
    actual: object


def _load_json(path: Path) -> object:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _finding(findings, code, player, phase, expected, actual):
    findings.append(Finding(code, player, phase, expected, actual))


def _materialize(case: dict, phase: str) -> dict:
    """Expand the compact, reviewable fixture descriptor into deterministic evidence."""
    players = case["players"]
    clients = []
    panes = []
    audio_events = []
    models = ["kyle/default", "desann/default", "reborn/default", "tavion/default"]
    for player in range(1, players + 1):
        clients.append({
            "player": player,
            "slot": player - 1,
            "qport": 41000 + player,
            "snapshot_id": 900 + player,
            "snapshot_time": 9950,
            "lifecycle": "ALIVE",
            "score": 1,
            "team": "FREE",
            "objective": "NONE",
            "userinfo": {"model": models[player - 1]},
            "reliable_commands": [f"ack:{phase}:p{player}"],
        })
        panes.append({
            "player": player,
            "signature": f"pane-{player}-sha256",
            "hud_owner": player,
            "cursor_confined": True,
            "boundary_ok": True,
        })
        audio_events.extend([
            {"player": player, "event": "weapon_fire"},
            {"player": player, "event": "announcer_score"},
        ])
    evidence = {
        "state": {
            "complete": True,
            "server_time": 10000,
            "clean_disconnect": True,
            "clients": clients,
        },
        "image": {
            "blank_fraction": 0.01,
            "fade_fraction": 0.01,
            "menu": "gameplay",
            "panes": panes,
        },
        "audio": {"events": audio_events},
        "resources": {
            "rss_samples_kb": [200000, 201000, 201500],
            "fds_baseline": 40,
            "fds_final": 41,
            "sockets_final": 0,
            "threads_baseline": 12,
            "threads_final": 12,
            "frame_times_ms": [8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                               8, 9, 10, 11, 12, 13, 14, 15, 16, 17],
            "renderer_restarts": 1,
            "vm_restarts": players,
            "crash_markers": [],
        },
    }
    mutation = case.get("mutation")
    if mutation == "duplicate":
        evidence["image"]["panes"][1]["signature"] = evidence["image"]["panes"][0]["signature"]
    elif mutation == "blank":
        evidence["image"]["blank_fraction"] = 0.99
    elif mutation == "stale":
        evidence["state"]["clients"][1]["snapshot_time"] = 7000
    elif mutation == "cross_pane":
        evidence["image"]["panes"][1]["hud_owner"] = 1
    elif mutation == "wrong_client":
        evidence["state"]["clients"][1]["userinfo"]["model"] = models[0]
    elif mutation == "silent":
        evidence["audio"]["events"] = []
    elif mutation == "leaked_socket":
        evidence["resources"]["sockets_final"] = 1
    elif mutation == "truncated":
        evidence["state"]["complete"] = False
    return evidence


def _load_evidence(manifest_path: Path, manifest: dict, findings: list[Finding]) -> dict | None:
    spec = manifest["artifact"]
    phase = manifest["phase"]
    source = (manifest_path.parent / spec["path"]).resolve()
    if not source.is_file():
        _finding(findings, "ARTIFACT_MISSING", "*", phase, str(source), None)
        return None
    if spec.get("sha256") and _sha256(source) != spec["sha256"]:
        _finding(findings, "ARTIFACT_HASH", "*", phase, spec["sha256"], _sha256(source))
        return None
    kind = spec.get("kind", "fixture" if "case" in spec else None)
    if kind == "fixture":
        corpus = _load_json(source)
        return _materialize(corpus["cases"][spec["case"]], phase)
    if kind != "evidence_bundle":
        _finding(findings, "ARTIFACT_KIND", "*", phase,
                 "fixture or evidence_bundle", kind)
        return None

    bundle = _load_json(source)
    if bundle.get("schema_version") != SCHEMA:
        _finding(findings, "EVIDENCE_VERSION", "*", phase, SCHEMA, bundle.get("schema_version"))
        return None
    evidence = {}
    for section in ("state", "image", "audio", "resources"):
        record = bundle.get("records", {}).get(section, {})
        record_path = (source.parent / record.get("path", "")).resolve()
        if not record_path.is_file():
            _finding(findings, "ARTIFACT_MISSING", "*", phase, f"{section}:{record_path}", None)
            continue
        actual_hash = _sha256(record_path)
        if not record.get("sha256") or actual_hash != record["sha256"]:
            _finding(findings, "ARTIFACT_HASH", "*", phase,
                     f"{section}:{record.get('sha256')}", f"{section}:{actual_hash}")
            continue
        evidence[section] = _load_json(record_path)
    for attachment in bundle.get("attachments", []):
        attachment_path = (source.parent / attachment["path"]).resolve()
        if not attachment_path.is_file():
            _finding(findings, "ARTIFACT_MISSING", "*", phase, str(attachment_path), None)
        elif _sha256(attachment_path) != attachment["sha256"]:
            _finding(findings, "ARTIFACT_HASH", "*", phase,
                     attachment["sha256"], _sha256(attachment_path))
    return evidence if len(evidence) == 4 and not findings else None


def evaluate(manifest_path: Path) -> list[Finding]:
    manifest = _load_json(manifest_path)
    findings: list[Finding] = []
    if manifest.get("schema_version") != SCHEMA:
        return [Finding("MANIFEST_VERSION", "*", "manifest", SCHEMA, manifest.get("schema_version"))]

    evidence = _load_evidence(manifest_path, manifest, findings)
    if evidence is None:
        return findings
    expected_players = manifest["expected"]["players"]
    phase = manifest["phase"]

    state = evidence["state"]
    if not state.get("complete", False):
        _finding(findings, "STATE_TRUNCATED", "*", phase, True, state.get("complete"))
    clients = {client["player"]: client for client in state.get("clients", [])}
    for player in range(1, expected_players + 1):
        actual = clients.get(player)
        if actual is None:
            _finding(findings, "STATE_CLIENT_MISSING", player, phase, "client record", None)
            continue
        for key, value in manifest["expected"]["state"].items():
            if key == "userinfo":
                for info_key, info_value in value.items():
                    got = actual.get("userinfo", {}).get(info_key)
                    if got != info_value[player - 1]:
                        _finding(findings, "STATE_USERINFO", player, phase,
                                 {info_key: info_value[player - 1]}, {info_key: got})
            elif actual.get(key) != value:
                _finding(findings, "STATE_" + key.upper(), player, phase, value, actual.get(key))
        age = state["server_time"] - actual["snapshot_time"]
        if age > manifest["expected"]["max_snapshot_age_ms"]:
            _finding(findings, "STATE_STALE_SNAPSHOT", player, phase,
                     f"<= {manifest['expected']['max_snapshot_age_ms']} ms", age)
        required_command = f"ack:{phase}:p{player}"
        if required_command not in actual.get("reliable_commands", []):
            _finding(findings, "STATE_RELIABLE_COMMAND", player, phase,
                     required_command, actual.get("reliable_commands", []))
    for key, code in (("slot", "STATE_DUPLICATE_SLOT"), ("qport", "STATE_DUPLICATE_QPORT"),
                      ("snapshot_id", "STATE_DUPLICATE_SNAPSHOT")):
        values = [client[key] for client in clients.values()]
        if len(values) != len(set(values)):
            _finding(findings, code, "*", phase, "unique per player", values)
    if not state.get("clean_disconnect"):
        _finding(findings, "STATE_DIRTY_DISCONNECT", "*", phase, True, state.get("clean_disconnect"))

    image = evidence["image"]
    if image["blank_fraction"] > manifest["expected"]["image"]["max_blank_fraction"]:
        _finding(findings, "IMAGE_BLANK", "*", phase,
                 manifest["expected"]["image"]["max_blank_fraction"], image["blank_fraction"])
    if image["fade_fraction"] > manifest["expected"]["image"]["max_fade_fraction"]:
        _finding(findings, "IMAGE_FADE", "*", phase,
                 manifest["expected"]["image"]["max_fade_fraction"], image["fade_fraction"])
    if image["menu"] != manifest["expected"]["image"]["menu"]:
        _finding(findings, "IMAGE_WRONG_MENU", "*", phase,
                 manifest["expected"]["image"]["menu"], image["menu"])
    panes = image.get("panes", [])
    signatures = [pane["signature"] for pane in panes]
    if len(signatures) != len(set(signatures)):
        _finding(findings, "IMAGE_DUPLICATE_PANE", "*", phase, "unique signatures", signatures)
    for pane in panes:
        player = pane["player"]
        if pane["hud_owner"] != player:
            _finding(findings, "IMAGE_HUD_OWNER", player, phase, player, pane["hud_owner"])
        if not pane["cursor_confined"]:
            _finding(findings, "IMAGE_CURSOR_CONFINEMENT", player, phase, True, False)
        if not pane["boundary_ok"]:
            _finding(findings, "IMAGE_VIEWPORT_BOUNDARY", player, phase, True, False)

    audio = evidence["audio"]
    for player in range(1, expected_players + 1):
        events = [event["event"] for event in audio.get("events", []) if event["player"] == player]
        expected = manifest["expected"]["audio_events"]
        if events != expected:
            _finding(findings, "AUDIO_EVENT_TRACE", player, phase, expected, events)

    resources = evidence["resources"]
    if resources["sockets_final"] != 0:
        _finding(findings, "RESOURCE_SOCKET_LEAK", "*", phase, 0, resources["sockets_final"])
    for key, allowance, code in (
        ("fds", 2, "RESOURCE_FD_LEAK"),
        ("threads", 1, "RESOURCE_THREAD_LEAK"),
    ):
        delta = resources[f"{key}_final"] - resources[f"{key}_baseline"]
        if delta > allowance:
            _finding(findings, code, "*", phase, f"<= +{allowance}", delta)
    rss_growth = resources["rss_samples_kb"][-1] - resources["rss_samples_kb"][0]
    if rss_growth > manifest["expected"]["resources"]["max_rss_growth_kb"]:
        _finding(findings, "RESOURCE_RSS_GROWTH", "*", phase,
                 manifest["expected"]["resources"]["max_rss_growth_kb"], rss_growth)
    frame_p95 = statistics.quantiles(resources["frame_times_ms"], n=20, method="inclusive")[18]
    if frame_p95 > manifest["expected"]["resources"]["max_frame_p95_ms"]:
        _finding(findings, "RESOURCE_FRAME_P95", "*", phase,
                 manifest["expected"]["resources"]["max_frame_p95_ms"], frame_p95)
    if resources["renderer_restarts"] != manifest["expected"]["resources"]["renderer_restarts"]:
        _finding(findings, "RESOURCE_RENDERER_RESTART", "*", phase,
                 manifest["expected"]["resources"]["renderer_restarts"], resources["renderer_restarts"])
    if resources["vm_restarts"] != manifest["expected"]["resources"]["vm_restarts"]:
        _finding(findings, "RESOURCE_VM_RESTART", "*", phase,
                 manifest["expected"]["resources"]["vm_restarts"], resources["vm_restarts"])
    if resources["crash_markers"]:
        _finding(findings, "RESOURCE_CRASH_MARKER", "*", phase, [], resources["crash_markers"])
    return findings


def main(argv=None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    findings = evaluate(args.manifest)
    result = {
        "oracle_schema_version": SCHEMA,
        "manifest": str(args.manifest),
        "passed": not findings,
        "findings": [asdict(item) for item in findings],
    }
    if args.json:
        print(json.dumps(result, sort_keys=True))
    elif findings:
        for item in findings:
            print(f"FAIL {item.code} player={item.player} phase={item.phase} "
                  f"expected={item.expected!r} actual={item.actual!r}")
    else:
        print(f"PASS manifest={args.manifest}")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
