#!/usr/bin/env python3
"""Source regression for automatic global menus leaking across split panes."""

import re
from pathlib import Path

root = Path(__file__).resolve().parents[5]
source = (root / "codemp/client/cl_cgameapi.cpp").read_text()
start = source.index("static void CL_OpenUIMenu( int menuID ) {")
end = source.index("static void CGFX_AddLine", start)
body = source[start:end]

assert 'Cvar_VariableIntegerValue( "cl_splitScreen" )' in body
assert "menuID == UIMENU_PLAYERCONFIG" in body
assert "menuID == UIMENU_CLASSSEL" in body
assert body.index("menuID == UIMENU_PLAYERCONFIG") < body.rindex("UIVM_SetActiveMenu")
assert "CL_SplitNetSuppressAutomaticMenu" in body
assert "UIVM_SetActiveMenu" in body

input_source = (root / "codemp/client/cl_input.cpp").read_text()
assert 'CL_SplitScreenCommandPressed( player, "+scores" )' in input_source
assert 'CL_SplitScreenCommandReleased( player, "+scores" )' in input_source
assert 'CL_CGameConsoleCommandForPlayer( player, "+scores" )' in input_source
assert 'CL_CGameConsoleCommandForPlayer( player, "-scores" )' in input_source
assert 'CL_SplitScreenCommandPressed( player, "messagemode" )' in input_source
assert "Con_MessageModeForPlayer( player, qfalse )" in input_source
assert "cl_splitScreenControllerButtonReleased" in input_source

for command in ("weapnext", "weapprev", "invnext", "invprev", "forcenext", "forceprev"):
    assert f'CL_SplitScreenCommandPressed( player, "{command}" )' in input_source
    assert f'CL_CGameConsoleCommandForPlayer( player, "{command}" )' in input_source
assert "CL_SplitScreenCommandIsButtonBindable" in input_source
for command in ("sensitivity", "ui_mousePitch", "movesideaxis", "moveforwardaxis",
                "lookyawaxis", "lookpitchaxis", "cl_run", "cg_autoswitch",
                "+mlook", "voicechat"):
    assert f'"{command}"' in input_source
assert 'CL_SplitScreenCommandDown( player, "+strafe" )' in input_source
assert 'CL_SplitScreenCommandPressed( player, "centerview" )' in input_source
assert '"splitscreen_automap_toggle"' in input_source
assert '"splitscreen_thirdperson_toggle"' in input_source

cgame_source = (root / "codemp/client/cl_cgame.cpp").read_text()
helper_start = cgame_source.index("void CL_CGameConsoleCommandForPlayer")
helper_end = cgame_source.index("CL_CGameRendering", helper_start)
helper = cgame_source[helper_start:helper_end]
assert "CL_SwapSplitClientMemory" in cgame_source
assert "static clientActive_t primaryCl" not in helper
assert "static clientConnection_t primaryClc" not in helper
assert "char savedCommand[BIG_INFO_STRING]" in helper
assert "Cmd_Cmd()" in helper
assert "CGVM_ActivePlayer()" in helper
assert "CGVM_SelectPlayer( savedCGamePlayer )" in helper
assert helper.count("Cmd_TokenizeString( savedCommand )") == 3

keys_source = (root / "codemp/client/cl_keys.cpp").read_text()
assert "Key_SplitScreenKeyboardMousePlayer() != Key_GetConsolePlayer()" in keys_source
assert keys_source.count("Key_SplitScreenKeyboardMousePlayer() != Key_GetConsolePlayer()") == 2
assert "( catcher & KEYCATCH_MESSAGE ) && Key_GetConsolePlayer() == player" in keys_source
assert "cl_splitClients[player].enabled && cl_splitClients[player].state == CA_ACTIVE" in keys_source

console_source = (root / "codemp/client/cl_console.cpp").read_text()
assert console_source.count("Key_GetConsolePlayer() != player") >= 2
assert "Another local device must not steal or close the active chat field." in console_source
assert "SCR_FillRect( 0, v - 4, 320, BIGCHAR_HEIGHT + 12, chatBackdrop )" in console_source
assert '!Cvar_VariableIntegerValue( "cl_splitScreen" ) || Key_GetConsolePlayer() <= 1' in console_source
for mode in ("Con_MessageMode3_f", "Con_MessageMode4_f"):
    mode_start = console_source.index(f"void {mode}")
    mode_end = console_source.index("\n}\n", mode_start)
    mode_body = console_source[mode_start:mode_end]
    assert "Key_GetConsolePlayer() != 1" in mode_body
    assert "CGVM_SelectPlayer( 1 )" in mode_body
    assert "Key_SetConsolePlayer( 1 )" in mode_body

ui_source = (root / "codemp/ui/ui_main.c").read_text()
assert "UI_SplitScreenCommandIsButtonBindable" in ui_source
capture_start = ui_source.index("static qboolean UI_CaptureSplitScreenControllerBind")
capture_end = ui_source.index("static void UI_PaintSplitScreenControlsMenu", capture_start)
capture = ui_source[capture_start:capture_end]
assert capture.index("!UI_SplitScreenCommandIsButtonBindable") < capture.index("for ( i = 0;")
print("split modal automatic-menu scope regression: PASS")
