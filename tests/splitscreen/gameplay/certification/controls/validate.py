#!/usr/bin/env python3
"""Validate and seal the GP5-04 ordinary-client control artifacts."""

import argparse
import hashlib
import json
import shutil
from pathlib import Path


EXPECTED_HASH = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
MODES = (
    "FFA",
    "HOLOCRON",
    "JEDIMASTER",
    "DUEL",
    "POWERDUEL",
    "TEAMFFA",
    "SIEGE",
    "CTF",
    "CTY",
)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--run-root", type=Path, required=True)
    parser.add_argument("--results", type=Path, required=True)
    parser.add_argument("--candidate-hash", required=True)
    args = parser.parse_args()

    logs = args.run_root / "logs"
    local_log = logs / "local.log"
    candidate_log = logs / "candidate.log"
    remote_log = logs / "remote.log"
    server_log = logs / "server.log"
    texts = {p.name: p.read_text(errors="replace") for p in
             (local_log, candidate_log, remote_log, server_log)}
    checks = {}
    checks["frozen_hash"] = args.candidate_hash == EXPECTED_HASH
    checks["all_modes_reached"] = all(
        f"GP5ControlMode {mode}" in texts["local.log"] for mode in MODES
    )
    checks["split_disabled_every_mode"] = (
        texts["local.log"].count(
            "SplitUIAssert: PASS cvar=cl_splitScreen expected=0 actual=0"
        ) >= len(MODES) + 1
    )
    checks["candidate_joined_upstream"] = (
        "GP5ControlOnline candidate_active" in texts["candidate.log"]
        and "GP5_Candidate" in texts["server.log"]
    )
    checks["upstream_peer_joined"] = (
        "GP5ControlOnline upstream_active" in texts["remote.log"]
        and "GP5_Upstream" in texts["server.log"]
    )
    fatal_markers = ("SplitUIAssert: FAIL", "recursive error",
                     "segmentation fault", "assertion failed")
    checks["no_failure_markers"] = not any(
        marker.lower() in text.lower()
        for text in texts.values()
        for marker in fatal_markers
    )

    local_shots = args.run_root / "local/base/screenshots"
    candidate_shots = args.run_root / "candidate/base/screenshots"
    required = [
        local_shots / "gp5_control_main_menu.png",
        local_shots / "gp5_control_multiplayer.png",
        *[local_shots / f"gp5_control_{name}.png" for name in
          ("ffa", "holocron", "jedimaster", "duel", "powerduel",
           "teamffa", "siege", "ctf", "cty")],
        candidate_shots / "gp5_control_candidate_online.png",
    ]
    checks["screenshots_complete"] = all(
        path.is_file() and path.stat().st_size > 0 for path in required
    )

    args.results.mkdir(parents=True, exist_ok=True)
    evidence = args.results / "evidence"
    if evidence.exists():
        shutil.rmtree(evidence)
    evidence.mkdir()
    artifacts = []
    for path in (*logs.glob("*.log"), *required):
        target = evidence / path.name
        shutil.copy2(path, target)
        artifacts.append({
            "path": str(target.relative_to(args.results)),
            "sha256": digest(target),
            "bytes": target.stat().st_size,
        })
    report = {
        "format": "openjk-gp5-control-v1",
        "status": "passed" if all(checks.values()) else "failed",
        "candidate_sha256": args.candidate_hash,
        "checks": checks,
        "artifacts": sorted(artifacts, key=lambda item: item["path"]),
    }
    (args.results / "manifest.json").write_text(
        json.dumps(report, indent=2) + "\n"
    )
    print(json.dumps(report, indent=2))
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
