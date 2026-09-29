#!/usr/bin/env python3
import argparse, hashlib, json, re, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
FROZEN = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"


def fields(path):
    out = {}
    for line in path.read_text().splitlines():
        parts = line.split("\t", 2)
        if len(parts) >= 2:
            out.setdefault(parts[0], []).append(parts[1:])
    return out


def ordered(text, *needles):
    pos = -1
    for needle in needles:
        pos = text.find(needle, pos + 1)
        if pos < 0:
            return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", type=Path, required=True)
    ap.add_argument("--require-acceptance", action="store_true")
    args = ap.parse_args()
    args.results = args.results.resolve()
    findings, cells = [], []
    binary = ROOT / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    if hashlib.sha256(binary.read_bytes()).hexdigest() != FROZEN:
        findings.append({"id": "GP3-05-BUILD", "kind": "evidence-invalid",
                         "summary": "frozen client hash mismatch"})
    for players in (2, 3, 4):
        runs = sorted((args.results / f"audio-{players}p").glob("*/manifest.tsv"))
        cell = {"players": players, "status": "missing", "proven_log": [],
                "attempted_smoke": [], "unsupported": []}
        if not runs:
            findings.append({"id": f"GP3-05-{players}P-MISSING",
                             "kind": "evidence-invalid", "summary": "missing manifest"})
            cells.append(cell)
            continue
        manifest = runs[-1]
        cell["manifest"] = str(manifest.relative_to(args.results))
        replay = subprocess.run([str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"),
                                 "--validate-only", str(manifest)],
                                text=True, capture_output=True)
        data = fields(manifest)
        host_log = Path(data["process.client.log"][-1][0])
        remote_log = Path(data["process.remote.log"][-1][0])
        host = host_log.read_text(errors="replace")
        remote = remote_log.read_text(errors="replace")
        fail_patterns = r"Assert\w*: FAIL|Sys_Error|recursive error|segmentation|assertion"
        failure_count = len(re.findall(fail_patterns, host, re.I)) + len(
            re.findall(fail_patterns, remote, re.I))
        init_count = host.count("Sound memory manager started")
        all_inputs = all(f"SplitInputAssertCmd: PASS player={p}" in host
                         for p in range(1, players + 1))
        death_respawn = ordered(
            host, f"player={players} expected=DEAD actual=DEAD",
            f"player={players} expected=ALIVE actual=ALIVE")
        restart_log = ordered(host, "GP305:SND-RESTART",
                              "Closing SDL audio device",
                              "SDL audio device shut down",
                              "SDL audio initialized",
                              "GP305:SND-RESTART-COMPLETE")
        map_alive = ordered(
            host, "GP305:MAP-RESTART-COMPLETE",
            *[f"player={p} expected=ALIVE actual=ALIVE"
              for p in range(1, players + 1)])
        remote_join = "GP305_Remote" in host and "connected" in remote.lower()
        shutdown = (data.get("process.client.exit", [["?"]])[-1][0] == "0" and
                    data.get("process.remote.exit", [["?"]])[-1][0] == "0")
        for name, value in (
            ("sound_memory_init_log", init_count >= 1),
            ("ordered_all_pane_input_commands", all_inputs),
            ("ordered_death_respawn", death_respawn),
            ("sound_device_restart_log", restart_log),
            ("ordered_post_map_restart_alive", map_alive),
            ("independent_network_client_join", remote_join),
            ("clean_process_shutdown", shutdown),
        ):
            if value:
                cell["proven_log"].append(name)
            else:
                findings.append({"id": f"GP3-05-{players}P-{name.upper()}",
                                 "kind": "log-evidence-gap",
                                 "summary": f"missing {name}"})
        cell["attempted_smoke"] = [
            "audio_enabled_cvar", "weapon_actions_all_panes", "network_chat",
            "volume_configuration", "listen_network_transition",
            "screenshot_presence"
        ]
        cell["unsupported"] = [
            "audible_output", "listener_mix_policy", "spatial_position",
            "pane_audio_attribution", "event_drop_or_duplicate",
            "ui_force_voice_ambient_announcer_trace", "channel_voice_counts",
            "physical_device_loss_reopen", "device_thread_channel_leak",
            "30_minute_resource_baseline", "intermission_audio"
        ]
        pngs = sum(1 for row in data.get("artifact", []) if row[0].endswith(".png"))
        cell.update({"phase0_valid": replay.returncode == 0,
                     "failure_markers": failure_count, "sound_init_log_count": init_count,
                     "screenshots": pngs,
                     "status": "partial" if replay.returncode == 0 and not failure_count else "failed"})
        if replay.returncode:
            findings.append({"id": f"GP3-05-{players}P-MANIFEST",
                             "kind": "evidence-invalid", "summary": replay.stderr.strip()})
        if failure_count:
            findings.append({"id": f"GP3-05-{players}P-RUNTIME",
                             "kind": "runtime-failure",
                             "summary": f"{failure_count} crash/error/assert markers"})
        cells.append(cell)
    report = {"schema_version": 1, "ticket": "GP3-05",
              "status": "failed" if findings else "partial",
              "frozen_sha256": FROZEN, "cells": cells, "findings": findings,
              "policy": "Log evidence proves engine actions only; no audible, spatial, mixer, or attribution claim without captured audio telemetry."}
    out = args.results / "current"
    out.mkdir(parents=True, exist_ok=True)
    (out / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# GP3-05 audio-runtime findings", "", f"Status: **{report['status'].upper()}**", "",
             "Log evidence is not audible/spatial/mixer proof.", "",
             "| Players | Result | Proven log evidence | Attempted/smoke | Unsupported |",
             "|---:|---|---|---|---|"]
    for c in cells:
        lines.append(f"| {c['players']} | {c['status']} | {', '.join(c['proven_log']) or 'none'} | "
                     f"{', '.join(c['attempted_smoke']) or 'none'} | "
                     f"{', '.join(c['unsupported']) or 'none'} |")
    lines += ["", "## Findings", ""]
    lines += [f"- `{f['id']}` ({f['kind']}): {f['summary']}" for f in findings]
    (out / "findings.md").write_text("\n".join(lines) + "\n")
    print(json.dumps({"status": report["status"], "cells": len(cells),
                      "findings": len(findings)}))
    incomplete = bool(findings) or any(c["unsupported"] for c in cells)
    return 1 if args.require_acceptance and incomplete else 0


if __name__ == "__main__":
    raise SystemExit(main())
