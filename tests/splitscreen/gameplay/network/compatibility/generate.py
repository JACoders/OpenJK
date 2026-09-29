#!/usr/bin/env python3
"""Generate deterministic GP3-02 stock compatibility discovery configs."""

import argparse

ap = argparse.ArgumentParser()
ap.add_argument("players", type=int, choices=(2, 3, 4))
ap.add_argument("pure", type=int, choices=(0, 1))
a = ap.parse_args()

lines = [
    "set developer 1",
    "set con_notifytime 0",
    "set cl_splitScreen 1",
    f"set ui_splitScreenPlayerCount {a.players}",
    "set cl_splitScreenLocalCmds 0",
    "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1",
    "set ui_splitScreenP3Input controller2",
    "set ui_splitScreenP4Input controller3",
    f"set sv_pure {a.pure}",
    "set g_gametype 0",
    "set fraglimit 0",
    "set name Compat_P1",
]
for player in range(2, a.players + 1):
    lines.append(f"set ui_splitScreenP{player}Name Compat_P{player}")
lines += ["devmap mp/ffa3", "wait 480", "closemenu"]
for player in range(2, a.players + 1):
    lines += [f"splitnet_connect {player} localhost", "wait 45"]
lines += ["wait 240", "cmd team free"]
for player in range(2, a.players + 1):
    lines.append(f"splitnet_cmd {player} team free")
lines += ["wait 360", "echo GP3-02:INITIAL-BEGIN"]
for player in range(1, a.players + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_compat_initial",
    "echo GP3-02:INITIAL-END",
    "echo GP3-02:RESTART-BEGIN",
    "map_restart 0",
    "wait 480",
]
for player in range(1, a.players + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_compat_restart",
    "echo GP3-02:RESTART-END",
    "echo GP3-02:MAP-TRANSITION-BEGIN",
    "map mp/ffa2",
    "wait 720",
]
for player in range(1, a.players + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_compat_map_transition",
    "splitnet_status",
    "echo GP3-02:MAP-TRANSITION-END",
    "condump gp3_compat_console.txt",
    "echo GP3-02:COMPLETE",
    "wait 20",
    "quit",
]
print("\n".join(lines))
