#!/usr/bin/env python3
import struct
import tempfile
import unittest
import zlib
from pathlib import Path

import menu_clip_oracle


def write_rgb_png(path, width, height, pixels):
    raw = b"".join(b"\0" + bytes(pixels[y]) for y in range(height))
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw))
        + chunk(b"IEND", b"")
    )


class MenuClipOracleTests(unittest.TestCase):
    def fixture(self, leak=False):
        width, height = 16, 12
        reference = [bytearray([12, 18, 24] * width) for _ in range(height)]
        capture = [bytearray(row) for row in reference]
        for y in range(1, 5):
            for x in range(2, 14):
                capture[y][x * 3:x * 3 + 3] = b"\xc0\x80\x20"
        if leak:  # Preserved defect shape: a glyph crosses the horizontal viewport seam.
            capture[6][7 * 3:7 * 3 + 3] = b"\xff\xff\xff"
        return width, height, reference, capture

    def run_fixture(self, leak):
        with tempfile.TemporaryDirectory() as directory:
            width, height, reference, capture = self.fixture(leak)
            reference_path = Path(directory) / "clean.png"
            capture_path = Path(directory) / "menu.png"
            write_rgb_png(reference_path, width, height, reference)
            write_rgb_png(capture_path, width, height, capture)
            return menu_clip_oracle.main([
                str(reference_path), str(capture_path), "--allowed", "0,0,16,6"
            ])

    def test_clean_menu_is_accepted(self):
        self.assertEqual(self.run_fixture(False), 0)

    def test_preserved_seam_leak_is_rejected(self):
        self.assertEqual(self.run_fixture(True), 1)


if __name__ == "__main__":
    unittest.main()
