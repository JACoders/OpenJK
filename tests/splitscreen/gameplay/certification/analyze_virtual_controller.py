#!/usr/bin/env python3
"""Create a semantic report for the virtual input-subsystem restart probe."""

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

required = [
    "CERT-CONTROLLER:PRE-READY",
    "CERT-CONTROLLER:PRE-CHECKED",
    "CERT-CONTROLLER:POST-READY",
    "CERT-CONTROLLER:POST-CHECKED",
    "CERT-CONTROLLER:COMPLETE",
    "External gamepad bridge:",
]
expected_passes = (args.players - 1) * 4
actual_passes = text.count("SplitInputAssertCmd: PASS")
screenshot = args.screenshots / "cert_virtual_controller_restart.png"
screenshot_present = screenshot.is_file() and screenshot.stat().st_size > 0
passed = (
    all(marker in text for marker in required)
    and actual_passes >= expected_passes
    and re.search(r"SplitInputAssertCmd: FAIL|Sys_Error", text) is None
    and screenshot_present
)
report = {
    "schema_version": 1,
    "kind": "virtual_controller_restart",
    "run_id": args.run_id,
    "players": args.players,
    "frozen_sha256": args.frozen_sha,
    "claim": "stable virtual controller ownership before and after in_restart",
    "passed": passed,
    "input_assert_passes": actual_passes,
    "minimum_input_assert_passes": expected_passes,
    "screenshot_present": screenshot_present,
    "physical_hotplug_covered": False,
    "physical_hotplug_status": "BLOCKED_UNSUPPORTED",
}
args.output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, sort_keys=True))
raise SystemExit(0 if passed else 1)
