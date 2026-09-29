#!/usr/bin/env python3
"""Classify ordered GP3-03 recovery evidence from normal Phase 0 runs."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--index", type=Path, required=True)
ap.add_argument("--output", type=Path, required=True)
args = ap.parse_args()


def manifest_value(path: Path, key: str) -> str:
    for line in path.read_text().splitlines():
        fields = line.split("\t")
        if fields[0] == key:
            return "\t".join(fields[1:])
    return ""


def segment(text: str, name: str) -> str | None:
    begin, end = f"GP3-03:{name}-BEGIN", f"GP3-03:{name}-END"
    if begin not in text or end not in text:
        return None
    return text.split(begin, 1)[1].split(end, 1)[0]


entries = []
for line in args.index.read_text().splitlines():
    players_text, exit_text, manifest_text = line.split("\t", 2)
    entries.append((int(players_text), int(exit_text), Path(manifest_text)))

rows = []
manifests = []
phases = (
    ("BASELINE", "initial party has ordered ALIVE, team, and clientnum assertions"),
    ("SECONDARY-DISCONNECT", "target disconnect plus healthy-player identity/lifecycle"),
    ("SECONDARY-REJOIN", "ordered partial-to-active rejoin with stable clientnums"),
    ("RESTART", "post-restart ALIVE and stable clientnums"),
    ("MAP-TRANSITION", "post-map ALIVE and stable clientnums"),
    ("P1-PARTY-DISCONNECT", "whole-party reconnect with ordered clientnums"),
    ("SERVER-LOSS", "server-loss observation and whole-party recovery"),
)
for players, exit_code, manifest in entries:
    client_log = Path(manifest_value(manifest, "process.client.log"))
    server_log = Path(manifest_value(manifest, "process.server.log"))
    text = client_log.read_text(errors="replace") if client_log.is_file() else ""
    server_text = server_log.read_text(errors="replace") if server_log.is_file() else ""
    phase_status = {}
    for name, description in phases:
        data = segment(text, name)
        failed = (
            data is None
            or "Assert: FAIL" in data
            or "SplitNetStatAssert: FAIL" in data
            or "SplitNetLifecycleAssert: FAIL" in data
            or "SplitNetStateAssert: FAIL" in data
        )
        status = "DISCOVERED_FAIL" if failed else "COVERED_PASS"
        actual = "missing markers or assertion failure" if failed else description
        if name == "SECONDARY-REJOIN" and failed and data and "Too many connections from the same IP" in data:
            actual = (
                "target rejoin was rejected as Too many connections from the same IP; "
                "healthy earlier players retained ALIVE and stable clientnums"
            )
        elif name == "P1-PARTY-DISCONNECT" and failed:
            actual = (
                "ordered disconnect occurred, but reconnect remained connecting and "
                "all requested lifecycle/clientnum assertions were NOT_ACTIVE"
            )
        if name == "SECONDARY-REJOIN" and phase_status.get("SECONDARY-DISCONNECT") != "COVERED_PASS":
            status = "BLOCKED_PREREQUISITE"
            actual = "disconnect prerequisite was not proven"
        if name == "SERVER-LOSS" and data is not None:
            client_loss_seen = any(
                marker in data
                for marker in ("Server disconnected", "Connection interrupted", "timed out")
            )
            controlled_server_loss_seen = (
                "Rcon from " in server_text
                and ": quit" in server_text
                and server_text.count("------ Server Initialization ------") >= 2
            )
            loss_seen = client_loss_seen or controlled_server_loss_seen
            if not loss_seen and status == "COVERED_PASS":
                status = "BLOCKED_PREREQUISITE"
                actual = "post-reconnect assertions passed but authoritative server-loss state was not observed"
            elif controlled_server_loss_seen and status == "COVERED_PASS":
                actual = (
                    "authoritative server-loss observation from controlled-server "
                    "quit/restart and "
                    "whole-party recovery with stable clientnums"
                )
        phase_status[name] = status
        rows.append(
            {
                "players": players,
                "cell": name.lower().replace("-", "_"),
                "status": status,
                "actual": actual,
            }
        )
    unsupported = (
        ("password_required_wrong_correct", "no credentialed controlled-server fixture"),
        ("full_server_reserved_slots", "no capacity/reserved-slot fixture"),
        ("same_ip_duplicate_slot", "no authoritative same-IP/duplicate-slot oracle"),
        ("ban_kick_admin_reason", "no controlled ban/kick fixture"),
        ("protocol_version_mismatch", "single frozen protocol binary only"),
        ("pure_rejection_content_recovery", "owned suite does not mutate pure content"),
        ("timeout_connection_interrupted", "no deterministic packet-loss/timeout proxy"),
        ("reason_per_pane_no_secret_leak", "no semantic per-pane rejection-message oracle"),
        ("occupancy_returns_to_baseline", "no server occupancy snapshot oracle"),
    )
    for cell, reason in unsupported:
        status = "BLOCKED_NOT_COVERED"
        actual = reason
        if (
            players == 4
            and cell == "same_ip_duplicate_slot"
            and "Too many connections from the same IP" in text
        ):
            status = "DISCOVERED_FAIL"
            actual = (
                "authoritative same-IP rejection occurred during P4 rejoin, "
                "but retry did not recover P4 in that ordered cell"
            )
        rows.append(
            {
                "players": players,
                "cell": cell,
                "status": status,
                "actual": actual,
            }
        )
    manifests.append(
        {
            "players": players,
            "exit_code": exit_code,
            "path": str(manifest),
            "sha256": hashlib.sha256(manifest.read_bytes()).hexdigest(),
            "status": manifest_value(manifest, "status"),
            "client_log": str(client_log),
            "server_log": str(server_log),
        }
    )

summary = dict(sorted(Counter(row["status"] for row in rows).items()))
runtime_cells = {
    name.lower().replace("-", "_")
    for name, _description in phases
}
runtime_acceptance_met = all(
    row["status"] == "COVERED_PASS"
    for row in rows
    if row["cell"] in runtime_cells
)
acceptance_met = all(row["status"] == "COVERED_PASS" for row in rows)
report = {
    "schema_version": 1,
    "ticket": "GP3-03",
    "frozen_binary_sha256": "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
    "acceptance_met": acceptance_met,
    "runtime_acceptance_met": runtime_acceptance_met,
    "summary": summary,
    "manifests": manifests,
    "cells": rows,
}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
