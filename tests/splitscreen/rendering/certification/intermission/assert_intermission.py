#!/usr/bin/env python3
"""Assert that every split viewport contains a substantial scoreboard overlay."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from assert_screenshot import read_png


def rects(width, height):
    half_w, half_h = width // 2, height // 2
    return [(0, 0, half_w, half_h), (half_w, 0, width, half_h),
            (0, half_h, half_w, height), (half_w, half_h, width, height)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("before", type=Path)
    parser.add_argument("intermission", type=Path)
    args = parser.parse_args()
    bw, bh, bc, before = read_png(args.before)
    iw, ih, ic, intermission = read_png(args.intermission)
    if (bw, bh, bc) != (iw, ih, ic):
        print("IntermissionOracle: FAIL capture dimensions/formats differ")
        return 1

    failed = False
    for player, (x0, y0, x1, y1) in enumerate(rects(iw, ih), 1):
        # Scoreboards occupy the central viewport; ignore edge HUD decorations.
        mx, my = (x1 - x0) // 8, (y1 - y0) // 8
        changed = bright = pixels = 0
        for y in range(y0 + my, y1 - my):
            brow, irow = before[y], intermission[y]
            for x in range(x0 + mx, x1 - mx):
                base = x * ic
                old = brow[base:base + 3]
                new = irow[base:base + 3]
                if sum(abs(a - b) for a, b in zip(old, new)) >= 36:
                    changed += 1
                if min(new) >= 175 and max(new) - min(new) <= 65:
                    bright += 1
                pixels += 1
        changed_ratio = changed / pixels
        bright_ratio = bright / pixels
        print(f"IntermissionOracle: player={player} changed={changed_ratio:.4f} bright={bright_ratio:.4f}")
        # A kill/death message can change most of a view while still leaving
        # the normal HUD active. Requiring scoreboard-scale neutral text keeps
        # that transient from being mistaken for an intermission overlay.
        if changed_ratio < 0.08 or bright_ratio < 0.02:
            print(f"IntermissionOracle: FAIL player={player} missing scoreboard/overlay")
            failed = True
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
