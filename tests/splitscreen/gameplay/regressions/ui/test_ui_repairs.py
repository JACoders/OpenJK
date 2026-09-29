#!/usr/bin/env python3
"""Static regressions for split-screen UI ownership and setup persistence."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[5]
UI_MAIN = (ROOT / "codemp/ui/ui_main.c").read_text(encoding="utf-8")
UI_XCVAR = (ROOT / "codemp/ui/ui_xcvar.h").read_text(encoding="utf-8")
START_MENU = (
    ROOT / "assets/splitscreen/base/ui/jamp/splitscreen_start.menu"
).read_text(encoding="utf-8")


def function_body(source: str, name: str) -> str:
    match = re.search(rf"\b{name}\s*\([^)]*\)\s*\{{", source)
    if not match:
        raise AssertionError(f"function not found: {name}")
    start = match.end()
    depth = 1
    cursor = start
    while depth and cursor < len(source):
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    if depth:
        raise AssertionError(f"unterminated function: {name}")
    return source[start : cursor - 1]


class ModalOwnershipTests(unittest.TestCase):
    def test_top_menu_is_painted_only_for_active_owner(self) -> None:
        body = function_body(UI_MAIN, "UI_PaintSplitScreenIngameMenus")
        owner_guard = body.find("if ( player != activeTarget )")
        paint = body.find("Menu_Paint( ingameMenu, qtrue )")
        self.assertGreaterEqual(owner_guard, 0)
        self.assertGreater(paint, owner_guard)

    def test_stock_modal_top_bar_is_not_duplicated_into_other_panes(self) -> None:
        body = function_body(UI_MAIN, "UI_PaintSplitScreenStockMenu")
        self.assertRegex(
            body,
            r"if \( ingameMenu && player == activeTarget \) \{"
            r"[\s\S]*?Menu_Paint\( ingameMenu, qtrue \);",
        )

    def test_modal_paints_remain_viewport_scoped(self) -> None:
        for name in (
            "UI_PaintSplitScreenIngameMenus",
            "UI_PaintSplitScreenStockMenu",
            "UI_PaintSplitScreenPlayerSetup",
        ):
            body = function_body(UI_MAIN, name)
            self.assertIn("UI_PushViewportTransform(", body, name)
            self.assertIn("UI_PopViewportTransform()", body, name)


class MouseIsolationTests(unittest.TestCase):
    def test_mouse_is_confined_only_while_a_pane_scoped_menu_is_visible(self) -> None:
        confined = function_body(UI_MAIN, "UI_SplitScreenMouseConfined")
        self.assertIn("UI_SplitScreenPlayerSetupVisible()", confined)
        self.assertIn("UI_SplitScreenIngameVisible()", confined)
        self.assertIn('UI_SplitScreenModeVisible( "stock" )', confined)
        self.assertIn('UI_SplitScreenModeVisible( "controls" )', confined)

        clamp = function_body(UI_MAIN, "UI_ClampSplitScreenMouseCursor")
        self.assertIn("if ( !UI_SplitScreenMouseConfined() )", clamp)
        self.assertRegex(
            function_body(UI_MAIN, "UI_Refresh"),
            r'if \( trap->Cvar_VariableValue\( "cl_splitScreen" \) &&\s*'
            r"UI_SplitScreenMouseConfined\(\) \) \{\s*"
            r"UI_DrawSplitScreenMouseCursor\(\);\s*"
            r"\} else \{\s*"
            r"UI_DrawHandlePic\(",
        )

    def test_cursor_hotspot_can_reach_every_pixel_in_its_viewport(self) -> None:
        body = function_body(UI_MAIN, "UI_ClampSplitScreenMouseCursor")
        self.assertIn("viewportX + viewportW - 1.0f", body)
        self.assertIn("viewportY + viewportH - 1.0f", body)
        self.assertNotIn("viewportW - 40.0f", body)
        self.assertNotIn("viewportH - 40.0f", body)

    def test_cursor_artwork_is_clipped_to_the_mouse_owners_viewport(self) -> None:
        body = function_body(UI_MAIN, "UI_DrawSplitScreenMouseCursor")
        self.assertIn('UI_SplitScreenPlayerForInputDevice( "keyboard" )', body)
        self.assertIn("UI_SplitScreenSetupViewport(", body)
        self.assertIn("trap->R_DrawStretchPic(", body)

    def test_setup_paint_restores_each_players_own_hover_state(self) -> None:
        body = function_body(UI_MAIN, "UI_PaintSplitScreenPlayerSetup")
        self.assertIn(
            "playerMenu->window.flags |= ( WINDOW_FORCED | WINDOW_VISIBLE )",
            body,
        )
        load = body.find("UI_LoadSplitScreenPlayerMenuInteraction( menu, player )")
        paint = body.find("Menu_Paint( menu, qtrue )")
        save = body.find("UI_SaveSplitScreenPlayerMenuInteraction( menu, player )")
        self.assertGreaterEqual(load, 0)
        self.assertGreater(paint, load)
        self.assertGreater(save, paint)
        self.assertIn('"ui_splitScreenP%iSetupHasFocus"', body)
        self.assertIn('"ui_splitScreenP%iSetupHoverVisible"', body)

    def test_mouse_hover_is_loaded_and_saved_only_for_the_target_player(self) -> None:
        body = function_body(UI_MAIN, "UI_MouseEvent")
        setup = body.find("if ( UI_SplitScreenPlayerSetupVisible() )")
        load = body.find(
            "UI_LoadSplitScreenPlayerMenuInteraction( menu, player )", setup
        )
        hover = body.find("Menu_HandleMouseMove( menu, menuX, menuY )", load)
        save = body.find(
            "UI_SaveSplitScreenPlayerMenuInteraction( menu, player )", hover
        )
        self.assertGreaterEqual(setup, 0)
        self.assertGreater(load, setup)
        self.assertGreater(hover, load)
        self.assertGreater(save, hover)

    def test_only_transient_interaction_flags_are_player_scoped(self) -> None:
        interaction_flags = re.search(
            r"#define UI_SPLITSCREEN_INTERACTION_FLAGS\s+\\\s*"
            r"(?P<flags>\([^)]*\))",
            UI_MAIN,
        )
        self.assertIsNotNone(interaction_flags)
        flags = interaction_flags.group("flags")
        self.assertIn("WINDOW_MOUSEOVER", flags)
        self.assertIn("WINDOW_MOUSEOVERTEXT", flags)
        self.assertIn("WINDOW_HASFOCUS", flags)
        load = function_body(
            UI_MAIN, "UI_LoadSplitScreenPlayerMenuInteraction"
        )
        save = function_body(
            UI_MAIN, "UI_SaveSplitScreenPlayerMenuInteraction"
        )
        self.assertIn("WINDOW_VISIBLE", load)
        self.assertIn("itemForeColor", load)
        self.assertIn("itemBorderColor", load)
        self.assertIn("WINDOW_VISIBLE", save)


class SetupPersistenceTests(unittest.TestCase):
    def test_stock_visibility_groups_are_recomputed_for_every_player_pane(self) -> None:
        body = function_body(UI_MAIN, "UI_PrepareSplitScreenStockPlayerMenu")
        self.assertIn("UpdateForceStatus()", body)

    def test_opening_start_menu_does_not_reset_saved_choices(self) -> None:
        on_open = re.search(r"onOpen\s*\{(?P<body>[^}]*)\}", START_MENU)
        self.assertIsNotNone(on_open)
        body = on_open.group("body")
        self.assertNotIn("setcvar ui_splitScreenPlayerCount", body)
        self.assertNotIn("setcvar ui_splitScreenSessionType", body)
        self.assertIn("RefreshSplitScreenPlayerCount", body)
        self.assertIn("RefreshSplitScreenSessionType", body)
        self.assertIn("RefreshSplitScreenLayout", body)

    def test_count_buttons_use_validated_ui_script(self) -> None:
        for count in (2, 3, 4):
            self.assertIn(
                f"uiScript SplitScreenSelectPlayerCount {count}",
                START_MENU,
            )


class LayoutSelectionTests(unittest.TestCase):
    def test_two_player_layout_is_selectable_without_console_commands(self) -> None:
        self.assertIn(
            "uiScript SplitScreenSelectLayout horizontal",
            START_MENU,
        )
        self.assertIn(
            "uiScript SplitScreenSelectLayout vertical",
            START_MENU,
        )
        self.assertRegex(
            START_MENU,
            r"name\s+layouttopbottom[\s\S]*?"
            r"cvarTest ui_splitScreenPlayerCount showCvar \{ \"2\" \}",
        )
        self.assertRegex(
            START_MENU,
            r"name\s+layoutleftright[\s\S]*?"
            r"cvarTest ui_splitScreenPlayerCount showCvar \{ \"2\" \}",
        )

    def test_layout_script_validates_and_persists_the_engine_setting(self) -> None:
        self.assertIn('Q_stricmp(name, "SplitScreenSelectLayout")', UI_MAIN)
        self.assertIn('Q_stricmp( layout, "horizontal" )', UI_MAIN)
        self.assertIn('Q_stricmp( layout, "vertical" )', UI_MAIN)
        self.assertIn('trap->Cvar_Set( "cl_splitScreenLayout"', UI_MAIN)
        self.assertRegex(
            UI_XCVAR,
            r'XCVAR_DEF\( cl_splitScreenLayout,\s*"0",\s*NULL,\s*CVAR_ARCHIVE \)',
        )

    def test_setup_viewports_and_pointer_routing_follow_vertical_layout(self) -> None:
        viewport = function_body(UI_MAIN, "UI_SplitScreenSetupViewport")
        pointer = function_body(UI_MAIN, "UI_SplitScreenSetupPlayerForPoint")
        self.assertIn("UI_SplitScreenVerticalLayout()", viewport)
        self.assertIn("SCREEN_WIDTH / 2.0f", viewport)
        self.assertIn("SCREEN_HEIGHT", viewport)
        self.assertIn("UI_SplitScreenVerticalLayout()", pointer)
        self.assertIn("x >= ( SCREEN_WIDTH / 2 ) ? 2 : 1", pointer)

    def test_every_split_menu_uses_layout_aware_dividers(self) -> None:
        divider = function_body(UI_MAIN, "UI_PaintSplitScreenDividers")
        self.assertIn("UI_SplitScreenVerticalLayout()", divider)
        self.assertIn("SCREEN_WIDTH / 2.0f - 1.0f", divider)
        for name in (
            "UI_PaintSplitScreenPlayerSetup",
            "UI_PaintSplitScreenIngameMenus",
            "UI_PaintSplitScreenStockMenu",
            "UI_PaintSplitScreenControlsMenu",
        ):
            self.assertIn(
                "UI_PaintSplitScreenDividers( playerCount, divider )",
                function_body(UI_MAIN, name),
                name,
            )


if __name__ == "__main__":
    unittest.main()
