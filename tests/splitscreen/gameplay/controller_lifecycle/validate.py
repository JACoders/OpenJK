#!/usr/bin/env python3
"""Validate the archived GP1-03 discovery report and immutable artifacts."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
report_path = ROOT / "tests/splitscreen/gameplay/results/controller_lifecycle/matrix.json"
manifest_path = ROOT / "tests/splitscreen/gameplay/results/controller_lifecycle/certification-manifest.tsv"
report = json.loads(report_path.read_text())
assert report["schema_version"] == 1
assert report["ticket"] == "GP1-03"
assert report["discovery_only"] is True
assert report["summary"] == {
    "covered_pass": 2,
    "discovered_fail": 4,
    "blocked_unsupported": 13,
}
for artifact in report["artifacts"]:
    path = Path(artifact["path"])
    assert path.is_file(), path
    assert hashlib.sha256(path.read_bytes()).hexdigest() == artifact["sha256"], path
statuses = {row["cell"]: row["status"] for row in report["cells"]}
assert statuses["pause_during_held_input"] == "COVERED_PASS"
assert statuses["process_relaunch_stable_slots"] == "COVERED_PASS"
assert statuses["simultaneous_controllers_held_release"] == "DISCOVERED_FAIL"
assert statuses["in_restart_recovery"] == "DISCOVERED_FAIL"
assert statuses["physical_disconnect_gameplay"] == "BLOCKED_UNSUPPORTED"
manifest = manifest_path.read_text()
assert "format\topenjk-e2e-v1" in manifest
assert "status\tpassed" in manifest
assert "discovery_result\tacceptance_not_met" in manifest
print("GP1-03 discovery report valid: 2 covered, 4 failing, 13 unsupported")
