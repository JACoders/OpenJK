#!/usr/bin/env python3
"""Validate the frozen-binary GP2-05 Siege discovery report."""

from __future__ import annotations

import json
import subprocess
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAMEPLAY = HERE.parents[1]
RESULTS = GAMEPLAY / "results" / "modes" / "siege"
RUNNER = GAMEPLAY / "run_e2e.sh"
EXPECTED_BINARY = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
EXPECTED_SUMMARY = {
    "BLOCKED_NOT_COVERED": 24,
    "COVERED_PASS": 18,
    "DISCOVERED_FAIL": 0,
}


def main() -> int:
    matrix = json.loads((RESULTS / "matrix.json").read_text())
    assert matrix["schema_version"] == 1
    assert matrix["ticket"] == "GP2-05"
    assert matrix["frozen_binary_sha256"] == EXPECTED_BINARY
    assert matrix["summary"] == EXPECTED_SUMMARY

    cells = matrix["cells"]
    assert Counter(cell["status"] for cell in cells) == Counter(EXPECTED_SUMMARY)
    assert {cell["players"] for cell in cells} == {2, 3, 4}
    for players in (2, 3, 4):
        player_cells = [cell for cell in cells if cell["players"] == players]
        assert len(player_cells) == 14
        by_name = {cell["cell"]: cell for cell in player_cells}
        assert by_name["class_team"]["status"] == "COVERED_PASS"
        assert by_name["use"]["status"] == "COVERED_PASS"
        assert by_name["death_wave"]["status"] == "COVERED_PASS"
        assert by_name["mission_objectives_menu_image"]["status"] == "BLOCKED_NOT_COVERED"
        assert "cannot establish gameplay pane uniqueness" in by_name[
            "mission_objectives_menu_image"
        ]["actual"]
        assert by_name["class_change"]["status"] == "COVERED_PASS"
        assert "P2 was DEAD" in by_name["class_change"]["actual"]
        assert "Rebel Sniper's Disruptor" in by_name["class_change"]["actual"]
        assert "identity, class, inventory" in by_name["restart"]["actual"]
        assert "identity, class, inventory" in by_name["next_map"]["actual"]
        assert sum(c["status"] == "BLOCKED_NOT_COVERED" for c in player_cells) == 8

    manifest = RESULTS / "artifact-integrity-manifest.tsv"
    subprocess.run([str(RUNNER), "--validate-only", str(manifest)], check=True)
    manifest_text = manifest.read_text()
    assert f"\t{EXPECTED_BINARY}\n" in manifest_text
    assert "case\tGP2-05-artifact-integrity-only\n" in manifest_text
    assert "discovery_result\tacceptance_not_met\n" in manifest_text

    print(
        "GP2-05 report valid: "
        + ", ".join(f"{key}={value}" for key, value in sorted(EXPECTED_SUMMARY.items()))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
