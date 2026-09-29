#!/usr/bin/env python3
"""Build the GP1-04 discovery matrix from immutable modal run reports."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
RESULTS = HERE.parent / "results" / "modal_ownership"


def main() -> int:
    cases: list[dict[str, object]] = []
    manifests: list[str] = []
    for players in (2, 3, 4):
        report = json.loads((RESULTS / f"{players}p-report.json").read_text())
        manifest = (RESULTS / f"{players}p-manifest-path.txt").read_text().strip()
        manifests.append(manifest)
        for case in report["cases"]:
            passed = bool(case["passed"])
            cases.append(
                {
                    "id": f"{players}p-p{case['player']}-{case['surface']}",
                    "players": players,
                    "player": case["player"],
                    "surface": case["surface"],
                    "status": "COVERED_PASS" if passed else "DISCOVERED_FAIL",
                    "visual_differences": case["differences"],
                    "state_passed": case.get("state_passed"),
                    "evidence": case["path"],
                    "route": (
                        "GP4-01/GP4-05"
                        if case["surface"] == "top"
                        else "GP4-02"
                        if case["surface"] in ("score", "chat")
                        else "GP4-05"
                    ),
                }
            )

    blocked = [
        ("alternate-layouts", "Both layout modes for every surface"),
        ("chat-server-context", "Distinct global/team text and server recipient context"),
        ("console-p3-p4", "Distinct Player 3/4 console commands and history"),
        ("virtual-keyboard", "Virtual-keyboard ownership while gameplay continues"),
        ("controls-join-spectate", "Controls and join/spectate modal ownership"),
        ("death-respawn-held-input", "Death, respawn, and held input under a modal"),
        ("intermission", "Modal policy during intermission"),
        ("spectator", "Modal policy while spectating"),
        ("connection-error", "Modal policy on a connection-error screen"),
    ]
    for case_id, description in blocked:
        cases.append(
            {
                "id": case_id,
                "status": "BLOCKED_UNSUPPORTED",
                "description": description,
                "route": "GP4-01/GP4-02/GP4-05",
            }
        )

    counts = Counter(str(case["status"]) for case in cases)
    matrix = {
        "schema_version": 1,
        "ticket": "GP1-04",
        "policy": {
            "pause": "Multiplayer simulation continues; only the initiating player's pane and device own the menu.",
            "console": "Console is clipped to and edited by its initiating player.",
            "scoreboard": "Scoreboard press/release affects only the initiating player's cgame.",
            "chat": "Chat input and reliable command context belong to the initiating player.",
        },
        "manifests": manifests,
        "summary": dict(sorted(counts.items())),
        "acceptance_met": counts["DISCOVERED_FAIL"] == 0 and counts["BLOCKED_UNSUPPORTED"] == 0,
        "cases": cases,
    }
    (RESULTS / "matrix.json").write_text(json.dumps(matrix, indent=2) + "\n")
    print(
        "GP1-04 matrix: "
        + ", ".join(f"{key}={value}" for key, value in sorted(counts.items()))
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
