#!/usr/bin/env python3
import argparse, hashlib, json, re, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
FROZEN = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"


def manifest_fields(path):
    fields = {}
    for line in path.read_text().splitlines():
        parts = line.split("\t", 2)
        if len(parts) >= 2:
            fields.setdefault(parts[0], []).append(parts[1:])
    return fields


def ordered(text, *needles):
    pos = -1
    for needle in needles:
        pos = text.find(needle, pos + 1)
        if pos < 0:
            return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", type=Path, required=True)
    ap.add_argument("--require-acceptance", action="store_true")
    args = ap.parse_args()
    args.results = args.results.resolve()
    cells, findings = [], []
    binary = ROOT / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    if hashlib.sha256(binary.read_bytes()).hexdigest() != FROZEN:
        findings.append({"id": "GP3-01-BUILD", "kind": "evidence-invalid",
                         "summary": "frozen binary hash mismatch"})
    for players in (2, 3, 4):
        runs = sorted((args.results / f"listen-{players}p-lan").glob("*/manifest.tsv"))
        cell = {"players": players, "status": "missing", "proven": [],
                "attempted_smoke": [], "unsupported": []}
        if not runs:
            findings.append({"id": f"GP3-01-{players}P-MISSING",
                             "kind": "evidence-invalid", "summary": "missing manifest"})
            cells.append(cell)
            continue
        manifest = runs[-1]
        cell["manifest"] = str(manifest.relative_to(args.results))
        replay = subprocess.run([str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"),
                                 "--validate-only", str(manifest)],
                                text=True, capture_output=True)
        data = manifest_fields(manifest)
        client = Path(data["process.client.log"][-1][0])
        remote = Path(data["process.remote.log"][-1][0])
        host_text = client.read_text(errors="replace")
        remote_text = remote.read_text(errors="replace")
        failures = len(re.findall(r"Assert: FAIL|ERROR:|Sys_Error", host_text))
        party = f"all {players} local players active"
        alive = [f"player={p} expected=ALIVE actual=ALIVE" for p in range(1, players + 1)]
        attached = ordered(host_text, "GP301:ATTACHED", *alive)
        restarted = ordered(host_text, "GP301:RESTART", *alive)
        remote_join = "GP301_Remote" in host_text and "connected" in remote_text.lower()
        input_pass = f"SplitInputAssertCmd: PASS player={players}" in host_text
        pngs = sum(1 for row in data.get("artifact", []) if row[0].endswith(".png"))
        if attached:
            cell["proven"].append("ordered_local_attach_alive")
        if restarted:
            cell["proven"].append("ordered_post_restart_alive")
        if input_pass:
            cell["proven"].append("routed_last_player_input")
        if remote_join:
            cell["proven"].append("independent_loopback_client_join")
        cell["attempted_smoke"] = [
            "listen_host_direct_devmap", "ffa_server_settings", "graceful_process_exit",
            "screenshot_presence"
        ]
        cell["unsupported"] = [
            "visible_create_server_ui", "lan_browser_discovery",
            "serverinfo_exact_assertions", "team_terminal", "objective_terminal",
            "bot_add_remove", "password_change", "primary_disconnect",
            "no_host_migration", "listener_probe_after_shutdown"
        ]
        cell.update({"phase0_valid": replay.returncode == 0, "assert_fail": failures,
                     "screenshots": pngs,
                     "status": "partial" if replay.returncode == 0 and not failures else "failed"})
        if replay.returncode:
            findings.append({"id": f"GP3-01-{players}P-MANIFEST",
                             "kind": "evidence-invalid", "summary": replay.stderr.strip()})
        if failures:
            findings.append({"id": f"GP3-01-{players}P-ASSERT",
                             "kind": "product-or-harness", "summary": f"{failures} failure markers"})
        for prerequisite, value in (
            ("ordered local attach", attached), ("post-restart alive", restarted),
            ("routed input", input_pass), ("remote join", remote_join)):
            if not value:
                findings.append({"id": f"GP3-01-{players}P-{prerequisite.upper().replace(' ', '-')}",
                                 "kind": "coverage-gap", "summary": f"missing {prerequisite} evidence"})
        cells.append(cell)
    report = {"schema_version": 1, "ticket": "GP3-01", "status":
              "failed" if findings else "partial", "frozen_sha256": FROZEN,
              "cells": cells, "findings": findings}
    out = args.results / "current"
    out.mkdir(parents=True, exist_ok=True)
    (out / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# GP3-01 local-host/LAN findings", "", f"Status: **{report['status'].upper()}**", "",
             "| Players | Result | Proven | Attempted/smoke | Unsupported | PNGs |",
             "|---:|---|---|---|---|---:|"]
    for c in cells:
        lines.append(f"| {c['players']} | {c['status']} | {', '.join(c['proven']) or 'none'} | "
                     f"{', '.join(c['attempted_smoke']) or 'none'} | "
                     f"{', '.join(c['unsupported']) or 'none'} | {c.get('screenshots', 0)} |")
    lines += ["", "## Findings", ""]
    lines += [f"- `{f['id']}` ({f['kind']}): {f['summary']}" for f in findings]
    (out / "findings.md").write_text("\n".join(lines) + "\n")
    print(json.dumps({"status": report["status"], "cells": len(cells),
                      "findings": len(findings)}))
    acceptance_incomplete = bool(findings) or any(
        cell["status"] != "partial" or cell["unsupported"] for cell in cells)
    return 1 if args.require_acceptance and acceptance_incomplete else 0


if __name__ == "__main__":
    raise SystemExit(main())
