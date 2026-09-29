#!/usr/bin/env python3
"""Classify pane-local modal surfaces without averaging moving 3D views."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "tests/splitscreen"))
from assert_screenshot import read_png, viewport_rects  # noqa: E402

SURFACE_ROIS = {
    "top": (0.0, 0.0, 1.0, 0.10),
    "score": (0.08, 0.08, 0.84, 0.78),
    "console": (0.0, 0.0, 1.0, 0.62),
    "chat": (0.03, 0.0, 0.94, 0.22),
}
MIN_OWNER_COVERAGE = {"top": 0.002, "score": 0.06, "console": 0.18, "chat": 0.001}
MAX_NONOWNER_COVERAGE = {"top": 0.012, "score": 0.025, "console": 0.05, "chat": 0.008}


def modal_viewport_rects(width: int, height: int, players: int):
    # The renderer is row-major for four players: P1 TL, P2 TR, P3 BL,
    # P4 BR. Three-player mode remains P1 top, P2 BL, P3 BR.
    return viewport_rects(width, height, players)


def surface_samples(path: Path, players: int, surface: str):
    width, height, channels, rows = read_png(path)
    rx, ry, rw, rh = SURFACE_ROIS[surface]
    rects = []
    for x0, y0, x1, y1 in modal_viewport_rects(width, height, players):
        pane_w, pane_h = x1 - x0, y1 - y0
        rects.append((x0 + int(pane_w * rx), y0 + int(pane_h * ry),
                      x0 + int(pane_w * (rx + rw)), y0 + int(pane_h * (ry + rh))))
    samples = []
    for x0, y0, x1, y1 in rects:
        samples.append([
            tuple(rows[y][x * channels:x * channels + 3])
            for y in range(y0, y1) for x in range(x0, x1)
        ])
    return samples


def changed_fraction(before, after, channel_delta: int = 36) -> float:
    changed = sum(max(abs(a - b) for a, b in zip(left, right)) >= channel_delta
                  for left, right in zip(before, after))
    return changed / len(before)


def menu_signature_fraction(samples) -> float:
    orange = sum(r >= 145 and 55 <= g <= 190 and b <= 75 for r, g, b in samples)
    blue = sum(b >= 90 and b >= r * 1.35 and b >= g * 1.15 for r, g, b in samples)
    # Both the orange navigation glyphs and blue separator must be present.
    return min(orange / len(samples), blue / len(samples))


def classify_coverages(coverages, owner: int, surface: str):
    owner_value = coverages[owner - 1]
    other_max = max((value for index, value in enumerate(coverages, 1)
                     if index != owner), default=0.0)
    owner_present = owner_value >= MIN_OWNER_COVERAGE[surface]
    no_bleed = other_max <= MAX_NONOWNER_COVERAGE[surface]
    return {"owner": owner, "roi": SURFACE_ROIS[surface],
            "changed_coverage": coverages, "owner_present": owner_present,
            "nonowner_bleed": not no_bleed, "passed": owner_present and no_bleed}


def check_change(baseline: Path, target: Path, players: int, owner: int, surface: str):
    before = surface_samples(baseline, players, surface)
    after = surface_samples(target, players, surface)
    if surface == "top":
        coverages = [max(0.0, menu_signature_fraction(a) - menu_signature_fraction(b))
                     for b, a in zip(before, after)]
    else:
        coverages = [changed_fraction(a, b) for a, b in zip(before, after)]
    return classify_coverages(
        coverages, owner, surface)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("screenshots", type=Path)
    parser.add_argument("players", type=int, choices=(2, 3, 4))
    parser.add_argument("--log", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    prefix = f"modal_{args.players}p"
    log_text = args.log.read_text(errors="replace")
    cases = []
    for player in range(2, args.players + 1):
        for surface in ("top", "score"):
            path = args.screenshots / f"{prefix}_p{player}_{surface}.png"
            baseline = args.screenshots / f"{prefix}_p{player}_{surface}_pre.png"
            if path.is_file() and baseline.is_file():
                result = check_change(baseline, path, args.players, player, surface)
                if surface == "top":
                    state_passed = (
                        "SplitUIAssert: PASS cvar=ui_splitScreenMenuMode expected=top actual=top" in log_text
                        and f"SplitUIAssert: PASS cvar=ui_splitScreenInputTarget expected={player} actual={player}" in log_text)
                    result["state_passed"] = state_passed
                    result["passed"] = result["passed"] and state_passed
                result.update({"player": player, "surface": surface, "path": str(path)})
                cases.append(result)
    for surface in ("console", "chat"):
        path = args.screenshots / f"{prefix}_p2_{surface}.png"
        baseline = args.screenshots / f"{prefix}_p2_{surface}_pre.png"
        if path.is_file() and baseline.is_file():
            result = check_change(baseline, path, args.players, 2, surface)
            if surface == "console":
                state_passed = "SplitInputTrace: menu edge player=2 slot=1 pressed=1 prior=0 back=1" in log_text
                result["state_passed"] = state_passed
                result["passed"] = result["passed"] and state_passed
            result.update({"player": 2, "surface": surface, "path": str(path)})
            cases.append(result)
    report = {"schema_version": 2, "players": args.players, "cases": cases,
              "passed": bool(cases) and all(case["passed"] for case in cases)}
    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered)
    print(rendered, end="")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
