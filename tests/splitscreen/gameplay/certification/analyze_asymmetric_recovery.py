#!/usr/bin/env python3
"""Validate healthy-slot identity through one secondary-slot disconnect/rejoin."""

import argparse
import json
from pathlib import Path
import re


parser = argparse.ArgumentParser()
parser.add_argument("--players", type=int, choices=(2, 3), required=True)
parser.add_argument("--run-id", required=True)
parser.add_argument("--frozen-sha", required=True)
parser.add_argument("--log", type=Path, required=True)
parser.add_argument("--screenshots", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
text = args.log.read_text(encoding="utf-8", errors="replace")


def phase(name: str) -> str | None:
    begin = f"CERT-ASYMMETRIC:{name}-BEGIN"
    end = f"CERT-ASYMMETRIC:{name}-END"
    if begin not in text or end not in text:
        return None
    return text.split(begin, 1)[1].split(end, 1)[0]


segments = {name: phase(name) for name in ("BASELINE", "DISCONNECT", "REJOIN")}
phase_pass = {
    name: data is not None
    and re.search(r"Split(?:UI|NetLifecycle|NetStat)Assert: FAIL|Sys_Error", data) is None
    for name, data in segments.items()
}
identity_markers = []
for player in range(1, args.players):
    identity_markers += [
        f"SplitNetLifecycleAssert: PASS player={player} expected=ALIVE actual=ALIVE",
        f"SplitNetStatAssert: PASS player={player} field=clientnum op=eq expected={player - 1}",
    ]
disconnect = segments["DISCONNECT"] or ""
rejoin = segments["REJOIN"] or ""
healthy_identity_preserved = all(marker in disconnect for marker in identity_markers)
ordered_rejoin = all(
    f"SplitNetStatAssert: PASS player={player} field=clientnum op=eq expected={player - 1}"
    in rejoin
    for player in range(1, args.players + 1)
)
screenshots = {
    name: args.screenshots / f"cert_asymmetric_{name}.png"
    for name in ("baseline", "partial", "rejoined")
}
screenshots_present = all(path.is_file() and path.stat().st_size for path in screenshots.values())
passed = (
    all(phase_pass.values())
    and healthy_identity_preserved
    and ordered_rejoin
    and screenshots_present
    and "CERT-ASYMMETRIC:COMPLETE" in text
)
report = {
    "schema_version": 1,
    "kind": "asymmetric_network_recovery",
    "run_id": args.run_id,
    "players": args.players,
    "frozen_sha256": args.frozen_sha,
    "claim": "one secondary slot disconnects and rejoins while healthy slot identities remain stable",
    "passed": passed,
    "phases": phase_pass,
    "healthy_identity_preserved": healthy_identity_preserved,
    "ordered_rejoin": ordered_rejoin,
    "screenshots_present": screenshots_present,
    "whole_party_reconnect_covered": False,
    "server_loss_recovery_covered": False,
}
args.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, sort_keys=True))
raise SystemExit(0 if passed else 1)
