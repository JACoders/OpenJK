#!/usr/bin/env python3
"""Validate GP3-03 ordered classifications and normal per-run manifests."""

from __future__ import annotations

import hashlib
import json
import subprocess
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/network/recovery"
RUNNER = ROOT / "tests/splitscreen/gameplay/run_e2e.sh"
EXPECTED = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
RUNTIME_CELLS = {
    "baseline",
    "secondary_disconnect",
    "secondary_rejoin",
    "restart",
    "map_transition",
    "p1_party_disconnect",
    "server_loss",
}


def main() -> int:
    report = json.loads((RESULTS / "matrix.json").read_text())
    assert report["schema_version"] == 1 and report["ticket"] == "GP3-03"
    assert report["frozen_binary_sha256"] == EXPECTED
    assert Counter(cell["status"] for cell in report["cells"]) == Counter(report["summary"])
    assert len(report["cells"]) == 48
    assert report["acceptance_met"] is all(
        cell["status"] == "COVERED_PASS" for cell in report["cells"]
    )
    runtime_rows = [cell for cell in report["cells"] if cell["cell"] in RUNTIME_CELLS]
    assert len(runtime_rows) == 21
    assert report["runtime_acceptance_met"] is all(
        cell["status"] == "COVERED_PASS" for cell in runtime_rows
    )
    assert {item["players"] for item in report["manifests"]} == {2, 3, 4}
    for item in report["manifests"]:
        manifest = Path(item["path"])
        assert hashlib.sha256(manifest.read_bytes()).hexdigest() == item["sha256"]
        assert item["status"] in {"passed", "failed", "timeout"}
        if item["status"] == "passed":
            subprocess.run([str(RUNNER), "--validate-only", str(manifest)], check=True)
        else:
            # Failed runs remain evidence, but must never be replayed as passes.
            proc = subprocess.run(
                [str(RUNNER), "--validate-only", str(manifest)],
                check=False,
                capture_output=True,
            )
            assert proc.returncode != 0
        cells = {
            cell["cell"]: cell for cell in report["cells"] if cell["players"] == item["players"]
        }
        if cells["secondary_disconnect"]["status"] != "COVERED_PASS":
            assert cells["secondary_rejoin"]["status"] != "COVERED_PASS"
        if cells["server_loss"]["status"] == "COVERED_PASS":
            assert "server-loss observation" in cells["server_loss"]["actual"]
        for name in ("restart", "map_transition", "p1_party_disconnect"):
            if cells[name]["status"] == "COVERED_PASS":
                assert "clientnum" in cells[name]["actual"]
        if item["players"] == 4:
            same_ip = cells["same_ip_duplicate_slot"]
            assert same_ip["status"] in {"BLOCKED_NOT_COVERED", "DISCOVERED_FAIL"}
            if same_ip["status"] == "DISCOVERED_FAIL":
                assert "same-IP rejection" in same_ip["actual"]
    print(
        "GP3-03 report valid"
        f" runtime_acceptance_met={str(report['runtime_acceptance_met']).lower()}: "
        + ", ".join(f"{key}={value}" for key, value in sorted(report["summary"].items()))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
