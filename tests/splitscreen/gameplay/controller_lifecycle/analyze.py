#!/usr/bin/env python3
"""Produce the immutable GP1-03 discovery matrix from raw probe logs."""

import argparse
import hashlib
import json
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


parser = argparse.ArgumentParser()
parser.add_argument("--run1", type=Path, required=True)
parser.add_argument("--run2", type=Path, required=True)
parser.add_argument("--unsupported", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
logs = [p.read_text(errors="replace") for p in (args.run1, args.run2)]


def live_cell(cell, start, end):
    matching = []
    failures = []
    for text in logs:
        if start not in text or end not in text:
            matching.append(False)
            failures.append(True)
            continue
        segment = text.split(start, 1)[1].split(end, 1)[0]
        matching.append(True)
        failures.append("SplitInputAssertCmd: FAIL" in segment)
    passed = all(matching) and not any(failures)
    return {"cell": cell, "status": "COVERED_PASS" if passed else "DISCOVERED_FAIL",
            "expected": f"{start}..{end} without assertion failure",
            "actual": {"markers": matching, "assertion_failure": failures}, "runs": 2}


def completion_cell(cell):
    starts = ["External gamepad bridge: 3 SDL gamepads listening" in text for text in logs]
    finishes = ["GP1-CONTROLLER:COMPLETE" in text for text in logs]
    passed = all(starts) and all(finishes)
    return {"cell": cell, "status": "COVERED_PASS" if passed else "DISCOVERED_FAIL",
            "expected": "both fresh processes enumerate 3 devices and complete",
            "actual": {"enumerated": starts, "completed": finishes}, "runs": 2}


unsupported_reason = args.unsupported.read_text(errors="replace").strip()
rows = [
    live_cell("simultaneous_controllers_held_release",
              "GP1-CONTROLLER:READY-CONCURRENT", "GP1-CONTROLLER:RELEASE-CHECKED"),
    live_cell("pause_during_held_input",
              "GP1-CONTROLLER:READY-PAUSE", "GP1-CONTROLLER:PAUSE-CHECKED"),
    live_cell("in_restart_recovery",
              "GP1-CONTROLLER:READY-RESTART", "GP1-CONTROLLER:RESTART-CHECKED"),
    completion_cell("process_relaunch_stable_slots"),
    live_cell("gameplay_remap_attack_force_jump",
              "GP1-CONTROLLER:READY-CONCURRENT", "GP1-CONTROLLER:CONCURRENT-CHECKED"),
    live_cell("gameplay_remap_use_weapon",
              "GP1-CONTROLLER:READY-EXTRA-REMAP", "GP1-CONTROLLER:EXTRA-REMAP-CHECKED"),
]
for phase in ("setup", "gameplay", "pause", "held_axis_button"):
    rows.append({"cell": f"physical_disconnect_{phase}", "status": "BLOCKED_UNSUPPORTED",
                 "expected": "SDL device removed/added event", "actual": unsupported_reason})
for cell in (
    "reconnect_same_device", "reconnect_different_device", "enumeration_reorder",
    "explicit_ownership_after_reconnect", "duplicate_device_assignment",
    "missing_device_launch", "menu_accept_back_remap",
    "interrupted_bind_capture_physical_recovery",
    "simultaneous_keyboard_mouse_physical_controllers",
):
    rows.append({"cell": cell, "status": "BLOCKED_UNSUPPORTED",
                 "expected": "physical SDL lifecycle control", "actual": unsupported_reason})

report = {
    "schema_version": 1,
    "ticket": "GP1-03",
    "discovery_only": True,
    "summary": {
        "covered_pass": sum(r["status"] == "COVERED_PASS" for r in rows),
        "discovered_fail": sum(r["status"] == "DISCOVERED_FAIL" for r in rows),
        "blocked_unsupported": sum(r["status"] == "BLOCKED_UNSUPPORTED" for r in rows),
    },
    "artifacts": [
        {"path": str(path), "sha256": digest(path)}
        for path in (args.run1, args.run2, args.unsupported)
    ],
    "cells": rows,
}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report["summary"], sort_keys=True))
