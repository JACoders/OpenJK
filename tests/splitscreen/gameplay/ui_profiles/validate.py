#!/usr/bin/env python3
"""Static and artifact validation for GP1-01."""
import argparse, json, re, subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--results", type=Path)
    args = parser.parse_args()
    spec = json.loads((HERE / "matrix.json").read_text())
    forbidden = re.compile(spec["forbidden_profile_writes"], re.I)
    failures = []
    for players in (2, 3, 4):
        cfg = HERE / "cfg" / f"profile_{players}p.cfg"
        text = cfg.read_text()
        match = forbidden.search(text)
        if match:
            failures.append(f"{cfg.name}: forbidden direct profile write: {match.group(0)}")
        if "splitinput_device_" not in text:
            failures.append(f"{cfg.name}: no routed UI input")
    if args.results:
        for players in (2, 3, 4):
            manifests = sorted((args.results / f"{players}p").glob("*/manifest.tsv"))
            if not manifests:
                failures.append(f"{players}p: missing Phase 0 manifest")
                continue
            command = [str(ROOT / "tests/splitscreen/gameplay/run_e2e.sh"),
                       "--validate-only", str(manifests[-1])]
            result = subprocess.run(command, text=True, capture_output=True)
            if result.returncode:
                failures.append(f"{players}p: invalid Phase 0 manifest: {result.stderr.strip()}")
                continue
            fields = {}
            for line in manifests[-1].read_text().splitlines():
                parts = line.split("\t", 1)
                if len(parts) == 2:
                    fields[parts[0]] = parts[1]
            client_log = Path(fields.get("process.client.log", ""))
            if not client_log.is_file():
                failures.append(f"{players}p: missing client log")
            else:
                count = len(re.findall(r"Assert: FAIL", client_log.read_text(errors="replace")))
                if count:
                    failures.append(f"{players}p: client log contains {count} failed assertions")
    report = {"ticket": "GP1-01", "status": "failed" if failures else "passed",
              "failures": failures}
    print(json.dumps(report, indent=2))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
