#!/usr/bin/env python3
"""Validate the honesty and integrity of recorded GP3-04 discovery evidence."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


ap = argparse.ArgumentParser()
ap.add_argument("--require-acceptance", action="store_true")
args = ap.parse_args()

ROOT = Path(__file__).resolve().parents[5]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/network/wan_churn"
report = json.loads((RESULTS / "matrix.json").read_text())
assert report["schema_version"] == 1 and report["ticket"] == "GP3-04"
assert report["frozen_binary_sha256"] == \
    "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
assert sum(report["summary"].values()) == len(report["cells"])
assert report["discovery_result"] == "acceptance_not_met"

for artifact in report["artifacts"]:
    path = Path(artifact["path"])
    assert path.is_file()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == artifact["sha256"]
for players in (2, 3, 4):
    manifest = Path((RESULTS / f"{players}p-manifest-path.txt").read_text().strip())
    subprocess.run(
        [str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"),
         "--validate-only", str(manifest)],
        check=True,
    )
    cells = [row for row in report["cells"] if row["players"] == players]
    assert len([row for row in cells
                if row["cell"].startswith("secondary_leave_rejoin_")]) == 12

minimum = next(row for row in report["cells"]
               if row["cell"] == "minimum_100_cycles_varying_p1_p4")
assert minimum["status"] == "BLOCKED_NOT_COVERED"
for row in report["cells"]:
    if row["cell"].startswith(("latency_", "jitter_", "loss_", "duplication_",
                               "reordering_", "bandwidth_", "outage_during_")):
        assert row["status"] == "BLOCKED_UNSUPPORTED"

print("GP3-04 evidence valid:", json.dumps(report["summary"], sort_keys=True))
if args.require_acceptance and any(report["summary"][key] for key in
                                   ("DISCOVERED_FAIL", "BLOCKED_UNSUPPORTED",
                                    "BLOCKED_NOT_COVERED")):
    raise SystemExit(1)
