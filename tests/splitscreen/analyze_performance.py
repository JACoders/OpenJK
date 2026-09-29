#!/usr/bin/env python3
"""Analyze OpenJK com_speeds and macOS time output."""

import argparse
import json
import re
from pathlib import Path
from statistics import mean


FRAME_RE = re.compile(
	r"frame:(?P<frame>\d+)\s+all:\s*(?P<all>\d+)\s+sv:\s*(?P<sv>-?\d+)\s+"
	r"ev:\s*(?P<ev>-?\d+)\s+cl:\s*(?P<cl>-?\d+)\s+gm:\s*(?P<gm>-?\d+)\s+"
	r"rf:\s*(?P<rf>-?\d+)\s+bk:\s*(?P<bk>-?\d+)"
)
RSS_RE = re.compile(r"(?P<rss>\d+)\s+maximum resident set size")


def percentile(values, fraction):
	ordered = sorted(values)
	return ordered[min(len(ordered) - 1, round((len(ordered) - 1) * fraction))]


def analyze(path):
	text = path.read_text(encoding="utf-8", errors="replace")
	frames = [{key: int(value) for key, value in match.groupdict().items()} for match in FRAME_RE.finditer(text)]
	if len(frames) < 500:
		raise ValueError(f"{path}: expected at least 500 frame samples, found {len(frames)}")

	result = {"samples": len(frames), "timings_ms": {}}
	for field in ("all", "sv", "ev", "cl", "gm", "rf", "bk"):
		values = [frame[field] for frame in frames]
		result["timings_ms"][field] = {
			"mean": round(mean(values), 3),
			"p50": percentile(values, 0.50),
			"p95": percentile(values, 0.95),
			"p99": percentile(values, 0.99),
			"max": max(values),
		}
	rss = RSS_RE.search(text)
	result["peak_rss_bytes"] = int(rss.group("rss")) if rss else None
	return result


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("runs", nargs="+", help="PLAYER_COUNT=LOG")
	parser.add_argument("--output", required=True, type=Path)
	parser.add_argument("--max-p95-ms", type=int, default=50)
	parser.add_argument("--max-frame-ms", type=int, default=250)
	parser.add_argument("--max-rss-bytes", type=int, default=3221225472)
	args = parser.parse_args()

	report = {"budgets": {
		"max_p95_ms": args.max_p95_ms,
		"max_frame_ms": args.max_frame_ms,
		"max_rss_bytes": args.max_rss_bytes,
	}, "runs": {}}
	failures = []
	for spec in args.runs:
		players, value = spec.split("=", 1)
		result = analyze(Path(value))
		report["runs"][players] = result
		all_times = result["timings_ms"]["all"]
		if all_times["p95"] > args.max_p95_ms:
			failures.append(f"{players}p p95 {all_times['p95']} ms exceeds {args.max_p95_ms} ms")
		if all_times["max"] > args.max_frame_ms:
			failures.append(f"{players}p max {all_times['max']} ms exceeds {args.max_frame_ms} ms")
		if result["peak_rss_bytes"] is None:
			failures.append(f"{players}p peak RSS was not recorded")
		elif result["peak_rss_bytes"] > args.max_rss_bytes:
			failures.append(f"{players}p peak RSS {result['peak_rss_bytes']} exceeds {args.max_rss_bytes}")

	report["passed"] = not failures
	report["failures"] = failures
	args.output.parent.mkdir(parents=True, exist_ok=True)
	args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
	for players, result in report["runs"].items():
		all_times = result["timings_ms"]["all"]
		print(f"SplitPerf: players={players} samples={result['samples']} mean={all_times['mean']}ms "
			f"p95={all_times['p95']}ms max={all_times['max']}ms rss={result['peak_rss_bytes']}")
	for failure in failures:
		print(f"SplitPerf: FAIL {failure}")
	return 1 if failures else 0


if __name__ == "__main__":
	raise SystemExit(main())
