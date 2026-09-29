"""Source contracts for the phase-0 split-screen HUD audit."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[4]
CGAME = ROOT / "codemp" / "cgame"


def source(name: str) -> str:
    return (CGAME / name).read_text(encoding="utf-8")


def function_body(text: str, name: str) -> str:
    """Return a C function body using brace balancing (sufficient for audit)."""
    match = re.search(rf"\b{name}\s*\([^;]*?\)\s*\{{", text, re.S)
    if not match:
        raise AssertionError(f"function not found: {name}")
    start = match.end() - 1
    depth = 0
    for pos in range(start, len(text)):
        if text[pos] == "{":
            depth += 1
        elif text[pos] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : pos]
    raise AssertionError(f"unterminated function: {name}")


class HudStateAudit(unittest.TestCase):
    def test_viewport_transaction_wraps_all_draw2d_returns(self):
        body = function_body(source("cg_draw.c"), "CG_DrawActive2D")
        begin = body.index("CG_Set2DViewportTransform( qtrue")
        draw = body.index("CG_Draw2D();", begin)
        end = body.index("CG_Set2DViewportTransform( qfalse", draw)
        self.assertLess(begin, draw)
        self.assertLess(draw, end)

    def test_viewport_primitive_has_software_clip(self):
        body = function_body(source("cg_drawtools.c"), "CG_DrawPicUV")
        self.assertIn("clipRight", body)
        self.assertIn("clipBottom", body)
        self.assertIn("trap->R_DrawStretchPic", body)

    def test_rotated_primitives_cannot_cross_split_seam(self):
        tools = source("cg_drawtools.c")
        for name in ("CG_DrawRotatePic", "CG_DrawRotatePic2"):
            body = function_body(tools, name)
            self.assertIn("cg_2DViewportTransformActive", body)
            self.assertIn("CG_DrawPic", body)

    def test_scoreboard_and_intermission_reach_stock_scoreboard(self):
        draw = source("cg_draw.c")
        self.assertIn("CG_DrawIntermission();", function_body(draw, "CG_Draw2D"))
        self.assertIn("CG_DrawScoreboard()", function_body(draw, "CG_DrawIntermission"))
        self.assertIn("CG_DrawOldScoreboard()", function_body(draw, "CG_DrawScoreboard"))
        scoreboard = function_body(source("cg_scoreboard.c"), "CG_DrawOldScoreboard")
        self.assertIn("CG_Text_Width", scoreboard)
        self.assertIn("CG_Text_Paint", scoreboard)

    def test_text_metrics_and_paint_share_transform(self):
        draw = source("cg_draw.c")
        width = function_body(draw, "CG_Text_Width")
        height = function_body(draw, "CG_Text_Height")
        paint = function_body(draw, "CG_Text_Paint")
        self.assertIn("CG_Transform2DWidth", width)
        self.assertIn("CG_Transform2DHeight", height)
        self.assertIn("CG_Transform2DRect", paint)
        self.assertIn("CG_Transform2DScale", paint)

    def test_spectator_uses_mismatched_measurement_and_paint(self):
        body = function_body(source("cg_draw.c"), "CG_DrawSpectator")
        self.assertIn("CG_Text_Width", body)
        self.assertIn("CG_Text_Paint", body)

    def test_death_and_falling_death_paths_are_characterized(self):
        draw2d = function_body(source("cg_draw.c"), "CG_Draw2D")
        scoreboard = function_body(source("cg_scoreboard.c"), "CG_DrawOldScoreboard")
        self.assertIn("cg.snap->ps.fallingToDeath", draw2d)
        self.assertRegex(draw2d, r"CG_FillRect\s*\(\s*0\s*,\s*0\s*,\s*SCREEN_WIDTH")
        self.assertIn("cg.predictedPlayerState.pm_type == PM_DEAD", scoreboard)

    def test_zoom_routes_charge_bar_and_resets_color(self):
        body = function_body(source("cg_draw.c"), "CG_DrawZoomMask")
        self.assertRegex(body, r"CG_DrawPicUV\s*\(\s*257\s*,\s*435")
        statements = [line.strip() for line in body.splitlines() if line.strip()]
        self.assertEqual(statements[-1], "trap->R_SetColor( NULL );")

    def test_crosshair_uses_viewport_local_coordinates(self):
        body = function_body(source("cg_draw.c"), "CG_DrawCrosshair")
        self.assertNotIn("cg.refdef.x + 0.5 * (640 - w)", body)
        self.assertNotIn("cg.refdef.y + 0.5 * (480 - h)", body)
        self.assertGreaterEqual(body.count("CG_DrawPic"), 2)
        self.assertNotIn("trap->R_DrawStretchPic", body)

    def test_lagometer_uses_transformed_primitives(self):
        body = function_body(source("cg_draw.c"), "CG_DrawLagometer")
        self.assertIn("CG_DrawPic", body)
        self.assertNotIn("trap->R_DrawStretchPic", body)

    def test_viewport_boundary_resets_renderer_color(self):
        body = function_body(source("cg_draw.c"), "CG_DrawActive2D")
        draw = body.index("CG_Draw2D();")
        self.assertIn("trap->R_SetColor( NULL );", body[:draw])
        self.assertIn("trap->R_SetColor( NULL );", body[draw:])

    def test_render_order_has_no_persistent_cgame_draw_state(self):
        body = function_body(source("cg_draw.c"), "CG_DrawActive2D")
        self.assertGreaterEqual(body.count("trap->R_SetColor( NULL );"), 2)
        self.assertIn("CG_Set2DViewportTransform( qfalse", body)

    def test_menu_owner_draw_is_dormant_in_stock_hud(self):
        draw2d = function_body(source("cg_draw.c"), "CG_Draw2D")
        owner_draw = function_body(source("cg_newDraw.c"), "CG_OwnerDraw")
        self.assertRegex(draw2d, r"if\s*\(\s*/\*cg_drawStatus\.integer\*/0\s*\)")
        self.assertIn("#if 0", owner_draw)


if __name__ == "__main__":
    unittest.main()
