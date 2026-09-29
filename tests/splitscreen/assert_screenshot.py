#!/usr/bin/env python3
"""Small dependency-free PNG oracle for split-screen gameplay captures."""

import argparse
import struct
import sys
import zlib


def read_png(path):
    with open(path, "rb") as image_file:
        data = image_file.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("not a PNG")

    offset = 8
    compressed = bytearray()
    width = height = color_type = None
    while offset < len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        chunk_type = data[offset + 4:offset + 8]
        payload = data[offset + 8:offset + 8 + length]
        offset += 12 + length
        if chunk_type == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", payload)
            if depth != 8 or color_type not in (2, 6) or interlace:
                raise ValueError("only non-interlaced 8-bit RGB/RGBA PNGs are supported")
        elif chunk_type == b"IDAT":
            compressed.extend(payload)
        elif chunk_type == b"IEND":
            break

    channels = 3 if color_type == 2 else 4
    stride = width * channels
    raw = zlib.decompress(bytes(compressed))
    rows = []
    previous = bytearray(stride)
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        cursor += 1
        scanline = bytearray(raw[cursor:cursor + stride])
        cursor += stride
        for index in range(stride):
            left = scanline[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 1:
                scanline[index] = (scanline[index] + left) & 255
            elif filter_type == 2:
                scanline[index] = (scanline[index] + above) & 255
            elif filter_type == 3:
                scanline[index] = (scanline[index] + ((left + above) // 2)) & 255
            elif filter_type == 4:
                predictor = left + above - upper_left
                pa = abs(predictor - left)
                pb = abs(predictor - above)
                pc = abs(predictor - upper_left)
                nearest = left if pa <= pb and pa <= pc else above if pb <= pc else upper_left
                scanline[index] = (scanline[index] + nearest) & 255
            elif filter_type != 0:
                raise ValueError("unknown PNG filter")
        rows.append(scanline)
        previous = scanline
    return width, height, channels, rows


def viewport_rects(width, height, players):
    half_w = width // 2
    half_h = height // 2
    if players == 2:
        return [(0, 0, width, half_h), (0, half_h, width, height)]
    if players == 3:
        return [(0, 0, width, half_h), (0, half_h, half_w, height), (half_w, half_h, width, height)]
    return [
        (0, 0, half_w, half_h),
        (half_w, 0, width, half_h),
        (0, half_h, half_w, height),
        (half_w, half_h, width, height),
    ]


def sample_viewport(rows, channels, rect, sample_size=48):
    x0, y0, x1, y1 = rect
    inset_x = max(2, (x1 - x0) // 50)
    inset_y = max(2, (y1 - y0) // 50)
    x0 += inset_x
    x1 -= inset_x
    y0 += inset_y
    y1 -= inset_y
    samples = []
    for sy in range(sample_size):
        y = y0 + ((2 * sy + 1) * (y1 - y0)) // (2 * sample_size)
        row = rows[y]
        for sx in range(sample_size):
            x = x0 + ((2 * sx + 1) * (x1 - x0)) // (2 * sample_size)
            base = x * channels
            samples.append((row[base], row[base + 1], row[base + 2]))
    return samples


def variance(samples):
    luminance = [(77 * r + 150 * g + 29 * b) / 256.0 for r, g, b in samples]
    mean = sum(luminance) / len(luminance)
    return sum((value - mean) ** 2 for value in luminance) / len(luminance)


def mean_absolute_difference(left, right):
    total = 0
    for left_pixel, right_pixel in zip(left, right):
        total += sum(abs(a - b) for a, b in zip(left_pixel, right_pixel))
    return total / (len(left) * 3)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("image")
    parser.add_argument("players", type=int, choices=(2, 3, 4))
    parser.add_argument("--min-variance", type=float, default=40.0)
    parser.add_argument("--min-difference", type=float, default=2.0)
    args = parser.parse_args()

    width, height, channels, rows = read_png(args.image)
    samples = [sample_viewport(rows, channels, rect) for rect in viewport_rects(width, height, args.players)]
    failed = False
    for index, viewport in enumerate(samples, 1):
        value = variance(viewport)
        print(f"ScreenshotOracle: player={index} variance={value:.2f}")
        if value < args.min_variance:
            print(f"ScreenshotOracle: FAIL player={index} viewport is blank or nearly uniform", file=sys.stderr)
            failed = True

    for left_index in range(len(samples)):
        for right_index in range(left_index + 1, len(samples)):
            difference = mean_absolute_difference(samples[left_index], samples[right_index])
            print(
                f"ScreenshotOracle: players={left_index + 1},{right_index + 1} difference={difference:.2f}"
            )
            if difference < args.min_difference:
                print(
                    f"ScreenshotOracle: FAIL players={left_index + 1},{right_index + 1} appear duplicated",
                    file=sys.stderr,
                )
                failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
