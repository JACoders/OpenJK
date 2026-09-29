#!/usr/bin/env python3
"""Classify narrow CTF/CTY assertions without umbrella lifecycle overclaims."""

from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
RESULTS = HERE.parents[1] / "results" / "modes" / "ctf_cty"


def passed(text: str, fragment: str, minimum: int = 1) -> bool:
    return text.count(fragment) >= minimum


def add_case(
    cases: list[dict[str, object]],
    mode: str,
    players: int,
    name: str,
    ok: bool,
    evidence: str,
    manifest: Path,
    log: Path,
) -> None:
    cases.append(
        {
            "id": f"{mode}-{players}p-{name}",
            "mode": mode,
            "players": players,
            "status": "COVERED_PASS" if ok else "DISCOVERED_FAIL",
            "evidence_rule": evidence,
            "manifest": str(manifest),
            "log": str(log),
            "route": "GP4-03/GP4-04/GP4-05",
            "scope": "staged authoritative probe; not a visible-UI end-to-end claim",
        }
    )


def main() -> int:
    cases: list[dict[str, object]] = []
    manifests: list[str] = []
    for mode in ("ctf", "cty"):
        for players in (2, 3, 4):
            pointer = RESULTS / f"{mode}-{players}p-manifest-path.txt"
            if not pointer.is_file():
                continue
            manifest = Path(pointer.read_text().strip())
            manifests.append(str(manifest))
            fields: dict[str, str] = {}
            for line in manifest.read_text().splitlines():
                parts = line.split("\t")
                if len(parts) >= 2:
                    fields[parts[0]] = parts[1]
            log = Path(fields["process.client.log"])
            text = log.read_text(errors="replace")

            team_ok = all(
                passed(text, f"SplitNetStatAssert: PASS player={player} field=team")
                for player in range(1, players + 1)
            )
            client_ok = all(
                passed(
                    text,
                    f"SplitNetStatAssert: PASS player={player} field=clientnum "
                    f"op=eq expected={player - 1} actual={player - 1}",
                )
                for player in range(1, players + 1)
            )
            add_case(cases, mode, players, "setup-identity", team_ok and client_ok,
                     "all expected team and unique client-number assertions pass", manifest, log)

            first_pickup = (
                passed(text, "SplitNetStatAssert: PASS player=1 field=score op=ge expected=10")
                and "got the BLUE flag!" in text
            )
            add_case(cases, mode, players, "enemy-objective-pickup", first_pickup,
                     "score>=10 and server message confirms BLUE objective pickup", manifest, log)

            carrier_death = passed(
                text,
                "SplitNetLifecycleAssert: PASS player=1 expected=DEAD actual=DEAD",
            )
            add_case(cases, mode, players, "carrier-death", carrier_death,
                     "authoritative P1 DEAD snapshot after carrying", manifest, log)

            return_repickup = (
                carrier_death
                and re.search(
                    r"clientCommand: Objective_P2 .* setviewpos 512 0 988 270",
                    text,
                )
                and passed(text, "SplitNetStatAssert: PASS player=1 field=score op=ge expected=19")
                and text.count("got the BLUE flag!") >= 2
            )
            add_case(cases, mode, players, "return-and-repickup", return_repickup,
                     "P2 touches dropped-objective area, then P1 can pick up reset base objective", manifest, log)

            capture_score = any(
                int(value) >= 100
                for value in re.findall(
                    r"SplitNetStatAssert: PASS player=1 field=score op=ge expected=1 actual=(-?\d+)",
                    text,
                )
            )
            capture_ok = (
                return_repickup
                and capture_score
                and "captured the BLUE flag!" in text
                and "hit the capture limit" in text
            )
            add_case(cases, mode, players, "capture-limit", capture_ok,
                     "score>=100 plus authoritative capture and capture-limit server messages", manifest, log)

            intermission_ok = all(
                passed(
                    text,
                    f"SplitNetLifecycleAssert: PASS player={player} "
                    "expected=INTERMISSION actual=INTERMISSION",
                )
                for player in range(1, players + 1)
            )
            add_case(cases, mode, players, "intermission", intermission_ok,
                     "every local snapshot is PM_INTERMISSION", manifest, log)

            restart_ok = all(
                passed(
                    text,
                    f"SplitNetStatAssert: PASS player={player} field=clientnum "
                    f"op=eq expected={player - 1} actual={player - 1}",
                    minimum=2,
                )
                and passed(
                    text,
                    f"SplitNetLifecycleAssert: PASS player={player} expected=ALIVE actual=ALIVE",
                    minimum=2,
                )
                for player in range(1, players + 1)
            )
            add_case(cases, mode, players, "restart-identity", restart_ok,
                     "post-restart ALIVE and original unique client numbers reassert for every player", manifest, log)

            add_case(cases, mode, players, "clean-exit", fields.get("status") == "passed",
                     "Phase0 process status is passed", manifest, log)

    unsupported = (
        ("ctf-objective-location", "Direct objective owner/location oracle"),
        ("ctf-timed-return", "Timed flag return"),
        ("ctf-disconnect-carrier", "Carrier disconnect without duplication"),
        ("ctf-remote-fifth", "Remote fifth-player interaction"),
        ("ctf-next-map", "Post-intermission next-map rotation"),
        ("cty-force-restriction", "Carrier Force restriction with assigned-device input"),
        ("cty-transfer", "Ysalamiri carrier transfer"),
        ("cty-disconnect-carrier", "Carrier disconnect without stranded objective"),
        ("cty-remote-fifth", "Remote fifth-player interaction"),
        ("exact-team-score", "Machine-readable exact team score"),
        ("ui-selection", "Fresh-home per-pane team/profile selection through visible UI"),
    )
    for case_id, description in unsupported:
        cases.append(
            {
                "id": case_id,
                "status": "BLOCKED_UNSUPPORTED",
                "description": description,
                "route": "GP4-03/GP4-04/GP4-05",
            }
        )

    counts = Counter(str(case["status"]) for case in cases)
    matrix = {
        "schema_version": 2,
        "ticket": "GP2-04",
        "frozen_binary_sha256": "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
        "summary": dict(sorted(counts.items())),
        "acceptance_met": counts["DISCOVERED_FAIL"] == 0 and counts["BLOCKED_UNSUPPORTED"] == 0,
        "manifests": manifests,
        "cases": cases,
    }
    RESULTS.mkdir(parents=True, exist_ok=True)
    (RESULTS / "matrix.json").write_text(json.dumps(matrix, indent=2) + "\n")
    print(", ".join(f"{key}={value}" for key, value in sorted(counts.items())))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
