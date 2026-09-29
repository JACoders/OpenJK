#!/usr/bin/env python3
"""Detect menu pixels escaping an allowed viewport by comparison to a clean frame."""

import argparse
import struct
import sys
import zlib
from pathlib import Path


def read_png(path):
    data = Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")
    offset, compressed = 8, bytearray()
    width = height = color_type = None
    while offset < len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        offset += length + 12
        if kind == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
            if depth != 8 or color_type not in (2, 6) or interlace:
                raise ValueError("only non-interlaced 8-bit RGB/RGBA PNGs are supported")
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break
    channels = 3 if color_type == 2 else 4
    stride, cursor = width * channels, 0
    raw, rows, previous = zlib.decompress(bytes(compressed)), [], bytearray(width * channels)
    for _ in range(height):
        filter_type, cursor = raw[cursor], cursor + 1
        row = bytearray(raw[cursor:cursor + stride])
        cursor += stride
        for i in range(stride):
            left = row[i - channels] if i >= channels else 0
            above = previous[i]
            upper_left = previous[i - channels] if i >= channels else 0
            if filter_type == 1:
                row[i] = (row[i] + left) & 255
            elif filter_type == 2:
                row[i] = (row[i] + above) & 255
            elif filter_type == 3:
                row[i] = (row[i] + ((left + above) // 2)) & 255
            elif filter_type == 4:
                predictor = left + above - upper_left
                distances = (abs(predictor - left), abs(predictor - above), abs(predictor - upper_left))
                row[i] = (row[i] + (left, above, upper_left)[distances.index(min(distances))]) & 255
            elif filter_type != 0:
                raise ValueError(f"unsupported PNG filter {filter_type}")
        rows.append(row)
        previous = row
    return width, height, channels, rows


def parse_rect(value):
    values = tuple(int(part) for part in value.split(","))
    if len(values) != 4:
        raise argparse.ArgumentTypeError("rect must be x0,y0,x1,y1")
    return values


def outside(rect, x, y):
    x0, y0, x1, y1 = rect
    return x < x0 or x >= x1 or y < y0 or y >= y1


def changed_pixels(reference, capture, allowed, threshold):
    rw, rh, rc, rr = reference
    cw, ch, cc, cr = capture
    if (rw, rh, rc) != (cw, ch, cc):
        raise ValueError("reference and capture dimensions/formats differ")
    leaked = 0
    worst = 0
    for y in range(rh):
        for x in range(rw):
            if not outside(allowed, x, y):
                continue
            offset = x * rc
            delta = max(abs(rr[y][offset + channel] - cr[y][offset + channel]) for channel in range(3))
            worst = max(worst, delta)
            leaked += delta > threshold
    return leaked, worst


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", help="clean frame captured immediately before opening the menu")
    parser.add_argument("capture", help="same scene with the menu visible")
    parser.add_argument("--allowed", required=True, type=parse_rect, help="half-open pixel rect x0,y0,x1,y1")
    parser.add_argument("--channel-tolerance", type=int, default=3)
    parser.add_argument("--max-leaked-pixels", type=int, default=0)
    args = parser.parse_args(argv)
    leaked, worst = changed_pixels(read_png(args.reference), read_png(args.capture), args.allowed, args.channel_tolerance)
    print(f"MenuClipOracle: leaked_pixels={leaked} worst_channel_delta={worst} allowed={args.allowed}")
    if leaked > args.max_leaked_pixels:
        print(f"MenuClipOracle: FAIL leaked_pixels={leaked} budget={args.max_leaked_pixels}", file=sys.stderr)
        return 1
    print("MenuClipOracle: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
