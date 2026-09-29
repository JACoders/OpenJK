#!/usr/bin/env python3
"""Build the GP2-03 discovery matrix from frozen run logs."""

import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--logs", nargs=3, type=Path, required=True)
ap.add_argument("--output", type=Path, required=True)
ap.add_argument("--screenshot-oracle", type=Path, required=True)
a = ap.parse_args()

texts = {players: path.read_text(errors="replace") for players, path in zip((2, 3, 4), a.logs)}


def segment(text, name):
    begin, end = f"GP2-03:{name}-BEGIN", f"GP2-03:{name}-END"
    if begin not in text or end not in text:
        return None
    return text.split(begin, 1)[1].split(end, 1)[0]


rows = []
for players, text in texts.items():
    for name in ("TEAM-ASSIGN", "ENEMY-KILL", "RESPAWN", "SUICIDE",
                 "TEAM-SWITCH", "SPECTATE", "RESTART", "SECOND-MAP", "INTERMISSION"):
        data = segment(text, name)
        fail = (
            data is None
            or re.search(r"Split\w*Assert\w*: FAIL|SplitNetStagePair: FAIL", data)
            is not None
        )
        status = "DISCOVERED_FAIL" if fail else "COVERED_PASS"
        actual = ("missing markers" if data is None else
                  ("assertion failure" if fail else "all assertions passed"))
        if name == "RESPAWN" and not fail:
            enemy = segment(text, "ENEMY-KILL")
            enemy_dead = (
                enemy is not None
                and "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD"
                in enemy
                and re.search(
                    r"Split\w*Assert\w*: FAIL|SplitNetStagePair: FAIL", enemy
                )
                is None
            )
            if enemy_dead:
                actual = (
                    "the preceding enemy-kill cell proved P2 DEAD and this cell "
                    "then proved P2 ALIVE"
                )
            else:
                status = "BLOCKED_NOT_COVERED"
                actual = (
                    "ALIVE was observed, but the preceding enemy-kill cell did not "
                    "prove DEAD; no death-to-respawn transition was established"
                )
        elif name == "RESTART" and not fail:
            actual = (
                "all requested players were ALIVE after map_restart; client "
                "identity, team, model, and Force-profile preservation were not asserted"
            )
        elif name == "SECOND-MAP" and not fail:
            actual = (
                "all requested players were ALIVE after loading mp/ffa2 and "
                "command-driven team requests; identity/profile preservation was not asserted"
            )
        rows.append({"players": players, "cell": name.lower().replace("-", "_"),
                     "status": status, "actual": actual})
    for name in ("TEAMKILL-OFF", "TEAMKILL-ON"):
        if players == 2:
            rows.append({"players": players, "cell": name.lower().replace("-", "_"),
                         "status": "BLOCKED_UNSUPPORTED",
                         "actual": "two local players cannot provide a same-team attacker/victim pair"})
        else:
            data = segment(text, name)
            fail = (
                data is None
                or re.search(r"Split\w*Assert\w*: FAIL", data) is not None
            )
            rows.append({"players": players, "cell": name.lower().replace("-", "_"),
                         "status": "DISCOVERED_FAIL" if fail else "COVERED_PASS",
                         "actual": "missing/assertion failure" if fail else "all assertions passed"})

for players in (2, 3, 4):
    for cell, reason in (
        ("pane_profile_selection", "no routed per-pane profile flow in this mode probe"),
        ("team_score_exact", "client oracle exposes personal score, not team score"),
        ("team_hud_scoreboard_announcer_spawn_camera", "screenshots exist but no semantic pane oracle is wired"),
    ):
        rows.append({"players": players, "cell": cell, "status": "BLOCKED_NOT_COVERED", "actual": reason})
    image = a.output.parent / f"{players}p/gp2_team_assignment.png"
    proc = subprocess.run(
        [sys.executable, str(a.screenshot_oracle), str(image), str(players)],
        text=True, capture_output=True, check=False,
    )
    rows.append({
        "players": players,
        "cell": "player_configuration_image_uniqueness",
        "status": "COVERED_PASS" if proc.returncode == 0 else "DISCOVERED_FAIL",
        "actual": (
            "gameplay capture checks per-pane image uniqueness but does not by "
            "itself establish authoritative team identity; " + proc.stdout.strip()
        ),
    })

summary = {key: sum(r["status"] == key for r in rows)
           for key in ("COVERED_PASS", "DISCOVERED_FAIL", "BLOCKED_UNSUPPORTED", "BLOCKED_NOT_COVERED")}
report = {
    "schema_version": 1, "ticket": "GP2-03", "frozen_binary_sha256":
    "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
    "summary": summary,
    "artifacts": [{"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                  for path in a.logs],
    "cells": rows,
}
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
