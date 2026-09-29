#!/usr/bin/env python3
"""Validate every latest GP2-02 Phase 0 manifest and matrix artifact hash."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--root", type=Path, required=True)
parser.add_argument("--results", type=Path, required=True)
args = parser.parse_args()
runner = args.root / "tests/splitscreen/gameplay/run_e2e.sh"
matrix_path = args.results / "matrix.json"
matrix = json.loads(matrix_path.read_text())
failures = []
statuses = ("COVERED_PASS", "COVERED_SMOKE", "DISCOVERED_FAIL", "BLOCKED",
            "BLOCKED_UNSUPPORTED")

if matrix.get("schema_version") != 1 or matrix.get("ticket") != "GP2-02":
    failures.append("matrix identity/schema mismatch")
if matrix.get("discovery_only") is not True:
    failures.append("matrix must remain discovery_only")

cells = matrix.get("cells", [])
recomputed = {status: sum(cell.get("status") == status for cell in cells)
              for status in statuses}
if matrix.get("summary") != recomputed:
    failures.append(
        f"matrix summary mismatch: recorded={matrix.get('summary')} recomputed={recomputed}"
    )
unknown = sorted({cell.get("status") for cell in cells} - set(statuses))
if unknown:
    failures.append(f"unknown matrix statuses: {unknown}")

keys = [(cell.get("mode"), str(cell.get("players")), cell.get("cell")) for cell in cells]
if len(keys) != len(set(keys)):
    failures.append("duplicate matrix cells")

# COVERED_PASS is deliberately restricted to narrow, cell-specific claims.
# A generic PASS assertion or marker must never promote a round, transition,
# queue-order, or role claim.
for cell in cells:
    if cell.get("status") != "COVERED_PASS":
        continue
    if cell.get("cell") == "observed_population":
        actual = cell.get("actual", {})
        players = int(cell["players"])
        if not (
            len(actual.get("states", {})) == players
            and actual.get("alive") == 2
            and actual.get("spectators") == players - 2
        ):
            failures.append(f"population prerequisites not met: {cell}")
    elif cell.get("cell") == "scripted_attack_score_evidence":
        actual = cell.get("actual", {})
        if not (
            actual.get("stage_pair_pass") is True
            and actual.get("score_passes")
            and actual.get("score_assertion_failed") is False
            and "not a round-win or death proof" in cell.get("claim", "")
        ):
            failures.append(f"score-evidence prerequisites not met: {cell}")
    else:
        failures.append(f"forbidden broad COVERED_PASS cell: {cell}")

for cell in cells:
    if cell.get("cell") == "spectate_rejoin_attempt":
        if cell.get("status") == "COVERED_PASS":
            failures.append("spectate/rejoin cannot pass without a full asserted transition")
        if cell.get("actual", {}).get("transition_proven") is not False:
            failures.append(f"unsubstantiated spectate transition: {cell}")

required_unsupported = {
    "authoritative_queue_order",
    "duel_role_numeric_assertion",
    "both_directions_multi_round",
    "ready_and_next_match",
    "camera_model_hud_inheritance",
}
actual_unsupported = {
    cell.get("cell") for cell in cells
    if cell.get("status") == "BLOCKED_UNSUPPORTED"
}
if actual_unsupported != required_unsupported:
    failures.append(
        f"unsupported-cell mismatch: expected={sorted(required_unsupported)} "
        f"actual={sorted(actual_unsupported)}"
    )

for mode in ("duel", "powerduel"):
    for players in (2, 3, 4):
        manifests = sorted((args.results / f"{mode}-{players}p").glob("*/manifest.tsv"))
        if not manifests:
            failures.append(f"missing {mode}-{players}p manifest")
            continue
        result = subprocess.run([runner, "--validate-only", manifests[-1]], check=False)
        if result.returncode:
            failures.append(f"Phase0 replay failed: {manifests[-1]}")

for artifact in matrix.get("artifacts", []):
    path = Path(artifact["path"])
    if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != artifact["sha256"]:
        failures.append(f"matrix artifact mismatch: {path}")

if failures:
    print("\n".join(failures))
    raise SystemExit(1)
print("GP2-02 validator: PASS (6 Phase0 manifests and matrix artifact hashes)")
