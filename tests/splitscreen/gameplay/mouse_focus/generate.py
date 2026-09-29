#!/usr/bin/env python3
"""Generate deterministic GP1-02 discovery configs for 2/3/4 players."""

from pathlib import Path

HERE = Path(__file__).resolve().parent


def emit(players: int) -> None:
    max_x = 319 if players == 4 else 639
    max_y = 239
    vertical_max_x = 319 if players in (2, 4) else 639
    vertical_max_y = 479 if players == 2 else 239
    assignments = [
        "set ui_splitScreenP1Input keyboard",
        "set ui_splitScreenP2Input controller1",
        "set ui_splitScreenP3Input controller2",
        "set ui_splitScreenP4Input controller3",
    ]
    lines = [
        "set developer 1",
        "set con_notifytime 0",
        "set cl_splitScreen 1",
        "set cl_splitScreenLayout 0",
        f"set ui_splitScreenPlayerCount {players}",
        "set cl_splitScreenLocalCmds 0",
        "set ui_splitScreenTraceInput 1",
        *assignments,
        "set ui_splitScreenPartyState configuring",
        "splitnet_party_connect localhost",
        "devmap mp/ffa3",
        "wait 720",
        "closemenu",
        "cmd team free",
    ]
    for player in range(2, players + 1):
        lines.append(f"splitnet_cmd {player} team free")
    lines += [
        "wait 180",
        "splitscreen_setup 1",
        "wait 60",
        "echo GP1-MOUSE:SURFACE-SETUP",
        f"screenshot_png gp1_mouse_{players}p_setup",
        "echo GP1-MOUSE:HOVER-ISOLATION",
        "splitinput_device_mouse -10000 -10000",
        "splitinput_device_mouse 200 220",
        "wait 2",
        "splitui_assert ui_splitScreenP1SetupHasFocus 1",
        "splitui_assert ui_splitScreenP2SetupHasFocus 0",
        "splitui_assert ui_splitScreenP1SetupHoverVisible 1",
        "splitui_assert ui_splitScreenP2SetupHoverVisible 0",
    ]

    points = [
        ("top_left", -10000, -10000, 0, 0),
        ("top_right", 10000, -10000, max_x, 0),
        ("bottom_left", -10000, 10000, 0, max_y),
        ("bottom_right", 10000, 10000, max_x, max_y),
        ("left_edge", -10000, 100 - max_y, 0, 100),
        ("right_edge", 10000, 0, max_x, 100),
        ("top_edge", -100, -10000, max(0, max_x - 100), 0),
        ("bottom_edge", 0, 10000, max(0, max_x - 100), max_y),
    ]
    for name, dx, dy, x, y in points:
        lines += [
            f"echo GP1-MOUSE:POINT:{name}",
            f"splitinput_device_mouse {dx} {dy}",
            "wait 2",
            "splitui_assert ui_splitScreenMouseOwner 1",
            f"splitui_assert ui_splitScreenMouseCursorX {x}",
            f"splitui_assert ui_splitScreenMouseCursorY {y}",
        ]

    lines += [
        "echo GP1-MOUSE:HUGE-REPEATED",
        "splitinput_device_mouse -10000 -10000",
        "splitinput_device_mouse 1000000 1000000",
        "splitinput_device_mouse 1000000 1000000",
        "splitinput_device_mouse 1000000 1000000",
        f"splitui_assert ui_splitScreenMouseCursorX {max_x}",
        f"splitui_assert ui_splitScreenMouseCursorY {max_y}",
        "echo GP1-MOUSE:BOUNDARY-DRAG",
        "splitinput_device_key keyboard MOUSE1 1",
        "splitinput_device_mouse 1000000 1000000",
        "splitinput_device_mouse -1000000 -1000000",
        "splitinput_device_key keyboard MOUSE1 0",
        "splitui_assert ui_splitScreenMouseOwner 1",
        "splitui_assert ui_splitScreenMouseCursorX 0",
        "splitui_assert ui_splitScreenMouseCursorY 0",
    ]
    for player in range(2, players + 1):
        lines.append(f"splitinput_assert_model {player} kyle/default")

    lines += [
        "echo GP1-MOUSE:LAYOUT-HORIZONTAL",
        "set cl_splitScreenLayout 0",
        "splitinput_device_mouse 1000000 1000000",
        f"splitui_assert ui_splitScreenMouseCursorX {max_x}",
        f"splitui_assert ui_splitScreenMouseCursorY {max_y}",
        "echo GP1-MOUSE:LAYOUT-VERTICAL",
        "set cl_splitScreenLayout 1",
        "splitinput_device_mouse -1000000 -1000000",
        "splitui_assert ui_splitScreenMouseCursorX 0",
        "splitui_assert ui_splitScreenMouseCursorY 0",
        "echo GP1-MOUSE:SURFACE-FORCE",
        "splitinput_device_mouse 332 191",
        "splitinput_device_key keyboard MOUSE1 1",
        "splitinput_device_key keyboard MOUSE1 0",
        "wait 30",
        "splitui_assert ui_splitScreenInputTarget 1",
        f"screenshot_png gp1_mouse_{players}p_force",
        "splitinput_device_key keyboard ESCAPE 1",
        "splitinput_device_key keyboard ESCAPE 0",
        "wait 20",
        "echo GP1-MOUSE:SURFACE-CONTROLS",
        "splitscreen_topmenu 1",
        "wait 30",
        "set ui_splitScreenMenuMode controls",
        "set ui_splitScreenInputTarget 1",
        "splitinput_device_mouse 1000000 1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"screenshot_png gp1_mouse_{players}p_controls",
        "echo GP1-MOUSE:SURFACE-VIRTUAL-KEYBOARD",
        "splitscreen_keyboard 1",
        "wait 30",
        "splitinput_device_mouse -1000000 -1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"screenshot_png gp1_mouse_{players}p_keyboard",
        "echo GP1-MOUSE:SURFACE-CONSOLE",
        "toggleconsole 1",
        "wait 30",
        "splitinput_device_mouse 1000000 1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"screenshot_png gp1_mouse_{players}p_console",
        "toggleconsole 1",
        "echo GP1-MOUSE:SURFACE-SCOREBOARD",
        "+scores",
        "wait 20",
        "splitinput_device_mouse -1000000 -1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"screenshot_png gp1_mouse_{players}p_scoreboard",
        "-scores",
        "echo GP1-MOUSE:SURFACE-CHAT",
        "messagemode",
        "wait 20",
        "splitinput_device_mouse 1000000 1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"screenshot_png gp1_mouse_{players}p_chat",
        "splitinput_device_key keyboard ESCAPE 1",
        "splitinput_device_key keyboard ESCAPE 0",
        "echo GP1-MOUSE:RESIZE",
        "set r_mode 4",
        "vid_restart",
        "wait 360",
        "splitscreen_setup 1",
        "wait 60",
        "splitinput_device_mouse -1000000 -1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        "splitui_assert ui_splitScreenMouseCursorX 0",
        "splitui_assert ui_splitScreenMouseCursorY 0",
        "echo GP1-MOUSE:RENDERER-RESTART",
        "vid_restart",
        "wait 360",
        "splitscreen_setup 1",
        "wait 60",
        "splitinput_device_mouse 1000000 1000000",
        "splitui_assert ui_splitScreenMouseOwner 1",
        f"splitui_assert ui_splitScreenMouseCursorX {vertical_max_x}",
        f"splitui_assert ui_splitScreenMouseCursorY {vertical_max_y}",
        "echo GP1-MOUSE:COUNT-CHANGE",
        "set ui_splitScreenPlayerCount 4",
        "wait 30",
        "splitinput_device_mouse 1000000 1000000",
        "splitui_assert ui_splitScreenMouseCursorX 319",
        "splitui_assert ui_splitScreenMouseCursorY 239",
        f"set ui_splitScreenPlayerCount {players}",
        "echo GP1-MOUSE:REASSIGNMENT-POLICY",
        "ui_splitScreenP2Input",
        "splitui_assert ui_splitScreenP1Input keyboard",
        "splitui_assert ui_splitScreenP2Input controller1",
        "echo GP1-MOUSE:COMPLETE",
        f"screenshot_png gp1_mouse_{players}p_final",
        f"condump gp1_mouse_{players}p_console.txt",
        "wait 20",
        "quit",
    ]
    (HERE / f"mouse_focus_{players}p.cfg").write_text("\n".join(lines) + "\n")


for count in (2, 3, 4):
    emit(count)
