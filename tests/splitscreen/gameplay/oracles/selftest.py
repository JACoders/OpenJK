#!/usr/bin/env python3
"""Self-test every positive and deliberately invalid oracle fixture."""

from pathlib import Path
import json
import subprocess
import sys

HERE = Path(__file__).resolve().parent
MANIFESTS = HERE.parent / "fixtures" / "manifests"
REPORT = HERE / "selftest-report.json"

EXPECTED = {
    "good-2.json": None,
    "good-3.json": None,
    "good-4.json": None,
    "bad-duplicate.json": "IMAGE_DUPLICATE_PANE",
    "bad-blank.json": "IMAGE_BLANK",
    "bad-stale.json": "STATE_STALE_SNAPSHOT",
    "bad-cross-pane.json": "IMAGE_HUD_OWNER",
    "bad-wrong-client.json": "STATE_USERINFO",
    "bad-silent.json": "AUDIO_EVENT_TRACE",
    "bad-leaked-socket.json": "RESOURCE_SOCKET_LEAK",
    "bad-truncated.json": "STATE_TRUNCATED",
    "bad-hash.json": "ARTIFACT_HASH",
}


def main() -> int:
    rows = []
    failed = False
    for name, expected_code in EXPECTED.items():
        manifest = MANIFESTS / name
        proc = subprocess.run(
            [sys.executable, str(HERE / "oracle.py"), "--json", str(manifest)],
            text=True, capture_output=True, check=False,
        )
        payload = json.loads(proc.stdout)
        codes = [item["code"] for item in payload["findings"]]
        ok = (proc.returncode == 0 and not codes) if expected_code is None else (
            proc.returncode == 1 and expected_code in codes
        )
        rows.append({
            "manifest": name,
            "expected": expected_code or "PASS",
            "returncode": proc.returncode,
            "codes": codes,
            "result": "PASS" if ok else "FAIL",
        })
        failed |= not ok
        print(f"SELFTEST {'PASS' if ok else 'FAIL'} {name} expected={expected_code or 'PASS'} actual={codes or ['PASS']}")
    REPORT.write_text(json.dumps({"schema_version": 1, "passed": not failed, "cases": rows}, indent=2) + "\n")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
