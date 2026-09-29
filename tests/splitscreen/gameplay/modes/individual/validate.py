#!/usr/bin/env python3
import argparse, hashlib, json, re, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
FROZEN = "737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fields(path):
    result = {}
    for line in path.read_text().splitlines():
        parts = line.split("\t", 2)
        if len(parts) >= 2:
            result.setdefault(parts[0], []).append(parts[1:])
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", type=Path, required=True)
    ap.add_argument("--require-acceptance", action="store_true",
                    help="exit nonzero when discovery contains product findings")
    args = ap.parse_args()
    args.results = args.results.resolve()
    binary = ROOT / "build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
    findings, cells = [], []
    if sha(binary) != FROZEN:
        findings.append({"id": "GP2-01-BUILD", "severity": "blocker",
                         "actual": sha(binary), "expected": FROZEN})
    for mode in ("ffa", "holocron", "jedimaster"):
        for players in (2, 3, 4):
            manifests = sorted((args.results / f"{mode}-{players}p").glob("*/manifest.tsv"))
            cell = {"mode": mode, "players": players, "status": "missing",
                    "proven": [], "attempted_smoke": [], "unsupported": []}
            if not manifests:
                findings.append({"id": f"GP2-01-{mode}-{players}P-MISSING",
                                 "severity": "blocker", "summary": "No manifest"})
                cells.append(cell)
                continue
            manifest = manifests[-1]
            cell["manifest"] = str(manifest.relative_to(args.results))
            checked = subprocess.run(
                [str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"),
                 "--validate-only", str(manifest)], text=True, capture_output=True)
            data = fields(manifest)
            log = Path(data.get("process.client.log", [[""]])[-1][0])
            text = log.read_text(errors="replace") if log.is_file() else ""
            fail_count = len(re.findall(r"Assert: FAIL", text))
            pass_count = len(re.findall(r"Assert: PASS", text))
            shots = sum(1 for parts in data.get("artifact", []) if parts[0].endswith(".png"))
            cell.update({"phase0_valid": checked.returncode == 0,
                         "assert_pass": pass_count, "assert_fail": fail_count,
                         "screenshots": shots})
            def ordered(*needles):
                position = -1
                for needle in needles:
                    position = text.find(needle, position + 1)
                    if position < 0:
                        return False
                return True

            party = f"all {players} local players active"
            initial_alive = [f"player={player} expected=ALIVE actual=ALIVE"
                             for player in range(1, players + 1)]
            dead = "player=2 expected=DEAD actual=DEAD"
            spectator_player = 2 if players == 2 else players
            spectator = f"player={spectator_player} expected=SPECTATOR actual=SPECTATOR"
            rejoined = f"player={spectator_player} expected=ALIVE actual=ALIVE"
            proven = {
                "spawn": ordered(party, *initial_alive),
                "score": "field=score op=ge expected=1 actual=1" in text,
                "death": dead in text,
                "respawn": ordered(dead, "player=2 expected=ALIVE actual=ALIVE"),
                "spectate_rejoin": ordered(spectator, rejoined),
            }
            attempted = {
                "routed_combat_input": "SplitInputSim: key device=keyboard" in text,
                "restart_command": "map_restart 0" in (
                    HERE / "cfg" / f"lifecycle_{players}p.cfg").read_text(),
                "next_map_screenshots": shots >= 3,
                "spectate_state": spectator in text,
                "party_attachment_observed": party in text,
            }
            cell["proven"] = [key for key, value in proven.items() if value]
            cell["attempted_smoke"] = [key for key, value in attempted.items() if value]
            cell["unsupported"] = ["objective_role", "intermission", "remote_fifth_control"]
            cell["status"] = "partial" if checked.returncode == 0 and fail_count == 0 else "failed"
            if fail_count:
                findings.append({
                    "id": f"GP2-01-{mode.upper()}-{players}P-ASSERT",
                    "severity": "product-or-harness-defect",
                    "summary": f"{fail_count} lifecycle/stat assertions failed",
                    "log": str(log.relative_to(args.results))
                })
            if checked.returncode:
                findings.append({
                    "id": f"GP2-01-{mode.upper()}-{players}P-MANIFEST",
                    "severity": "evidence-invalid",
                    "summary": checked.stderr.strip()
                })
            cells.append(cell)
    status = "failed" if findings else "partial"
    report = {"schema_version": 1, "ticket": "GP2-01", "frozen_sha256": FROZEN,
              "status": status, "cells": cells, "findings": findings,
              "global_unsupported": ["remote fifth-client control",
                                     "Holocron holder/pickup assertion hook",
                                     "Jedi Master role/saber assertion hook"]}
    out = args.results / "current"
    out.mkdir(parents=True, exist_ok=True)
    (out / "matrix.json").write_text(json.dumps(report, indent=2) + "\n")
    lines = ["# GP2-01 findings", "", f"Status: **{status.upper()}**", "",
             "| Mode | Players | Result | Pass | Fail | Proven | Attempted/smoke | Unsupported |",
             "|---|---:|---|---:|---:|---|---|---|"]
    for c in cells:
        lines.append(f"| {c['mode']} | {c['players']} | {c['status']} | "
                     f"{c.get('assert_pass', 0)} | {c.get('assert_fail', 0)} | "
                     f"{', '.join(c['proven']) or 'none'} | "
                     f"{', '.join(c['attempted_smoke']) or 'none'} | "
                     f"{', '.join(c['unsupported'])} |")
    lines += ["", "## Findings", ""]
    lines += [f"- `{f['id']}`: {f['summary']}" for f in findings] or ["- No assertion failures; all cells remain partial due to unsupported objective/intermission/control coverage."]
    (out / "findings.md").write_text("\n".join(lines) + "\n")
    print(json.dumps({"status": status, "findings": len(findings), "cells": len(cells)}))
    return 1 if args.require_acceptance and findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
