#!/usr/bin/env python3
"""Build an evidence-honest GP3-02 compatibility discovery matrix."""

import argparse
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--results", type=Path, required=True)
a = ap.parse_args()


def manifest_fields(path):
    fields = {}
    for line in path.read_text(errors="replace").splitlines():
        parts = line.split("\t", 2)
        if len(parts) >= 2:
            fields[parts[0]] = parts[1]
    return fields


def segment(text, name):
    begin, end = f"GP3-02:{name}-BEGIN", f"GP3-02:{name}-END"
    if begin not in text or end not in text:
        return ""
    return text.split(begin, 1)[1].split(end, 1)[0]


cells = []
artifacts = []
manifests = []
for pure_name, pure in (("nonpure", 0), ("pure", 1)):
    for players in (2, 3, 4):
        case = f"stock-{pure_name}-{players}p"
        pointer = a.results / f"{case}.latest"
        if not pointer.is_file():
            cells.append({"case": case, "players": players, "pure": pure,
                          "cell": "run", "status": "BLOCKED",
                          "claim": "Phase 0 run exists", "actual": "missing latest pointer"})
            continue
        manifest = Path(pointer.read_text().strip())
        fields = manifest_fields(manifest)
        log = Path(fields.get("process.client.log", ""))
        text = log.read_text(errors="replace") if log.is_file() else ""
        manifests.append(str(manifest))
        for path in (manifest, log):
            if path.is_file():
                artifacts.append({"path": str(path),
                                  "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})

        serverinfo_pure = f"\\sv_pure\\{pure}" in text
        cells.append({
            "case": case, "players": players, "pure": pure,
            "cell": "server_pure_mode_observation",
            "status": "COVERED_PASS" if serverinfo_pure else "DISCOVERED_FAIL",
            "claim": f"serverinfo reported sv_pure={pure}",
            "prerequisites": [f"literal serverinfo \\\\sv_pure\\\\{pure}"],
            "actual": {"serverinfo_match": serverinfo_pure},
        })

        checksum_players = []
        ordering = {}
        for player in range(1, players + 1):
            checksum = text.find(f"PureChecksums P{player}:")
            checksum_players.append(player) if checksum >= 0 else None
            if player >= 2:
                sent = text.find(f"SplitNet P{player}: sent primary pure checksums")
                ordering[str(player)] = checksum >= 0 and sent > checksum
        checksum_ok = len(checksum_players) == players and all(ordering.values())
        cells.append({
            "case": case, "players": players, "pure": pure,
            "cell": "checksum_emission_observation",
            "status": "COVERED_PASS" if checksum_ok else "DISCOVERED_FAIL",
            "claim": "each local client emitted a checksum command; secondary emission "
                     "preceded its 'sent primary pure checksums' log marker. "
                     "This does not prove ordering before first usermove.",
            "prerequisites": [f"PureChecksums P1..P{players}",
                              "each secondary checksum precedes its sent marker"],
            "actual": {"players_with_checksum": checksum_players,
                       "secondary_log_order": ordering,
                       "before_first_usermove_proven": False},
        })

        for marker, cell, claim in (
            ("INITIAL", "initial_slot_lifecycle",
             "all local clients were ALIVE with expected client numbers"),
            ("RESTART", "restart_slot_lifecycle",
             "all local clients were ALIVE with expected client numbers after map_restart"),
            ("MAP-TRANSITION", "stock_map_transition_slot_lifecycle",
             "all local clients were ALIVE with expected client numbers after stock map change"),
        ):
            data = segment(text, marker)
            required = []
            for player in range(1, players + 1):
                required += [
                    rf"SplitNetLifecycleAssert: PASS player={player} expected=ALIVE actual=ALIVE",
                    rf"SplitNetStatAssert: PASS player={player} field=clientnum op=eq "
                    rf"expected={player - 1} actual={player - 1}",
                ]
            missing = [item for item in required if item not in data]
            assertion_failure = bool(re.search(r"(?:Assert|AssertCmd|StagePair): FAIL", data))
            passed = bool(data) and not missing and not assertion_failure
            cells.append({
                "case": case, "players": players, "pure": pure,
                "cell": cell,
                "status": "COVERED_PASS" if passed else
                          ("DISCOVERED_FAIL" if data else "BLOCKED"),
                "claim": claim,
                "prerequisites": required + ["no assertion failure in stage"],
                "actual": {"missing": missing, "assertion_failure": assertion_failure},
            })

        screenshots = list(Path(fields.get("screenshots", "")).glob("gp3_compat_*.png"))
        cells.append({
            "case": case, "players": players, "pure": pure,
            "cell": "screenshot_artifact_observation",
            "status": "COVERED_SMOKE" if len(screenshots) == 3 else "DISCOVERED_FAIL",
            "claim": "three transition screenshots exist; no semantic pane oracle applied",
            "prerequisites": ["initial, restart, and map-transition PNG artifacts"],
            "actual": [str(path) for path in sorted(screenshots)],
        })

unsupported = (
    ("checksum_before_first_usermove", "No first-usermove timestamp/oracle is exposed."),
    ("qvm_module_compatibility", "This frozen-native discovery did not execute QVM modules."),
    ("missing_pak_download_success", "No controlled downloadable-pak HTTP endpoint/fixture."),
    ("download_cancel_retry", "No controlled slow download endpoint or routed cancel flow."),
    ("corrupt_hash_recovery", "No controlled corrupt-pak endpoint."),
    ("unavailable_download_recovery", "No controlled unavailable-download endpoint."),
    ("insufficient_space_recovery", "No scoped disk-exhaustion simulator."),
    ("third_party_mod_matrix", "No vendored, version-pinned third-party mod fixtures."),
    ("external_product_protocol_matrix", "No controlled legacy/external product endpoints."),
    ("vm_reload_state_ownership", "No authoritative UI/cgame/game VM reload ownership oracle."),
    ("userinfo_identity_ownership", "Client numbers are asserted, but per-pane userinfo identity is not."),
    ("device_routing_after_transition", "No post-transition assigned-device input assertion."),
    ("ui_pane_ownership", "Screenshots exist, but no semantic UI ownership oracle is applied."),
    ("stale_snapshot_duplicate_pane", "No snapshot-age or duplicate-pane semantic oracle."),
)
for cell, reason in unsupported:
    cells.append({"case": "cross-case", "players": "2/3/4", "pure": "0/1",
                  "cell": cell, "status": "BLOCKED_UNSUPPORTED",
                  "claim": reason, "actual": reason})

summary = dict(sorted(Counter(cell["status"] for cell in cells).items()))
report = {
    "schema_version": 1,
    "ticket": "GP3-02",
    "discovery_only": True,
    "frozen_binary_sha256":
        "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13",
    "acceptance_met": False,
    "summary": summary,
    "manifests": manifests,
    "artifacts": artifacts,
    "cells": cells,
}
(a.results / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(summary, sort_keys=True))
