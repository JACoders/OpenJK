#!/usr/bin/env python3
"""Build a fail-closed four-player certification report.

Absence of a failure string is never sufficient for a pass. Every local row
requires its positive assertion count, terminal marker, and durable artifact.
"""

from __future__ import annotations

from collections import Counter
import json
from pathlib import Path
import sys


run_root = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve()
expected = sys.argv[3]
rows: list[dict[str, str]] = []


def read(name: str) -> str:
    path = run_root / name
    return path.read_text(errors="replace") if path.is_file() else ""


def clean(text: str) -> bool:
    bad = (
        "Assert: FAIL",
        "SplitInputAssertCmd: FAIL",
        "SplitNetStagePair: FAIL",
        "Sys_Error",
        "ERROR:",
        "AddressSanitizer",
        "UndefinedBehaviorSanitizer",
    )
    return bool(text) and not any(marker in text for marker in bad)


def add(cell: str, passed: bool, actual: str, evidence: Path) -> None:
    rows.append(
        {
            "cell": cell,
            "status": "COVERED_PASS" if passed else "DISCOVERED_FAIL",
            "actual": actual,
            "evidence": str(evidence),
        }
    )


local = read("local.log")
local_complete = (
    clean(local)
    and local.count("SplitProfileAssert: PASS") >= 12
    and local.count("SplitInputAssertCmd: PASS") >= 11
    and local.count("SplitNetLifecycleAssert: PASS") >= 8
    and "SplitNetStagePair: PASS" in local
    and (run_root / "local-manifest.tsv").is_file()
)
add(
    "local_attack_force_and_simultaneous_input",
    local_complete,
    (
        f"profiles={local.count('SplitProfileAssert: PASS')} "
        f"input={local.count('SplitInputAssertCmd: PASS')} "
        f"lifecycle={local.count('SplitNetLifecycleAssert: PASS')}"
    ),
    run_root / "local.log",
)
profiles_complete = (
    clean(local)
    and local.count("SplitProfileAssert: PASS") >= 12
    and all(
        marker in local
        for marker in (
            "actual=QA_4P_1",
            "actual=QA_4P_2",
            "actual=QA_4P_3",
            "actual=QA_4P_4",
            "actual=kyle/default",
            "actual=jan/default",
            "actual=reborn/default",
            "actual=luke/default",
        )
    )
)
add(
    "four_distinct_profile_userinfos",
    profiles_complete,
    f"positive profile assertions={local.count('SplitProfileAssert: PASS')}",
    run_root / "local.log",
)

controlled = read("controlled.log")
controlled_complete = (
    clean(controlled)
    and controlled.count("SplitNetLifecycleAssert: PASS") >= 8
    and controlled.count("SplitNetStatAssert: PASS") >= 5
    and controlled.count(
        "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active"
    )
    >= 2
    and (run_root / "controlled-manifest.tsv").is_file()
)
add(
    "controlled_server_four_slots_rejoin",
    controlled_complete,
    (
        f"lifecycle={controlled.count('SplitNetLifecycleAssert: PASS')} "
        f"stats={controlled.count('SplitNetStatAssert: PASS')}"
    ),
    run_root / "controlled.log",
)

visible = read("visible-ui-runner.log")
visible_game_path = run_root / "visible-ui-home/base/routed-selection-4p.stdout.log"
visible_game = (
    visible_game_path.read_text(errors="replace")
    if visible_game_path.is_file()
    else ""
)
visible_complete = (
    clean(visible)
    and clean(visible_game)
    and "Four-player routed selection acceptance passed" in visible
    and (run_root / "visible-ui-manifest.tsv").is_file()
)
add(
    "visible_ui_profile_selection",
    visible_complete,
    "runner terminal success marker present" if visible_complete else "positive terminal marker missing",
    run_root / "visible-ui-runner.log",
)


def owned_manifest(case: str) -> Path | None:
    log = read(f"{case}-runner.log")
    candidates = [Path(line) for line in log.splitlines() if line.endswith("manifest.tsv")]
    return candidates[-1] if candidates else None


churn_log = read("slot-churn-runner.log")
churn_manifest = owned_manifest("slot-churn")
churn_complete = (
    read("slot-churn-status.txt").strip() == "passed"
    and clean(churn_log)
    and churn_manifest is not None
    and churn_manifest.is_file()
)
add(
    "varied_secondary_slot_churn",
    churn_complete,
    "owned runner and E2E manifest passed" if churn_complete else "owned runner incomplete or failed",
    run_root / "slot-churn-runner.log",
)

fifth_log = read("fifth-runner.log")
fifth_manifest = owned_manifest("fifth")
fifth_complete = (
    read("fifth-status.txt").strip() == "passed"
    and clean(fifth_log)
    and fifth_manifest is not None
    and fifth_manifest.is_file()
)
add(
    "independent_controlled_fifth_client",
    fifth_complete,
    "four stable local slots plus independent live fifth client passed"
    if fifth_complete
    else "owned runner incomplete or failed",
    run_root / "fifth-runner.log",
)

modal_log = read("modal-runner.log")
modal_path = run_root / "modal-report.json"
try:
    modal = json.loads(modal_path.read_text())
except (FileNotFoundError, json.JSONDecodeError):
    modal = {}
modal_cases = modal.get("cases", [])
modal_complete = (
    read("modal-status.txt").strip() == "passed"
    and clean(modal_log)
    and modal.get("schema_version") == 2
    and modal.get("players") == 4
    and modal.get("passed") is True
    and len(modal_cases) >= 8
    and all(case.get("passed") is True for case in modal_cases)
    and {case.get("owner") for case in modal_cases} == {2, 3, 4}
    and {"top", "score", "console", "chat"}
    <= {case.get("surface") for case in modal_cases}
)
add(
    "per_pane_modal_ownership",
    modal_complete,
    f"positive modal cases={sum(case.get('passed') is True for case in modal_cases)}",
    modal_path,
)

public = read("public.log")
public_status_text = ""
public_manifest = run_root / "public-manifest.tsv"
if public_manifest.is_file():
    for line in public_manifest.read_text().splitlines():
        if line.startswith("status\t"):
            public_status_text = line.split("\t", 1)[1]
            break
public_pass = (
    public_status_text == "passed"
    and clean(public)
    and "SplitNet party: all 4 local players active" in public
)
public_status = (
    "COVERED_PASS"
    if public_pass
    else ("BLOCKED_EXTERNAL" if public_status_text == "skipped" else "DISCOVERED_FAIL")
)
rows.append(
    {
        "cell": "safe_public_attempt",
        "status": public_status,
        "actual": (
            "four-player party reached active public server"
            if public_pass
            else (
                "not executed; authorization gate remained closed"
                if public_status_text == "skipped"
                else "positive four-player active proof missing"
            )
        ),
        "evidence": str(run_root / "public.log"),
    }
)

local_cells = [row for row in rows if row["cell"] != "safe_public_attempt"]
local_passed = bool(local_cells) and all(
    row["status"] == "COVERED_PASS" for row in local_cells
)
summary = dict(sorted(Counter(row["status"] for row in rows).items()))
report = {
    "schema_version": 2,
    "ticket": "GP5-03",
    "frozen_binary_sha256": expected,
    "run_root": str(run_root),
    "summary": summary,
    "local_certification_passed": local_passed,
    "certification_passed": local_passed and public_pass,
    "cells": rows,
    "manifests": [
        str(path)
        for path in (
            run_root / "local-manifest.tsv",
            run_root / "controlled-manifest.tsv",
            run_root / "visible-ui-manifest.tsv",
            public_manifest,
            churn_manifest,
            fifth_manifest,
        )
        if path is not None
    ],
}
output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
raise SystemExit(0 if report["certification_passed"] else 1)
