#!/usr/bin/env python3
"""Classify GP3-04 evidence without promoting unsupported claims."""

import hashlib
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/network/wan_churn"
FROZEN = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"


def manifest_path(players: int) -> Path:
    return Path((RESULTS / f"{players}p-manifest-path.txt").read_text().strip())


def manifest_value(path: Path, key: str) -> str:
    for line in path.read_text().splitlines():
        parts = line.split("\t", 1)
        if len(parts) == 2 and parts[0] == key:
            return parts[1]
    raise KeyError(key)


rows = []
artifacts = []
findings = []
for players in (2, 3, 4):
    manifest = manifest_path(players)
    log = Path(manifest_value(manifest, "process.client.log"))
    text = log.read_text(errors="replace")
    artifacts.extend((manifest, log))

    initial = text.split("GP3-04:INITIAL-BEGIN", 1)[-1].split("GP3-04:INITIAL-END", 1)[0]
    initial_fail = "GP3-04:INITIAL-BEGIN" not in text or "GP3-04:INITIAL-END" not in text
    initial_fail = initial_fail or "Assert: FAIL" in initial
    rows.append({
        "players": players,
        "cell": "initial_unique_party",
        "status": "DISCOVERED_FAIL" if initial_fail else "COVERED_PASS",
        "actual": "missing marker or assertion failure" if initial_fail
        else "all local players ALIVE with exact distinct clientnum assertions",
        "manifest": str(manifest),
        "log": str(log),
    })

    seen = 0
    for match in re.finditer(r"GP3-04:CYCLE-(\d{3})-P(\d)-BEGIN(.*?)"
                             r"GP3-04:CYCLE-\1-P\2-END", text, re.S):
        seen += 1
        cycle, target, segment = int(match.group(1)), int(match.group(2)), match.group(3)
        failed = "Assert: FAIL" in segment or "SplitNet P" not in segment
        rows.append({
            "players": players,
            "cell": f"secondary_leave_rejoin_{cycle:03d}_p{target}",
            "status": "DISCOVERED_FAIL" if failed else "COVERED_PASS",
            "actual": "assertion failure or missing transport evidence" if failed else
            "ordered ALIVE -> disconnect/partial survivor health -> rejoin/active/ALIVE "
            "with exact target clientnum",
            "manifest": str(manifest),
            "log": str(log),
        })
    if seen != 12:
        findings.append({
            "id": f"GP3-04-{players}P-CYCLE-COVERAGE",
            "players": players,
            "expected": 12,
            "actual": seen,
            "route": "GP4-03",
        })

    rows += [
        {
            "players": players,
            "cell": "input_does_not_reroute_during_outage",
            "status": "BLOCKED_NOT_COVERED",
            "actual": "journey proves survivor lifecycle only; no simultaneous routed-input oracle",
        },
        {
            "players": players,
            "cell": "fresh_snapshot_after_rejoin",
            "status": "BLOCKED_NOT_COVERED",
            "actual": "clientnum and lifecycle are asserted; snapshot identity/age is not exported",
        },
        {
            "players": players,
            "cell": "socket_fd_growth",
            "status": "BLOCKED_NOT_COVERED",
            "actual": "Phase 0 harness records processes, not per-cycle socket/FD counts",
        },
    ]

for impairment in ("latency", "jitter", "loss", "duplication", "reordering", "bandwidth"):
    for scope in ("all_clients", "one_secondary"):
        rows.append({
            "players": "2/3/4",
            "cell": f"{impairment}_{scope}",
            "status": "BLOCKED_UNSUPPORTED",
            "actual": "no unprivileged deterministic per-socket impairment backend is installed",
        })
for phase in ("handshake", "pure_validation", "active_combat", "intermission",
              "download", "map_change"):
    rows.append({
        "players": "2/3/4",
        "cell": f"outage_during_{phase}",
        "status": "BLOCKED_UNSUPPORTED",
        "actual": "requires the missing deterministic impairment backend",
    })
rows += [
    {
        "players": "2/3/4",
        "cell": "minimum_100_cycles_varying_p1_p4",
        "status": "BLOCKED_NOT_COVERED",
        "actual": "36 secondary cycles executed (12 per party size); P1 reconnect and "
        "the required 100-cycle soak remain for Phase 6",
    },
    {
        "players": "2/3/4",
        "cell": "remote_fifth_during_churn",
        "status": "BLOCKED_NOT_COVERED",
        "actual": "controlled fifth-client orchestration is not part of this bounded discovery run",
    },
    {
        "players": "2/3/4",
        "cell": "server_restart_during_churn",
        "status": "BLOCKED_NOT_COVERED",
        "actual": "Phase 0 support-process teardown does not restart a server mid-run",
    },
    {
        "players": "2/3/4",
        "cell": "recovery_time_snapshot_gap_budgets",
        "status": "BLOCKED_NOT_COVERED",
        "actual": "no machine-readable timing/snapshot-gap oracle; values are not inferred from waits",
    },
]

for row in rows:
    if row["status"] == "DISCOVERED_FAIL":
        findings.append({
            "id": f"GP3-04-{len(findings) + 1:03d}",
            "players": row["players"],
            "cell": row["cell"],
            "actual": row["actual"],
            "route": "GP4-03",
        })

summary = {status: sum(row["status"] == status for row in rows) for status in
           ("COVERED_PASS", "DISCOVERED_FAIL", "BLOCKED_UNSUPPORTED",
            "BLOCKED_NOT_COVERED")}
unique_artifacts = sorted(set(artifacts))
report = {
    "schema_version": 1,
    "ticket": "GP3-04",
    "frozen_binary_sha256": FROZEN,
    "discovery_result": "acceptance_not_met",
    "summary": summary,
    "artifacts": [{"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                  for path in unique_artifacts],
    "cells": rows,
}
RESULTS.mkdir(parents=True, exist_ok=True)
(RESULTS / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
(RESULTS / "findings.json").write_text(json.dumps(findings, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
