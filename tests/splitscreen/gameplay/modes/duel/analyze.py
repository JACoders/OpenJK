#!/usr/bin/env python3
"""Validate GP2-02 evidence and emit an honest lifecycle matrix."""

import argparse
import hashlib
import json
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--results", type=Path, required=True)
args = parser.parse_args()

STAT_PASS = re.compile(r"SplitNetStatAssert: PASS player=(\d+) field=(\w+).*actual=(-?\d+)")
LIFECYCLE = re.compile(
    r"SplitNetLifecycleAssert: (PASS|FAIL) player=(\d+) expected=(\w+) actual=(\w+)"
)


def add_cell(mode, players, cell, status, claim, prerequisites, actual):
    cells.append({
        "mode": mode,
        "players": players,
        "cell": cell,
        "status": status,
        "claim": claim,
        "prerequisites": prerequisites,
        "actual": actual,
    })


def segment(text, marker, following):
    start = text.find(f"GP2-DUEL:{marker}")
    if start < 0:
        return ""
    ends = [text.find(f"GP2-DUEL:{candidate}", start + 1) for candidate in following]
    ends = [position for position in ends if position >= 0]
    return text[start:min(ends) if ends else len(text)]


cells = []
artifacts = []
for mode in ("duel", "powerduel"):
    for players in (2, 3, 4):
        manifests = sorted((args.results / f"{mode}-{players}p").glob("*/manifest.tsv"))
        if not manifests:
            cells.append({"mode": mode, "players": players, "cell": "launch",
                          "status": "BLOCKED", "actual": "no manifest"})
            continue
        manifest = manifests[-1]
        lines = manifest.read_text(errors="replace").splitlines()
        log_path = next((Path(x.split("\t", 1)[1]) for x in lines
                         if x.startswith("process.client.log\t")), None)
        text = log_path.read_text(errors="replace") if log_path and log_path.exists() else ""
        markers = (
            ["INITIAL", "DUEL-CAPACITY", "ROUND-P1-WIN", "TURNOVER", "COMPLETE"]
            if mode == "duel" else
            ["INITIAL", "POWER-CAPACITY", "COMPLETE"]
        )
        if mode == "duel" and players >= 3:
            markers += ["VOLUNTARY-SPECTATE-REJOIN", "DISCONNECT-REJOIN"]
        if mode == "powerduel" and players == 3:
            markers += ["POWER-ROUND-SINGLE-WIN", "POWER-ROLE-ROTATION"]
        if mode == "powerduel" and players != 3:
            markers += ["POWER-UNSUPPORTED-CAPACITY"]
        segments = {marker: segment(text, marker, markers) for marker in markers}

        initial_present = bool(segments["INITIAL"])
        add_cell(mode, players, "launch_observation",
                 "COVERED_SMOKE" if initial_present else "BLOCKED",
                 "case reached the initial lifecycle marker",
                 ["GP2-DUEL:INITIAL marker"],
                 "marker observed" if initial_present else "marker missing")

        capacity_marker = "DUEL-CAPACITY" if mode == "duel" else "POWER-CAPACITY"
        capacity = segments[capacity_marker]
        lifecycle = [(int(player), actual) for _, player, _, actual in LIFECYCLE.findall(capacity)]
        observed = {player: state for player, state in lifecycle}
        if mode == "duel":
            alive = sum(state == "ALIVE" for state in observed.values())
            spectators = sum(state == "SPECTATOR" for state in observed.values())
            population_ok = len(observed) == players and alive == 2 and spectators == players - 2
            add_cell(mode, players, "observed_population",
                     "COVERED_PASS" if population_ok else
                     ("DISCOVERED_FAIL" if capacity else "BLOCKED"),
                     "observed two active duelists and the remaining local clients as spectators",
                     [
                         f"lifecycle state recorded for all {players} local clients",
                         "exactly 2 actual=ALIVE",
                         f"exactly {players - 2} actual=SPECTATOR",
                     ],
                     {"states": observed, "alive": alive, "spectators": spectators})
            identity_failures = [
                {"player": int(player), "expected": expected, "actual": actual}
                for result, player, expected, actual in LIFECYCLE.findall(capacity)
                if result == "FAIL"
            ]
            if identity_failures:
                add_cell(mode, players, "connection_order_identity_assumption",
                         "DISCOVERED_FAIL",
                         "the probe's connection-order active/spectator identity expectation",
                         ["all per-player lifecycle expectations pass"],
                         identity_failures)
        else:
            add_cell(mode, players, "population_marker_observation",
                     "COVERED_SMOKE" if capacity else "BLOCKED",
                     "Power Duel capacity stage was reached; no authoritative role/population assertion",
                     ["GP2-DUEL:POWER-CAPACITY marker"],
                     "marker observed" if capacity else "marker missing")

        score_marker = "ROUND-P1-WIN" if mode == "duel" else "POWER-ROUND-SINGLE-WIN"
        if score_marker in segments:
            score_segment = segments[score_marker]
            stage_pass = "SplitNetStagePair: PASS" in score_segment
            score_passes = [
                {"player": int(player), "field": field, "actual": int(actual)}
                for player, field, actual in STAT_PASS.findall(score_segment)
                if field == "score"
            ]
            score_fail = "SplitNetStatAssert: FAIL player=1 field=score" in score_segment
            add_cell(mode, players, "scripted_attack_score_evidence",
                     "COVERED_PASS" if stage_pass and score_passes and not score_fail else
                     ("DISCOVERED_FAIL" if score_segment else "BLOCKED"),
                     "a staged attack was followed by a passing personal-score assertion; "
                             "this is not a round-win or death proof",
                     ["SplitNetStagePair: PASS", "passing score assertion", "no score assertion failure"],
                     {"stage_pair_pass": stage_pass, "score_passes": score_passes,
                               "score_assertion_failed": score_fail})

        for marker, cell, claim in (
            ("TURNOVER", "turnover_marker_observation",
             "turnover marker reached; no promotion or queue assertion"),
            ("POWER-ROLE-ROTATION", "role_rotation_marker_observation",
             "role-rotation marker reached; no authoritative role assertion"),
            ("DISCONNECT-REJOIN", "disconnect_rejoin_marker_observation",
             "disconnect/rejoin marker reached; no lifecycle or identity assertion"),
            ("POWER-UNSUPPORTED-CAPACITY", "unsupported_capacity_marker_observation",
             "unsupported-capacity marker reached; no behavior assertion"),
            ("COMPLETE", "completion_observation",
             "case reached its completion marker"),
        ):
            if marker in segments:
                present = bool(segments[marker])
                add_cell(mode, players, cell, "COVERED_SMOKE" if present else "BLOCKED",
                         claim, [f"GP2-DUEL:{marker} marker"],
                         "marker observed" if present else "marker missing")

        if "VOLUNTARY-SPECTATE-REJOIN" in segments:
            spectate = segments["VOLUNTARY-SPECTATE-REJOIN"]
            states = [
                {"result": result, "player": int(player),
                 "expected": expected, "actual": actual}
                for result, player, expected, actual in LIFECYCLE.findall(spectate)
            ]
            failed = any(state["result"] == "FAIL" for state in states)
            # In the 3p run P3 was already a spectator before this stage, and
            # there is no post-rejoin ALIVE assertion.  Preserve it only as an
            # observation rather than claiming a spectate/rejoin transition.
            status = "DISCOVERED_FAIL" if failed else (
                "COVERED_SMOKE" if spectate else "BLOCKED"
            )
            add_cell(mode, players, "spectate_rejoin_attempt",
                     status,
                     "spectate/rejoin command stage; PASS requires an asserted active→spectator→active transition",
                     [
                         "precondition assertion actual=ALIVE",
                         "post-spectate assertion actual=SPECTATOR",
                         "post-rejoin assertion actual=ALIVE",
                     ],
                     {"assertions": states,
                      "transition_proven": False,
                      "note": "3p target was pre-existing spectator; no rejoin ALIVE assertion"
                              if players == 3 else "spectate assertion failed"})
        artifacts.append({"path": str(manifest),
                          "sha256": hashlib.sha256(manifest.read_bytes()).hexdigest()})
        if log_path and log_path.exists():
            artifacts.append({"path": str(log_path),
                              "sha256": hashlib.sha256(log_path.read_bytes()).hexdigest()})

for cell in (
    "authoritative_queue_order", "duel_role_numeric_assertion",
    "both_directions_multi_round", "ready_and_next_match",
    "camera_model_hud_inheritance",
):
    cells.append({"mode": "cross-mode", "players": "2/3/4", "cell": cell,
                  "status": "BLOCKED_UNSUPPORTED",
                  "actual": "no production diagnostic exposes this authoritative field"})

summary = {status: sum(c["status"] == status for c in cells)
           for status in ("COVERED_PASS", "COVERED_SMOKE", "DISCOVERED_FAIL", "BLOCKED",
                          "BLOCKED_UNSUPPORTED")}
report = {"schema_version": 1, "ticket": "GP2-02", "discovery_only": True,
          "summary": summary, "cells": cells, "artifacts": artifacts}
(args.results / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
