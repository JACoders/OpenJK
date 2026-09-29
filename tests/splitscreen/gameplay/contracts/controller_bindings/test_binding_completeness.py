#!/usr/bin/env python3
"""Completeness contract for advertised split-screen controller bindings.

The binding table is user-facing API. Every row must be classified here as an
executable button binding with a dispatch route, a settings-only row, or an
explicitly nonbindable action whose stock implementation would leak state
between players.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[5]
SOURCE = ROOT / "codemp/client/cl_input.cpp"
UI_SOURCE = ROOT / "codemp/ui/ui_main.c"

# These rows name configuration values, not actions. They are retained in the
# source table for compatibility with the controls data model, but assigning
# them to a controller button cannot execute meaningful gameplay behavior.
SETTINGS_ONLY = {
    "sensitivity",
    "ui_mousePitch",
    "movesideaxis",
    "moveforwardaxis",
    "lookyawaxis",
    "lookpitchaxis",
    "cl_run",
    "cg_autoswitch",
}

# These are genuine stock actions, but their existing implementations use
# process-global input/UI state. They must stay unassignable until a genuinely
# per-player implementation exists.
NONBINDABLE = {
    "+mlook",
    "voicechat",
}

REJECTED_BY_CAPTURE = SETTINGS_ONLY | NONBINDABLE

# Explicit inventory prevents an unreviewed table addition from silently being
# accepted merely because its spelling happens to occur elsewhere in the
# dispatch function.
EXECUTABLE = {
    "+forward", "+back", "+left", "+right", "+speed", "+moveleft",
    "+moveright", "+strafe", "+moveup", "+movedown", "+attack",
    "+altattack", "saberAttackCycle", "+use", "+button2", "invnext",
    "invprev", "+lookup", "+lookdown", "centerview",
    "weapon 1", "weapon 2", "weapon 3", "weapon 4", "weapon 5",
    "weapon 6", "weapon 7", "weapon 8", "weapon 13", "weapon 9",
    "weapon 10", "weapnext", "weapprev", "force_throw", "force_pull",
    "force_speed", "force_seeing", "+useforce", "forcenext", "forceprev",
    "force_protect", "force_absorb", "force_heal", "force_healother",
    "force_distract", "+force_grip", "+force_drain", "+force_lightning",
    "force_rage", "force_forcepowerother", "messagemode", "messagemode2",
    "automap_toggle", "+scores", "engage_duel",
    "cg_thirdperson !", "taunt", "bow", "meditate", "flourish", "gloat",
}

REQUIRED_PLAYER_LOCAL_ACTIONS = {
    "+strafe",
    "automap_toggle",
    "centerview",
    "cg_thirdperson !",
}


def extract_braced_function(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:index]
    raise AssertionError(f"unterminated function: {signature}")


def binding_table(source: str) -> list[str]:
    match = re.search(
        r"cl_splitScreenBindCommands\[\]\s*=\s*\{(?P<body>.*?)\};",
        source,
        re.DOTALL,
    )
    if not match:
        raise AssertionError("cl_splitScreenBindCommands table not found")
    return re.findall(r'"([^"]+)"', match.group("body"))


def local_string_array(function_body: str, name: str) -> set[str]:
    match = re.search(
        rf"{re.escape(name)}\[\]\s*=\s*\{{(?P<body>.*?)\}};",
        function_body,
        re.DOTALL,
    )
    if not match:
        raise AssertionError(f"local string array not found: {name}")
    return set(re.findall(r'"([^"]+)"', match.group("body")))


def routed_commands(dispatch: str, advertised: set[str]) -> set[str]:
    routed = set()
    for command in advertised:
        if re.search(
            rf"CL_SplitScreenCommand(?:Down|Pressed|Released)"
            rf"\s*\(\s*player\s*,\s*\"{re.escape(command)}\"\s*\)",
            dispatch,
        ):
            routed.add(command)

    # Direct weapon rows intentionally share one numeric dispatch loop.
    if (
        'CL_SplitScreenCommandDown( player, va( "weapon %i", weapon ) )'
        in dispatch
        and re.search(r"for\s*\(\s*weapon\s*=\s*1;\s*weapon\s*<=\s*13;", dispatch)
    ):
        routed.update(command for command in advertised if command.startswith("weapon "))
    return routed


class ControllerBindingCompleteness(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = SOURCE.read_text()
        cls.ui_source = UI_SOURCE.read_text()
        cls.advertised_list = binding_table(cls.source)
        cls.advertised = set(cls.advertised_list)
        cls.dispatch = extract_braced_function(
            cls.source,
            "static void CL_SplitScreenApplyButtonBindings",
        )
        cls.cli_filter = extract_braced_function(
            cls.source,
            "static qboolean CL_SplitScreenCommandIsButtonBindable",
        )
        cls.cli_capture = extract_braced_function(
            cls.source,
            "static void CL_SplitScreenBindController_f",
        )
        cls.ui_filter = extract_braced_function(
            cls.ui_source,
            "static qboolean UI_SplitScreenCommandIsButtonBindable",
        )
        cls.ui_capture = extract_braced_function(
            cls.ui_source,
            "static qboolean UI_CaptureSplitScreenControllerBind",
        )

    def test_table_has_no_duplicate_rows(self):
        self.assertEqual(len(self.advertised_list), len(self.advertised))

    def test_every_advertised_row_has_an_explicit_classification(self):
        classified = EXECUTABLE | SETTINGS_ONLY | NONBINDABLE
        self.assertEqual(
            classified,
            self.advertised,
            "update the explicit executable/settings-only/nonbindable "
            "inventory when the advertised binding table changes",
        )
        self.assertFalse(EXECUTABLE & SETTINGS_ONLY)
        self.assertFalse(EXECUTABLE & NONBINDABLE)
        self.assertFalse(SETTINGS_ONLY & NONBINDABLE)

    def test_every_executable_row_has_a_dispatch_route(self):
        routed = routed_commands(self.dispatch, self.advertised)
        missing = sorted(EXECUTABLE - routed)
        self.assertFalse(
            missing,
            "advertised executable controller bindings have no route in "
            f"CL_SplitScreenApplyButtonBindings: {', '.join(missing)}",
        )

    def test_all_four_player_local_actions_remain_required_and_routed(self):
        routed = routed_commands(self.dispatch, self.advertised)
        self.assertLessEqual(REQUIRED_PLAYER_LOCAL_ACTIONS, EXECUTABLE)
        self.assertLessEqual(REQUIRED_PLAYER_LOCAL_ACTIONS, routed)

    def test_rejected_rows_are_not_mistaken_for_button_dispatch(self):
        routed = routed_commands(self.dispatch, self.advertised)
        self.assertFalse(
            REJECTED_BY_CAPTURE & routed,
            "settings-only and explicitly nonbindable rows must not execute "
            "through split controller button dispatch",
        )

    def test_engine_cli_rejects_exact_policy_before_binding_eviction(self):
        self.assertEqual(
            REJECTED_BY_CAPTURE,
            local_string_array(self.cli_filter, "nonButtonCommands"),
        )
        rejection = re.search(
            r"if\s*\(\s*!CL_SplitScreenCommandIsButtonBindable"
            r"\s*\(\s*command\s*\)\s*\)\s*\{.*?\breturn\s*;",
            self.cli_capture,
            re.DOTALL,
        )
        self.assertIsNotNone(rejection)
        eviction = self.cli_capture.index("if ( button >= 0 )")
        self.assertLess(rejection.start(), eviction)

    def test_ui_capture_rejects_exact_policy_before_binding_eviction(self):
        self.assertEqual(
            REJECTED_BY_CAPTURE,
            local_string_array(self.ui_filter, "nonButtonCommands"),
        )
        rejection = re.search(
            r"if\s*\(\s*!UI_SplitScreenCommandIsButtonBindable"
            r"\s*\(\s*command\s*\)\s*\)\s*\{.*?\breturn\s+qtrue\s*;",
            self.ui_capture,
            re.DOTALL,
        )
        self.assertIsNotNone(rejection)
        eviction = self.ui_capture.index(
            "for ( i = 0; i < (int)ARRAY_LEN( ui_splitScreenControllerBindCommands ); i++ )"
        )
        self.assertLess(rejection.start(), eviction)


if __name__ == "__main__":
    unittest.main()
