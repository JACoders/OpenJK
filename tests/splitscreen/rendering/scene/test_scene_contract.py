from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[4]
CG_VIEW = ROOT / "codemp/cgame/cg_view.c"
TR_SCENE = ROOT / "codemp/rd-vanilla/tr_scene.cpp"
TR_BACKEND = ROOT / "codemp/rd-vanilla/tr_backend.cpp"


def expected_rects(width, height, players, vertical):
    """Canonical top-origin, half-open split-screen rectangles."""
    half_width = width // 2
    half_height = height // 2
    right_width = width - half_width
    bottom_height = height - half_height

    if players == 2:
        if vertical:
            return [(0, 0, half_width, height),
                    (half_width, 0, right_width, height)]
        return [(0, 0, width, half_height),
                (0, half_height, width, bottom_height)]
    if players == 3:
        return [(0, 0, width, half_height),
                (0, half_height, half_width, bottom_height),
                (half_width, half_height, right_width, bottom_height)]
    return [(0, 0, half_width, half_height),
            (half_width, 0, right_width, half_height),
            (0, half_height, half_width, bottom_height),
            (half_width, half_height, right_width, bottom_height)]


class SceneGeometryContract(unittest.TestCase):
    def test_all_layouts_cover_every_pixel_exactly_once(self):
        # Odd dimensions expose both outer-edge and internal-seam truncation.
        for width, height in ((640, 480), (641, 480), (640, 481), (641, 481),
                              (1279, 719), (2240, 1260)):
            for players in (2, 3, 4):
                for vertical in (False, True):
                    with self.subTest(size=(width, height), players=players,
                                      vertical=vertical):
                        coverage = bytearray(width * height)
                        rects = expected_rects(width, height, players, vertical)
                        self.assertEqual(players, len(rects))
                        for x, y, w, h in rects:
                            self.assertGreater(w, 0)
                            self.assertGreater(h, 0)
                            self.assertGreaterEqual(x, 0)
                            self.assertGreaterEqual(y, 0)
                            self.assertLessEqual(x + w, width)
                            self.assertLessEqual(y + h, height)
                            for row in range(y, y + h):
                                start = row * width + x
                                for pixel in range(start, start + w):
                                    coverage[pixel] += 1
                        self.assertNotIn(0, coverage, "stale/uncovered framebuffer pixel")
                        self.assertNotIn(2, coverage, "scene coverage escaped/overlapped")

    def test_cgame_does_not_truncate_partition_dimensions(self):
        source = CG_VIEW.read_text(encoding="utf-8")
        function = source[source.index("static void CG_ApplySplitScreenRect"):]
        function = function[:function.index("\n}\n") + 3]
        self.assertNotRegex(
            function,
            r"cg\.refdef\.(?:width|height)\s*&=\s*~1",
            "rounding each partition down leaves stale rows/columns",
        )

    def test_renderer_does_not_split_an_already_partitioned_refdef(self):
        source = TR_SCENE.read_text(encoding="utf-8")
        render_scene = source[source.index("void RE_RenderScene") :]
        self.assertNotRegex(
            render_scene,
            r"R_RenderSplitScreenViews\s*\(",
            "cgame already partitions one refdef per player; renderer must render it once",
        )


class ClearAndStateContract(unittest.TestCase):
    def test_scene_clear_only_advances_submission_boundaries(self):
        source = TR_SCENE.read_text(encoding="utf-8")
        clear_scene = source[source.index("void RE_ClearScene") :]
        clear_scene = clear_scene[:clear_scene.index("\n}\n") + 3]
        self.assertNotRegex(clear_scene, r"qgl(?:Clear|Viewport|Scissor)\s*\(")
        for boundary in ("r_firstSceneDlight", "r_firstSceneEntity",
                         "r_firstScenePoly", "r_firstSceneMiniEntity"):
            self.assertRegex(clear_scene, rf"{boundary}\s*=")

    def test_viewport_and_scissor_precede_view_clear(self):
        source = TR_BACKEND.read_text(encoding="utf-8")
        begin = source[source.index("void RB_BeginDrawingView") :]
        begin = begin[:begin.index("if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )")]
        self.assertLess(begin.index("SetViewportAndScissor();"),
                        begin.index("qglClear( clearBits )"))

    def test_generic_2d_explicitly_restores_full_frame_clip(self):
        source = TR_BACKEND.read_text(encoding="utf-8")
        set_2d = source[source.index("void\tRB_SetGL2D") :]
        set_2d = set_2d[:set_2d.index("\n}\n") + 3]
        self.assertRegex(set_2d, r"qglViewport\(\s*0,\s*0,\s*glConfig\.vidWidth,\s*glConfig\.vidHeight\s*\)")
        self.assertRegex(set_2d, r"qglScissor\(\s*0,\s*0,\s*glConfig\.vidWidth,\s*glConfig\.vidHeight\s*\)")


if __name__ == "__main__":
    unittest.main()
