#!/usr/bin/env python3
"""Validate GP3-02 evidence integrity and classification honesty."""

import argparse
import hashlib
import json
import subprocess
from collections import Counter
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--root", type=Path, required=True)
ap.add_argument("--results", type=Path, required=True)
a = ap.parse_args()

matrix = json.loads((a.results / "matrix.json").read_text())
errors = []
if matrix.get("ticket") != "GP3-02" or matrix.get("schema_version") != 1:
    errors.append("matrix identity/schema mismatch")
if matrix.get("discovery_only") is not True or matrix.get("acceptance_met") is not False:
    errors.append("discovery must not certify product acceptance")
if matrix.get("frozen_binary_sha256") != \
        "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13":
    errors.append("frozen binary hash mismatch")

recomputed = dict(sorted(Counter(cell.get("status") for cell in matrix["cells"]).items()))
if recomputed != matrix.get("summary"):
    errors.append(f"summary mismatch: recorded={matrix.get('summary')} actual={recomputed}")

for manifest in matrix.get("manifests", []):
    proc = subprocess.run(
        [str(a.root / "tests/splitscreen/gameplay/run_e2e.sh"),
         "--validate-only", manifest], check=False
    )
    if proc.returncode:
        errors.append(f"Phase 0 validation failed: {manifest}")
if len(matrix.get("manifests", [])) != 6:
    errors.append(f"expected 6 manifests, got {len(matrix.get('manifests', []))}")

for artifact in matrix.get("artifacts", []):
    path = Path(artifact["path"])
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != artifact["sha256"]:
        errors.append(f"artifact hash mismatch: {path}")

allowed_passes = {
    "server_pure_mode_observation",
    "checksum_emission_observation",
    "initial_slot_lifecycle",
    "restart_slot_lifecycle",
    "stock_map_transition_slot_lifecycle",
}
for cell in matrix["cells"]:
    if cell["status"] == "COVERED_PASS" and cell["cell"] not in allowed_passes:
        errors.append(f"forbidden broad pass: {cell['case']}:{cell['cell']}")
    if cell["cell"] == "checksum_emission_observation":
        if cell["actual"].get("before_first_usermove_proven") is not False:
            errors.append("checksum evidence must not claim first-usermove ordering")
    if cell["cell"] in {
        "initial_slot_lifecycle", "restart_slot_lifecycle",
        "stock_map_transition_slot_lifecycle",
    } and cell["status"] == "COVERED_PASS":
        if cell["actual"].get("missing") or cell["actual"].get("assertion_failure"):
            errors.append(f"slot/lifecycle prerequisites failed: {cell['case']}:{cell['cell']}")

required_unsupported = {
    "checksum_before_first_usermove", "qvm_module_compatibility",
    "missing_pak_download_success", "download_cancel_retry",
    "corrupt_hash_recovery", "unavailable_download_recovery",
    "insufficient_space_recovery", "third_party_mod_matrix",
    "external_product_protocol_matrix", "vm_reload_state_ownership",
    "userinfo_identity_ownership", "device_routing_after_transition",
    "ui_pane_ownership", "stale_snapshot_duplicate_pane",
}
actual_unsupported = {
    cell["cell"] for cell in matrix["cells"]
    if cell["status"] == "BLOCKED_UNSUPPORTED"
}
if actual_unsupported != required_unsupported:
    errors.append("unsupported coverage set mismatch")

if errors:
    print("\n".join(errors))
    raise SystemExit(1)
print(f"GP3-02 evidence valid: {matrix['summary']}")
