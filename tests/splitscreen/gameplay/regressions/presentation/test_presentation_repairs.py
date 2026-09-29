#!/usr/bin/env python3
"""Source regressions for split-screen presentation ownership."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[5]
CGAME = ROOT / "codemp/cgame"
RENDERER = ROOT / "codemp/rd-vanilla"


def source(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def function_body(text: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^;]*?\)\s*\{{", text, re.S)
    if not match:
        raise AssertionError(f"function not found: {name}")
    start = match.end()
    depth = 1
    for cursor in range(start, len(text)):
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        if depth == 0:
            return text[start:cursor]
    raise AssertionError(f"unterminated function: {name}")


class HudIsolationTests(unittest.TestCase):
    def test_axis_aligned_hud_primitives_use_clipped_draw_path(self) -> None:
        tools = source(CGAME / "cg_drawtools.c")
        for name in ("CG_DrawPic", "CG_FillRect", "CG_DrawChar"):
            self.assertIn("CG_DrawPicUV(", function_body(tools, name), name)
        char = function_body(tools, "CG_DrawChar")
        self.assertNotIn("trap->R_DrawStretchPic", char)

    def test_clipped_draw_path_clips_geometry_and_uvs(self) -> None:
        body = function_body(source(CGAME / "cg_drawtools.c"), "CG_DrawPicUV")
        for marker in (
            "clipRight",
            "clipBottom",
            "s1 +=",
            "t1 +=",
            "s2 -=",
            "t2 -=",
        ):
            self.assertIn(marker, body)

    def test_hud_transaction_resets_color_and_transform(self) -> None:
        body = function_body(source(CGAME / "cg_draw.c"), "CG_DrawActive2D")
        draw = body.index("CG_Draw2D();")
        self.assertIn("trap->R_SetColor( NULL );", body[:draw])
        self.assertIn("trap->R_SetColor( NULL );", body[draw:])
        self.assertLess(
            draw,
            body.index("CG_Set2DViewportTransform( qfalse", draw),
        )


class SceneOwnershipTests(unittest.TestCase):
    def test_cgame_partitions_cover_odd_frame_edges(self) -> None:
        body = function_body(source(CGAME / "cg_view.c"), "CG_ApplySplitScreenRect")
        self.assertNotRegex(body, r"cg\.refdef\.(?:width|height)\s*&=\s*~1")
        self.assertIn("cgs.glconfig.vidWidth - halfWidth", body)
        self.assertIn("cgs.glconfig.vidHeight - halfHeight", body)

    def test_renderer_does_not_split_partitioned_refdef_again(self) -> None:
        body = function_body(source(RENDERER / "tr_scene.cpp"), "RE_RenderScene")
        self.assertNotIn("R_RenderSplitScreenViews", body)
        self.assertEqual(body.count("R_RenderView( &parms )"), 1)

    def test_renderer_drops_invalid_model_without_debug_only_assert(self) -> None:
        body = function_body(
            source(RENDERER / "tr_scene.cpp"),
            "RE_AddRefEntityToScene",
        )
        self.assertIn("dropping RT_MODEL", body)
        self.assertNotRegex(body, r"assert\s*\(\s*ent->hModel")


if __name__ == "__main__":
    unittest.main()
