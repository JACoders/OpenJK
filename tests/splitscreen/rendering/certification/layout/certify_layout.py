#!/usr/bin/env python3
"""Run pixel-level sanity checks for one layout certification capture."""

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3]))
from assert_screenshot import mean_absolute_difference, read_png, sample_viewport, variance


def rects(width, height, players, layout):
    hw, hh = width // 2, height // 2
    if players == 2:
        if layout == "vertical":
            return [(0, 0, hw, height), (hw, 0, width, height)]
        return [(0, 0, width, hh), (0, hh, width, height)]
    if players == 3:
        return [(0, 0, width, hh), (0, hh, hw, height), (hw, hh, width, height)]
    return [(0, 0, hw, hh), (hw, 0, width, hh),
            (0, hh, hw, height), (hw, hh, width, height)]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("players", type=int, choices=(2, 3, 4))
    parser.add_argument("layout", choices=("horizontal", "vertical"))
    parser.add_argument("width", type=int)
    parser.add_argument("height", type=int)
    args = parser.parse_args()

    width, height, channels, rows = read_png(args.image)
    if (width, height) != (args.width, args.height):
        print(f"FAIL dimensions expected={args.width}x{args.height} actual={width}x{height}")
        return 1

    samples = [sample_viewport(rows, channels, rect) for rect in
               rects(width, height, args.players, args.layout)]
    failed = False
    for index, pixels in enumerate(samples, 1):
        value = variance(pixels)
        print(f"player={index} variance={value:.2f}")
        if value < 40.0:
            print(f"FAIL player={index} blank/nearly uniform")
            failed = True
    for left in range(len(samples)):
        for right in range(left + 1, len(samples)):
            difference = mean_absolute_difference(samples[left], samples[right])
            print(f"players={left + 1},{right + 1} difference={difference:.2f}")
            if difference < 2.0:
                print(f"FAIL players={left + 1},{right + 1} appear duplicated")
                failed = True
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
