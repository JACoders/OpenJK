#!/usr/bin/env python3
import argparse, hashlib, json, subprocess, sys
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--logs", nargs=3, type=Path, required=True)
ap.add_argument("--output", type=Path, required=True)
ap.add_argument("--screenshot-oracle", type=Path, required=True)
a = ap.parse_args()
rows = []
for players, path in zip((2, 3, 4), a.logs):
    text = path.read_text(errors="replace")
    for name in ("CLASS-TEAM", "USE", "DEATH-WAVE", "CLASS-CHANGE", "RESTART", "NEXT-MAP"):
        begin, end = f"GP2-05:{name}-BEGIN", f"GP2-05:{name}-END"
        data = text.split(begin, 1)[1].split(end, 1)[0] if begin in text and end in text else None
        fail = (
            data is None
            or "Assert: FAIL" in data
            or "SplitInputAssertCmd: FAIL" in data
            or "SplitNetStagePair: FAIL" in data
        )
        actual = "missing/assertion failure" if fail else "all assertions passed"
        if name == "USE" and fail and data and "SplitInputAssertCmd: FAIL" in data:
            actual = "routed use command assertion failed"
        elif name == "CLASS-CHANGE" and not fail:
            actual = (
                "P2 was DEAD after the command-driven class request, remained "
                "BLUE, then returned ALIVE with the Rebel Sniper's Disruptor "
                "selected"
            )
        elif name == "RESTART" and not fail:
            actual = (
                "all requested players were ALIVE after map_restart; identity, "
                "class, inventory, and pane ownership were not asserted"
            )
        elif name == "NEXT-MAP" and not fail:
            actual = (
                "all requested players were ALIVE after loading mp/siege_desert; "
                "identity, class, inventory, and pane ownership were not asserted"
            )
        rows.append({"players": players, "cell": name.lower().replace("-", "_"),
                     "status": "DISCOVERED_FAIL" if fail else "COVERED_PASS",
                     "actual": actual})
    image = a.output.parent / f"{players}p/gp2_siege_class_team.png"
    proc = subprocess.run([sys.executable, str(a.screenshot_oracle), str(image), str(players)],
                          text=True, capture_output=True, check=False)
    rows.append({
        "players": players,
        "cell": "mission_objectives_menu_image",
        "status": "BLOCKED_NOT_COVERED",
        "actual": (
            "capture is a full-screen Mission Objectives/Join menu and cannot "
            "establish gameplay pane uniqueness; diagnostic oracle output: "
            + proc.stdout.strip()
        ),
    })
    for cell, reason in (
        ("routed_pane_class_selection", "classes were sent by commands; no routed pane flow"),
        ("class_inventory_server_state", "no class/inventory snapshot oracle"),
        ("objective_chain_state", "use command is observable but objective state is not exposed"),
        ("class_combat", "death probe uses kill; no deterministic enemy class combat"),
        ("round_end_side_swap_score", "no deterministic stock objective completion hook"),
        ("scoreboard_intermission_ready", "round end cannot be reached deterministically"),
        ("controlled_bot_remote_population", "no controlled Siege bot/remote fixture"),
    ):
        rows.append({"players": players, "cell": cell, "status": "BLOCKED_NOT_COVERED", "actual": reason})

summary = {s: sum(r["status"] == s for r in rows)
           for s in ("COVERED_PASS", "DISCOVERED_FAIL", "BLOCKED_NOT_COVERED")}
report = {"schema_version": 1, "ticket": "GP2-05",
          "frozen_binary_sha256": "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
          "summary": summary,
          "artifacts": [{"path": str(p), "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                        for p in a.logs], "cells": rows}
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
