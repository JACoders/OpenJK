#!/usr/bin/env python3
"""Stable source-level routing checks for split-screen menu transforms."""
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
SOURCE = (ROOT / "codemp/ui/ui_main.c").read_text()


class UITransformContractTests(unittest.TestCase):
    def test_text_uses_the_viewport_transform(self):
        body = re.search(r"void Text_Paint\(.*?\n\}\n", SOURCE, re.S).group(0)
        self.assertIn("UI_TransformRect( &x, &y, &w, &h );", body)
        self.assertIn("UI_TransformScale( scale )", body)

    def test_stock_menu_is_painted_inside_viewport_scope(self):
        body = re.search(r"static void UI_PaintSplitScreenStockMenu\(.*?\n\}\n", SOURCE, re.S).group(0)
        self.assertIn("UI_PushViewportTransform(", body)
        self.assertIn("Menu_Paint( menu, qtrue );", body)
        self.assertIn("UI_PopViewportTransform();", body)

    def test_picture_clipping_preserves_texture_coordinates(self):
        body = re.search(r"void UI_TransformPicRect\(.*?\n\}\n", SOURCE, re.S).group(0)
        self.assertIn("*s1 +=", body)
        self.assertIn("*t1 +=", body)
        self.assertIn("*s2 -=", body)
        self.assertIn("*t2 -=", body)

    def test_text_is_bounded_by_active_viewport(self):
        body = re.search(r"void Text_Paint\(.*?\n\}\n", SOURCE, re.S).group(0)
        self.assertIn("availableWidth", body)
        self.assertIn("ui_viewportTransformW", body)
        self.assertIn("pixelBudget > availableWidth", body)
        self.assertIn(
            "pixelBudget = trap->R_Font_StrLenPixels( budgetText, iFontIndex, scale );",
            body,
        )
        draw = body[body.index("trap->R_Font_DrawString") :]
        self.assertIn("pixelBudget", draw)


if __name__ == "__main__":
    unittest.main()
