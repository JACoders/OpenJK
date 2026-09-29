#!/usr/bin/env python3
"""Fail-closed validator for current GP5-03 four-player evidence."""

from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess
import sys


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/certification/4p"
report_path = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else RESULTS / "matrix.json"
expected = sys.argv[2] if len(sys.argv) > 2 else None
report = json.loads(report_path.read_text())

assert report["schema_version"] == 2
assert report["ticket"] == "GP5-03"
if expected is not None:
    assert report["frozen_binary_sha256"] == expected
    binary = ROOT / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    assert hashlib.sha256(binary.read_bytes()).hexdigest() == expected

assert Counter(cell["status"] for cell in report["cells"]) == Counter(report["summary"])
local_cells = [cell for cell in report["cells"] if cell["cell"] != "safe_public_attempt"]
assert local_cells
assert {cell["cell"] for cell in local_cells} == {
    "local_attack_force_and_simultaneous_input",
    "four_distinct_profile_userinfos",
    "controlled_server_four_slots_rejoin",
    "visible_ui_profile_selection",
    "varied_secondary_slot_churn",
    "independent_controlled_fifth_client",
    "per_pane_modal_ownership",
}
assert all(cell["status"] == "COVERED_PASS" for cell in local_cells)
assert report["local_certification_passed"] is True

public = next(cell for cell in report["cells"] if cell["cell"] == "safe_public_attempt")
assert public["status"] in {"COVERED_PASS", "BLOCKED_EXTERNAL", "DISCOVERED_FAIL"}
assert report["certification_passed"] is (
    report["local_certification_passed"] and public["status"] == "COVERED_PASS"
)
assert public["status"] == "COVERED_PASS"
assert report["certification_passed"] is True

for cell in report["cells"]:
    evidence = Path(cell["evidence"])
    assert evidence.is_file() and evidence.stat().st_size > 0, cell["cell"]

runner = ROOT / "tests/splitscreen/gameplay/run_e2e.sh"
for manifest_text in report["manifests"]:
    manifest = Path(manifest_text)
    assert manifest.is_file() and manifest.stat().st_size > 0, manifest
    status = next(
        (
            line.split("\t", 1)[1]
            for line in manifest.read_text().splitlines()
            if line.startswith("status\t")
        ),
        "",
    )
    if status == "passed":
        proc = subprocess.run([str(runner), "--validate-only", str(manifest)], check=False)
        assert proc.returncode == 0, manifest
    elif status == "skipped":
        assert manifest.name == "public-manifest.tsv"
    else:
        raise AssertionError(f"non-passing certification manifest: {manifest} ({status})")

print(
    "GP5-03 validation complete: certification_passed=true, "
    "all local and public cells covered"
)
