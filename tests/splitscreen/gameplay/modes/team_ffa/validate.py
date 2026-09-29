#!/usr/bin/env python3
"""Validate GP2-03 classifications and its artifact-integrity manifest."""

import hashlib, json, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
RESULTS = ROOT / "tests/splitscreen/gameplay/results/modes/team_ffa"
report = json.loads((RESULTS / "matrix.json").read_text())
assert report["schema_version"] == 1 and report["ticket"] == "GP2-03"
assert report["frozen_binary_sha256"] == "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
assert report["summary"] == {
    "COVERED_PASS": 34,
    "DISCOVERED_FAIL": 0,
    "BLOCKED_UNSUPPORTED": 2,
    "BLOCKED_NOT_COVERED": 9,
}
for players in (2, 3, 4):
    cells = {cell["cell"]: cell for cell in report["cells"] if cell["players"] == players}
    for cell in (
        "team_assign",
        "enemy_kill",
        "respawn",
        "suicide",
        "team_switch",
        "spectate",
        "restart",
        "second_map",
        "intermission",
        "player_configuration_image_uniqueness",
    ):
        assert cells[cell]["status"] == "COVERED_PASS", (players, cell, cells[cell])
    assert "proved P2 DEAD" in cells["respawn"]["actual"]
    assert "image uniqueness" in cells["player_configuration_image_uniqueness"]["actual"]
    for cell in (
        "pane_profile_selection",
        "team_score_exact",
        "team_hud_scoreboard_announcer_spawn_camera",
    ):
        assert cells[cell]["status"] == "BLOCKED_NOT_COVERED"
    if players == 2:
        assert cells["teamkill_off"]["status"] == "BLOCKED_UNSUPPORTED"
        assert cells["teamkill_on"]["status"] == "BLOCKED_UNSUPPORTED"
    else:
        assert cells["teamkill_off"]["status"] == "COVERED_PASS"
        assert cells["teamkill_on"]["status"] == "COVERED_PASS"
for artifact in report["artifacts"]:
    path = Path(artifact["path"])
    assert path.is_file()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == artifact["sha256"]
manifest_path = RESULTS / "artifact-integrity-manifest.tsv"
manifest = manifest_path.read_text()
assert "format\topenjk-e2e-v1" in manifest
assert "status\tpassed" in manifest
assert "case\tGP2-03-artifact-integrity-only" in manifest
assert "discovery_result\tacceptance_not_met" in manifest
subprocess.run(
    [str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"), "--validate-only", str(manifest_path)],
    check=True,
)
print("GP2-03 valid: 34 covered, 0 failing, 2 unsupported, 9 not covered")
