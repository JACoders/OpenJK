#!/usr/bin/env python3
"""Generate deterministic GP2-03 Team FFA discovery configs."""

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("players", type=int, choices=(2, 3, 4))
args = parser.parse_args()
p = args.players

lines = [
    "set cl_splitScreen 1",
    f"set ui_splitScreenPlayerCount {p}",
    "set cl_splitScreenLocalCmds 0",
    "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1",
    "set ui_splitScreenP3Input controller2",
    "set ui_splitScreenP4Input controller3",
    "set g_gametype 6",
    "set g_friendlyFire 0",
    "set g_saberDamageScale 20",
    "set fraglimit 0",
    "set con_notifytime 0",
    f"set name GP2_Team_{p}_P1",
]
for player in range(2, p + 1):
    lines.append(f"set ui_splitScreenP{player}Name GP2_Team_{p}_P{player}")
lines += ["devmap mp/ffa3", "wait 480", "closemenu", "cmd team red"]
for player in range(2, p + 1):
    lines += [f"splitnet_connect {player} localhost", "wait 30"]
lines += ["wait 360"]
for player in range(2, p + 1):
    team = "blue" if player % 2 == 0 else "red"
    lines.append(f"splitnet_cmd {player} team {team}")
lines += ["wait 180", "closemenu", "echo GP2-03:TEAM-ASSIGN-BEGIN",
          "splitnet_assert_stat 1 team eq 1", "splitnet_assert_lifecycle 1 ALIVE"]
for player in range(2, p + 1):
    team_id = 2 if player % 2 == 0 else 1
    lines += [f"splitnet_assert_stat {player} team eq {team_id}",
              f"splitnet_assert_lifecycle {player} ALIVE"]
lines += ["screenshot_png gp2_team_assignment", "echo GP2-03:TEAM-ASSIGN-END",
          "echo GP2-03:ENEMY-KILL-BEGIN",
          "cmd give weaponnum 3", "weapon 1", "wait 180",
          "splitnet_stage_pair 1 2 48", "cmd giveother 1 health 1", "wait 40",
          "splitinput_device_key keyboard MOUSE1 1", "wait 120",
          "splitinput_assert_cmd 1 -999 -999 -999 -998",
          "splitinput_device_key keyboard MOUSE1 0", "wait 80",
          "splitnet_stage_pair 1 2 48",
          "splitinput_device_key keyboard MOUSE1 1", "wait 160",
          "splitinput_device_key keyboard MOUSE1 0", "wait 80",
          "splitnet_stage_pair 1 2 48",
          "splitinput_device_key keyboard MOUSE1 1", "wait 200",
          "splitinput_device_key keyboard MOUSE1 0", "wait 120",
          "splitnet_assert_lifecycle 2 DEAD",
          "splitnet_assert_stat 1 score ge 1",
          "screenshot_png gp2_enemy_kill", "echo GP2-03:ENEMY-KILL-END",
          "echo GP2-03:RESPAWN-BEGIN",
          "wait 120", "splitinput_device_button controller1 0 1", "wait 60",
          "splitinput_device_button controller1 0 0", "wait 420",
          "splitnet_assert_lifecycle 2 ALIVE", "echo GP2-03:RESPAWN-END",
          "echo GP2-03:SUICIDE-BEGIN", "cmd kill", "wait 120",
          "splitnet_assert_lifecycle 1 DEAD", "splitnet_assert_stat 1 deaths ge 1",
          "splitinput_device_key keyboard MOUSE1 1", "wait 60",
          "splitinput_device_key keyboard MOUSE1 0", "wait 420",
          "splitnet_assert_lifecycle 1 ALIVE", "echo GP2-03:SUICIDE-END"]
if p >= 3:
    lines += [
        "echo GP2-03:TEAMKILL-OFF-BEGIN",
        "set g_friendlyFire 0", "splitnet_stage_pair 1 3 48",
        "cmd giveother 2 health 1", "wait 40",
        "splitinput_device_key keyboard MOUSE1 1", "wait 160",
        "splitinput_device_key keyboard MOUSE1 0", "wait 80",
        "splitnet_assert_lifecycle 3 ALIVE",
        "splitnet_assert_stat 3 health eq 1", "echo GP2-03:TEAMKILL-OFF-END",
        "echo GP2-03:TEAMKILL-ON-BEGIN",
        "set g_friendlyFire 1", "wait 30", "splitnet_stage_pair 1 3 48",
        "splitinput_device_key keyboard MOUSE1 1", "wait 180",
        "splitinput_device_key keyboard MOUSE1 0", "wait 80",
        "splitnet_stage_pair 1 3 48",
        "splitinput_device_key keyboard MOUSE1 1", "wait 180",
        "splitinput_device_key keyboard MOUSE1 0", "wait 80",
        "splitnet_assert_lifecycle 3 DEAD",
        "splitnet_assert_stat 1 score le 0", "echo GP2-03:TEAMKILL-ON-END",
    ]
lines += [
    "echo GP2-03:TEAM-SWITCH-BEGIN",
    "splitnet_cmd 2 team red", "wait 1500", "splitnet_assert_stat 2 team eq 1",
    "splitnet_cmd 2 team blue", "wait 1500", "splitnet_assert_stat 2 team eq 2",
    "echo GP2-03:TEAM-SWITCH-END",
    "echo GP2-03:SPECTATE-BEGIN",
    "splitnet_cmd 2 team spectator", "wait 1500",
    "splitnet_assert_lifecycle 2 SPECTATOR",
    "splitnet_cmd 2 team blue", "wait 1500",
    "splitnet_assert_lifecycle 2 ALIVE", "echo GP2-03:SPECTATE-END",
    "echo GP2-03:RESTART-BEGIN", "map_restart 0", "wait 420",
    "splitnet_assert_lifecycle 1 ALIVE",
]
for player in range(2, p + 1):
    lines.append(f"splitnet_assert_lifecycle {player} ALIVE")
lines += ["screenshot_png gp2_map_restart", "echo GP2-03:RESTART-END",
          "echo GP2-03:SECOND-MAP-BEGIN", "devmap mp/ffa2", "wait 600",
          "cmd team red"]
for player in range(2, p + 1):
    team = "blue" if player % 2 == 0 else "red"
    lines.append(f"splitnet_cmd {player} team {team}")
lines += ["wait 240", "splitnet_assert_lifecycle 1 ALIVE"]
for player in range(2, p + 1):
    lines.append(f"splitnet_assert_lifecycle {player} ALIVE")
lines += ["screenshot_png gp2_second_map", "echo GP2-03:SECOND-MAP-END",
          "echo GP2-03:INTERMISSION-BEGIN",
          "set fraglimit 1", "map_restart 0", "wait 420",
          "cmd give weaponnum 3", "weapon 1", "wait 180",
          "splitnet_stage_pair 1 2 48", "cmd giveother 1 health 1", "wait 40",
          "splitinput_device_key keyboard MOUSE1 1", "wait 160",
          "splitinput_device_key keyboard MOUSE1 0", "wait 80",
          "splitnet_stage_pair 1 2 48",
          "splitinput_device_key keyboard MOUSE1 1", "wait 200",
          "splitinput_device_key keyboard MOUSE1 0", "wait 420"]
for player in range(1, p + 1):
    lines.append(f"splitnet_assert_lifecycle {player} INTERMISSION")
lines += ["screenshot_png gp2_intermission"]
lines += ["wait 120", "echo GP2-03:INTERMISSION-END",
          "condump gp2_team_ffa_console.txt", "echo GP2-03:COMPLETE", "wait 20", "quit"]
print("\n".join(lines))
