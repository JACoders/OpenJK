#!/usr/bin/env python3
"""Static/unit regressions for GP4-02 input ownership and hotplug repairs."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
KEYS = (ROOT / "codemp/client/cl_keys.cpp").read_text()
INPUT = (ROOT / "codemp/client/cl_input.cpp").read_text()
MAIN = (ROOT / "codemp/client/cl_main.cpp").read_text()
SDL = (ROOT / "shared/sdl/sdl_input.cpp").read_text()
SHARED = (ROOT / "codemp/qcommon/q_shared.h").read_text()


def function(source, name):
    match = re.search(rf"(?:static\s+)?[\w\s*]+\b{name}\s*\([^)]*\)\s*\{{", source)
    assert match, f"missing {name}"
    start = match.start()
    depth = 0
    opened = False
    for index in range(match.end() - 1, len(source)):
        if source[index] == "{":
            depth += 1
            opened = True
        elif source[index] == "}":
            depth -= 1
            if opened and depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"unterminated {name}")


# GP4-02-INPUT-01: keyboard/mouse ownership must resolve from the explicit
# assignment and must never be unconditionally cleared to player zero.
claim = function(KEYS, "Key_ClaimSplitScreenKeyboardMouse")
assert "Key_SplitScreenKeyboardMousePlayer()" in claim
assert 'Cvar_Set( "ui_splitScreenInputTarget", va( "%i", player ) )' in claim
for name in ("CL_KeyDownEvent", "CL_KeyUpEvent"):
    body = function(KEYS, name)
    assert "Key_ClaimSplitScreenKeyboardMouse" in body
    assert 'Cvar_Set( "ui_splitScreenInputTarget", "0" )' not in body
device_mouse = KEYS[KEYS.index('Cmd_AddCommand( "splitinput_device_mouse"'):]
assert 'Key_ClaimSplitScreenKeyboardMouse( "mouse" )' in device_mouse

# GP4-02-INPUT-02: polling a held controller must not synthesize a new press
# edge every frame. SDL updates current values in place and clears only owners
# with no assigned device.
poll = function(SDL, "IN_UpdateSplitScreenControllerState")
slot_loop = poll.index("for ( slot = 0;")
assert "CL_SplitScreenSetControllerAxis" not in poll[:slot_loop]
assert "CL_SplitScreenSetControllerButton" not in poll[:slot_loop]
assert "assignedPlayers[player] = qtrue" in poll
assert "if ( !assignedPlayers[player] )" in poll
assert "CL_SplitScreenClearControllerState( player )" in poll


def edge_sequence(samples):
    down = False
    edges = []
    for sample in samples:
        edges.append(bool(sample and not down))
        down = sample
    return edges


assert edge_sequence([True, True, True, False, True]) == \
       [True, False, False, False, True]

# GP4-02-INPUT-04: the external SDL bridge must update the persistent joystick
# handle that gameplay polls. A temporary SDL_JoystickOpen/Close pair accepted
# setter calls but left splitSticks[] neutral in the corrected GP5-03 run.
bridge_poll = function(SDL, "IN_PollVirtualGamepads")
assert "joystick = splitSticks[packet.controller]" in bridge_poll
assert "SDL_JoystickOpen" not in bridge_poll
assert "SDL_JoystickClose" not in bridge_poll
assert "SDL_JoystickSetVirtualAxis" in bridge_poll
assert "SDL_JoystickSetVirtualButton" in bridge_poll

# Bridge buttons are raw JOY0..JOY15 protocol indices. Physical controllers
# must retain SDL's standardized GameController mapping.
assert "useMappedGamepad = (qboolean)( splitGamepads[slot] && !IN_IsVirtualGamepadSlot( slot ) )" in poll
physical_branch = poll[poll.index("useMappedGamepad ="):]
assert "SDL_GameControllerGetButton" in physical_branch
assert "IN_SplitJoystickButton( slot, controller, i )" in physical_branch

# Virtual UI/menu state must use the same persistent raw handle as gameplay.
# Its protocol follows SDL GameController numbering: BACK=4 and START=6.
for token in (
    "qboolean useMappedGamepad",
    "IN_SplitJoystickAxis( slot, controller, i )",
    "IN_UpdateSplitScreenControllerUIEvents( player, slot, controller )",
):
    assert token in poll
for token in (
    "splitVirtualGamepadButtons[packet.controller] = buttons",
    "splitVirtualGamepadAxes[packet.controller][i]",
):
    assert token in bridge_poll
assert poll.count("if ( useMappedGamepad )") >= 2
assert "Con_ToggleConsoleForPlayer" not in poll
set_button = function(INPUT, "CL_SplitScreenSetControllerButton")
assert "cl_splitScreenControllerButtons[player][4]" in set_button
assert "cl_splitScreenControllerButtons[player][6]" in set_button
assert "cl_splitScreenControllerButtons[player][7]" not in set_button
console_chord = function(KEYS, "Key_HandleSplitScreenConsoleChord")
assert "A_JOY4" in console_chord and "A_JOY6" in console_chord
assert "A_JOY7" not in console_chord

# Reliable commands for P2-P4 must retain quoted argv boundaries. Without
# this, stock multi-word Siege classes arrive as multiple server arguments.
build_reliable = function(MAIN, "CL_SplitNetBuildReliableCommand")
assert 'strpbrk( arg, " \\t" )' in build_reliable
assert build_reliable.count('Q_strcat( command, commandSize, "\\\"" )') >= 2
reliable = function(MAIN, "CL_SplitNetReliableCommand_f")
assert "CL_SplitNetBuildReliableCommand( 2, command, sizeof( command ) )" in reliable
assert "Cmd_ArgsFromBuffer( 2, command" not in reliable
rejoin = function(MAIN, "CL_SplitNetRejoin_f")
assert "CL_SplitNetBuildReliableCommand( 2, command, sizeof( command ) )" in rejoin
assert "Cmd_ArgsFromBuffer( 2, command" not in rejoin

# GP4-02-INPUT-03: the GP3-05 value 2 is BUTTON_TALK, not a remapped attack
# bit. Preserve protocol constants and keep this diagnosed as an active-catcher
# test precondition rather than changing BUTTON_ATTACK semantics.
assert re.search(r"#define\s+BUTTON_ATTACK\s+1\b", SHARED)
assert re.search(r"#define\s+BUTTON_TALK\s+2\b", SHARED)
cmd_buttons = function(INPUT, "CL_CmdButtons")
assert "if ( Key_GetCatcher( ) )" in cmd_buttons
assert "cmd->buttons |= BUTTON_TALK" in cmd_buttons

# Explicit clear neutralizes current state, pending edges, console chord and
# respawn/attack suppression state, and validates its player index.
clear = function(INPUT, "CL_SplitScreenClearControllerState")
for token in (
    "player < 1 || player > 4",
    "cl_splitScreenControllerAxis[player][axis] = 0",
    "cl_splitScreenControllerButtons[player][button] = qfalse",
    "cl_splitScreenControllerButtonPressed[player]",
    "cl_splitScreenConsoleChordDown[player] = qfalse",
    "cl_splitScreenWasDead[player] = qfalse",
    "cl_splitScreenAttackBlockedUntilRelease[player] = qfalse",
):
    assert token in clear

print("GP4-02 source regressions: PASS")
