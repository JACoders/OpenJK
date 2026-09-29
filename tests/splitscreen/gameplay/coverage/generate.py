#!/usr/bin/env python3
"""Validate immutable evidence and generate the split-screen coverage ledger."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
DEFAULT_SOURCE = Path(__file__).with_name("ledger.json")
DEFAULT_JSON = ROOT / "docs/splitscreen-gameplay-hardening/generated/coverage.json"
DEFAULT_MD = ROOT / "docs/splitscreen-gameplay-hardening/generated/coverage.md"
VALID = {"missing", "smoke", "pass"}
BUILD_CORRELATIONS = {"legacy-unverified", "frozen-verified"}


class LedgerError(RuntimeError):
    pass


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def checked_file(item: dict, label: str) -> Path:
    path = ROOT / item["path"]
    if not path.is_file():
        raise LedgerError(f"missing {label}: {item['path']}")
    actual = digest(path)
    if actual != item["sha256"]:
        raise LedgerError(f"stale {label}: {item['path']} expected={item['sha256']} actual={actual}")
    return path


def validate(data: dict) -> None:
    states = data["states"]
    if len(states) != len(set(states)):
        raise LedgerError("contradictory schema: duplicate lifecycle state")
    if data.get("build_correlation") not in BUILD_CORRELATIONS:
        raise LedgerError("missing or invalid build correlation")
    binary = checked_file({"path": data["build"]["binary"], "sha256": data["build"]["binary_sha256"]}, "binary")
    if digest(binary) != data["build"]["binary_sha256"]:
        raise LedgerError("wrong build")
    ids, cells = set(), {}
    for record in data["evidence"]:
        if record["id"] in ids:
            raise LedgerError(f"contradictory evidence id: {record['id']}")
        ids.add(record["id"])
        if record["players"] not in (2, 3, 4) or record["mode"] not in data["modes"]:
            raise LedgerError(f"invalid cell: {record['id']}")
        for label in ("runner", "config", "log"):
            checked_file(record[label], label)
        if not record.get("screenshots"):
            raise LedgerError(f"claim has no required screenshot: {record['id']}")
        for shot in record["screenshots"]:
            path = checked_file(shot, "screenshot")
            if path.read_bytes()[:8] != b"\x89PNG\r\n\x1a\n":
                raise LedgerError(f"invalid screenshot: {shot['path']}")
        log = (ROOT / record["log"]["path"]).read_text(errors="replace")
        if "Assert: FAIL" in log:
            raise LedgerError(f"failing assertion in log: {record['id']}")
        unknown = set(record["claims"]) - set(states)
        if unknown:
            raise LedgerError(f"unknown lifecycle states in {record['id']}: {sorted(unknown)}")
        for state, marker in record["claims"].items():
            if not marker or marker not in log:
                raise LedgerError(f"missing assertion marker: {record['id']}:{state}: {marker}")
        cell = (record["players"], record["mode"])
        overlap = cells.setdefault(cell, set()) & set(record["claims"])
        # Multiple artifacts may corroborate a state, but their markers must not disagree.
        for prior in data["evidence"]:
            if prior is record or (prior["players"], prior["mode"]) != cell:
                continue
            for state in overlap:
                if prior["claims"].get(state) != record["claims"][state]:
                    raise LedgerError(f"contradictory claim: {cell}:{state}")
        cells[cell].update(record["claims"])


def build(data: dict) -> dict:
    records = {(p, mode): {state: "missing" for state in data["states"]}
               for p in (2, 3, 4) for mode in data["modes"]}
    evidence_ids = {(p, mode): [] for p in (2, 3, 4) for mode in data["modes"]}
    for evidence in data["evidence"]:
        cell = (evidence["players"], evidence["mode"])
        evidence_ids[cell].append(evidence["id"])
        for state in evidence["claims"]:
            records[cell][state] = "pass"
    cells = []
    for (players, mode), states in records.items():
        claimed = sum(value == "pass" for value in states.values())
        classification = "missing" if not claimed else ("smoke" if claimed <= 3 else "partial")
        ids = evidence_ids[(players, mode)]
        correlations = [
            item.get("build_correlation", data["build_correlation"])
            for item in data["evidence"] if item["id"] in ids
        ]
        cells.append({"players": players, "mode": mode, "classification": classification,
                      "states": states, "evidence": ids,
                      "seal_eligible": bool(ids) and all(
                          value == "frozen-verified" for value in correlations)})
    return {"schema_version": 1, "build": data["build"], "states": data["states"],
            "build_correlation": data["build_correlation"], "modes": data["modes"],
            "evidence": data["evidence"], "cells": cells}


def markdown(output: dict) -> str:
    states = output["states"]
    lines = ["# Executed split-screen gameplay coverage", "",
             "Generated from hash-validated logs and screenshots. `P` is an executed pass; `—` is unclaimed.",
             "", f"Build commit: `{output['build']['git_commit']}`",
             f"Binary SHA-256: `{output['build']['binary_sha256']}`", "",
             f"Build correlation: `{output['build_correlation']}`. Historical logs do not embed "
             "the binary hash; these imports are not eligible for a frozen-build end-to-end or seal claim.",
             "",
             "| Players | Mode | Class | " + " | ".join(states) + " | Evidence |",
             "|---:|---|---|" + "|".join("---" for _ in states) + "|---|"]
    for cell in output["cells"]:
        marks = ["P" if cell["states"][state] == "pass" else "—" for state in states]
        evidence = ", ".join(f"`{item}`" for item in cell["evidence"]) or "none"
        lines.append(f"| {cell['players']} | {cell['mode']} | {cell['classification']} | " +
                     " | ".join(marks) + f" | {evidence} |")
    lines += ["", "## Explicit gaps", ""]
    empty = [c for c in output["cells"] if c["classification"] == "missing"]
    smoke = [c for c in output["cells"] if c["classification"] == "smoke"]
    lines.append(f"- Empty cells ({len(empty)}): " + ", ".join(f"{c['players']}p {c['mode']}" for c in empty))
    lines.append(f"- Smoke-only cells ({len(smoke)}): " +
                 (", ".join(f"{c['players']}p {c['mode']}" for c in smoke) or "none"))
    lines.append("- No imported evidence claims objective, next-map, or clean-exit coverage.")
    lines += ["", "## Evidence links", ""]
    for item in output["evidence"]:
        links = [item["log"]["path"]] + [s["path"] for s in item["screenshots"]]
        lines.append(f"- `{item['id']}`: " + ", ".join(f"[{Path(p).name}](../../../{p})" for p in links))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--json", type=Path, default=DEFAULT_JSON)
    parser.add_argument("--markdown", type=Path, default=DEFAULT_MD)
    args = parser.parse_args()
    data = json.loads(args.source.read_text())
    validate(data)
    output = build(data)
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(output, indent=2) + "\n")
    args.markdown.write_text(markdown(output))
    print(f"CoverageLedger: PASS evidence={len(data['evidence'])} cells={len(output['cells'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
