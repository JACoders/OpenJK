from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[5]
SOURCE = (ROOT / "codemp/ui/ui_main.c").read_text(encoding="utf-8")


def text_paint_body():
    match = re.search(
        r"void Text_Paint\s*\([^;]*?\)\s*\{(?P<body>.*?)\n\}", SOURCE, re.S
    )
    if not match:
        raise AssertionError("Text_Paint definition not found")
    return match.group("body")


class UITextPixelBudget(unittest.TestCase):
    def test_renderer_receives_measured_pixel_budget(self):
        body = text_paint_body()
        self.assertIn(
            "pixelBudget = trap->R_Font_StrLenPixels( budgetText, iFontIndex, scale )",
            body,
        )
        draw = body[body.index("trap->R_Font_DrawString") :]
        self.assertIn("pixelBudget", draw)
        self.assertNotRegex(draw, r"!.*limit\s*\?\s*-1\s*:\s*.*limit")

    def test_character_limit_selects_prefix_before_measurement(self):
        body = text_paint_body()
        limit_at = body.index("limit > 0 && limit < textLength")
        copy_at = body.index("memcpy( budgetText")
        measure_at = body.index("pixelBudget = trap->R_Font_StrLenPixels")
        self.assertLess(limit_at, copy_at)
        self.assertLess(copy_at, measure_at)

    def test_viewport_clipping_recomputes_pixel_budget(self):
        body = text_paint_body()
        loop = body.index("pixelBudget > availableWidth")
        recompute = body.index("pixelBudget = trap->R_Font_StrLenPixels", loop)
        self.assertLess(loop, recompute)


if __name__ == "__main__":
    unittest.main()
