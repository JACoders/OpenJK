#!/usr/bin/env python3
"""Validate GP2-04 classifications and replay Phase0 artifact validation."""

from __future__ import annotations

import json
import subprocess
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAMEPLAY = HERE.parents[1]
RESULTS = GAMEPLAY / "results" / "modes" / "ctf_cty"
RUNNER = GAMEPLAY / "run_e2e.sh"


def main() -> int:
    matrix = json.loads((RESULTS / "matrix.json").read_text())
    assert matrix["schema_version"] == 2
    assert matrix["ticket"] == "GP2-04"
    assert matrix["frozen_binary_sha256"] == "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
    assert matrix["acceptance_met"] is False
    statuses = {case["status"] for case in matrix["cases"]}
    assert "BLOCKED_UNSUPPORTED" in statuses
    actual = Counter(case["status"] for case in matrix["cases"])
    assert dict(sorted(actual.items())) == matrix["summary"]
    for manifest_text in matrix["manifests"]:
        manifest = Path(manifest_text)
        assert manifest.is_file(), manifest
        subprocess.run([str(RUNNER), "--validate-only", str(manifest)], check=True)
        text = manifest.read_text()
        assert (
            "hash\t"
            + str(GAMEPLAY.parents[2] / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64")
            + "\t737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
        ) in text
    for case in matrix["cases"]:
        if case["status"] != "BLOCKED_UNSUPPORTED":
            assert Path(case["log"]).is_file(), case["id"]
    print(
        "GP2-04 report valid: "
        + ", ".join(f"{key}={value}" for key, value in sorted(matrix["summary"].items()))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
