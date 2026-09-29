#!/usr/bin/env python3
"""Validate the shape and honesty of the GP1-04 discovery matrix."""

from __future__ import annotations

import json
from pathlib import Path

RESULTS = Path(__file__).resolve().parents[1] / "results" / "modal_ownership"


def main() -> int:
    matrix = json.loads((RESULTS / "matrix.json").read_text())
    cases = matrix["cases"]
    statuses = {case["status"] for case in cases}
    assert matrix["schema_version"] == 1
    assert matrix["ticket"] == "GP1-04"
    assert {"COVERED_PASS", "DISCOVERED_FAIL", "BLOCKED_UNSUPPORTED"} <= statuses
    assert matrix["acceptance_met"] is False
    assert len(matrix["manifests"]) == 3
    for manifest in matrix["manifests"]:
        assert Path(manifest).is_file(), manifest
    for case in cases:
        if case["status"] != "BLOCKED_UNSUPPORTED":
            assert Path(case["evidence"]).is_file(), case["id"]
    print(
        "GP1-04 discovery report valid: "
        f"{matrix['summary'].get('COVERED_PASS', 0)} covered, "
        f"{matrix['summary'].get('DISCOVERED_FAIL', 0)} failing, "
        f"{matrix['summary'].get('BLOCKED_UNSUPPORTED', 0)} unsupported"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
