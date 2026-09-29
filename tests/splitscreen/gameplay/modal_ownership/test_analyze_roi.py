#!/usr/bin/env python3
import unittest
import analyze
from pathlib import Path


class ModalRoiOracleTest(unittest.TestCase):
    def test_owner_only_passes(self):
        self.assertTrue(analyze.classify_coverages([.002, .20, .003, .001], 2, "console")["passed"])

    def test_nonowner_bleed_fails(self):
        result = analyze.classify_coverages([.002, .20, .09, .001], 2, "console")
        self.assertFalse(result["passed"])
        self.assertTrue(result["nonowner_bleed"])

    def test_missing_owner_surface_fails(self):
        self.assertFalse(analyze.classify_coverages([.0001, .0003, .0001, .0001], 2, "chat")["passed"])

    def test_small_camera_noise_is_ignored(self):
        self.assertEqual(0.0, analyze.changed_fraction([(80, 90, 100)] * 100,
                                                       [(88, 82, 109)] * 100))

    def test_modal_pixels_are_detected(self):
        before = [(80, 90, 100)] * 100
        self.assertEqual(.25, analyze.changed_fraction(before, before[:75] + [(10, 10, 10)] * 25))

    def test_four_player_modal_order_and_chat_local_top(self):
        self.assertEqual((320, 0, 640, 240), analyze.modal_viewport_rects(640, 480, 4)[1])
        self.assertEqual((.03, 0.0, .94, .22), analyze.SURFACE_ROIS["chat"])

    def test_top_signature_requires_orange_and_blue(self):
        samples = [(200, 120, 20)] * 20 + [(20, 30, 150)] * 20 + [(80, 80, 80)] * 60
        self.assertGreater(analyze.menu_signature_fraction(samples), 0.0)
        self.assertEqual(0.0, analyze.menu_signature_fraction([(200, 120, 20)] * 100))

    def test_retained_r4_chat_owner_roi_starts_at_player_two_top(self):
        root = Path(__file__).resolve().parents[4]
        fixture = root / (
            "tests/splitscreen/gameplay/results/modal_ownership/runs/modal-4p/"
            "20260723T164831Z-24779-20948/screenshots/modal_4p_p2_chat.png")
        if not fixture.is_file():
            self.skipTest("retained r4 fixture not present")
        width, height, _, _ = analyze.read_png(fixture)
        p2 = analyze.modal_viewport_rects(width, height, 4)[1]
        self.assertEqual(0, p2[1])
        self.assertEqual(0.0, analyze.SURFACE_ROIS["chat"][1])


if __name__ == "__main__":
    unittest.main()
