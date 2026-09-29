#!/usr/bin/env python3
import argparse

ap = argparse.ArgumentParser()
ap.add_argument("players", type=int, choices=(2, 3, 4))
p = ap.parse_args().players
classes = ["Imperial Snowtrooper", "Rebel Infantry", "Rocket Trooper", "Jedi Guardian"]
lines = [
    "set cl_splitScreen 1", f"set ui_splitScreenPlayerCount {p}",
    "set cl_splitScreenLocalCmds 0", "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1", "set ui_splitScreenP3Input controller2",
    "set ui_splitScreenP4Input controller3", "set g_gametype 7",
    "set g_siegeRespawn 0", "set con_notifytime 0", "bind e +use",
    f"set name GP2_Siege_{p}_P1", "splitnet_party_connect localhost",
    "devmap mp/siege_hoth", f"wait {2400 if p == 4 else 900}", "closemenu",
    "splitui_assert ui_splitScreenPartyState active",
    f'siegeclass "{classes[0]}"',
]
for player in range(2, p + 1):
    lines.append(f'splitnet_cmd {player} siegeclass "{classes[player-1]}"')
lines += ["wait 1600", "closemenu", "echo GP2-05:CLASS-TEAM-BEGIN",
          "splitui_assert g_gametype 7",
          "splitnet_assert_stat 1 team eq 1",
          "splitnet_assert_lifecycle 1 ALIVE"]
for player in range(2, p + 1):
    team_id = 2 if player % 2 == 0 else 1
    lines += [
        f"splitnet_assert_stat {player} team eq {team_id}",
        f"splitnet_assert_lifecycle {player} ALIVE",
    ]
lines += ["screenshot_png gp2_siege_class_team", "echo GP2-05:CLASS-TEAM-END",
          "echo GP2-05:USE-BEGIN", "splitinput_device_key keyboard e 1", "wait 30",
          "splitinput_assert_cmd 1 -999 -999 -999 288",
          "splitinput_device_key keyboard e 0", "wait 30", "echo GP2-05:USE-END",
          "echo GP2-05:DEATH-WAVE-BEGIN",
          "set g_siegeRespawn 0", "splitnet_cmd 2 kill", "wait 120",
          "splitnet_assert_lifecycle 2 DEAD", "screenshot_png gp2_siege_dead",
          "set g_siegeRespawn 1", "wait 500",
          "splitnet_assert_lifecycle 2 ALIVE",
          "screenshot_png gp2_siege_wave_respawn", "echo GP2-05:DEATH-WAVE-END",
          "echo GP2-05:CLASS-CHANGE-BEGIN",
          "set g_siegeRespawn 0",
          'splitnet_cmd 2 siegeclass "Rebel Sniper"', "wait 120",
          "splitnet_assert_lifecycle 2 DEAD",
          "splitnet_assert_stat 2 team eq 2",
          "set g_siegeRespawn 1", "wait 500",
          "splitnet_assert_lifecycle 2 ALIVE",
          "splitnet_assert_stat 2 weapon eq 6",
          "echo GP2-05:CLASS-CHANGE-END",
          "echo GP2-05:RESTART-BEGIN", "map_restart 0", "wait 700",
          "splitnet_assert_stat 1 team eq 1",
          "splitnet_assert_lifecycle 1 ALIVE"]
for player in range(2, p + 1):
    team_id = 2 if player % 2 == 0 else 1
    lines += [
        f"splitnet_assert_stat {player} team eq {team_id}",
        f"splitnet_assert_lifecycle {player} ALIVE",
    ]
lines += ["screenshot_png gp2_siege_restart", "echo GP2-05:RESTART-END",
          "echo GP2-05:NEXT-MAP-BEGIN", "map mp/siege_desert", "wait 900",
          "closemenu", "splitnet_assert_lifecycle 1 ALIVE"]
for player in range(2, p + 1):
    lines.append(f"splitnet_assert_lifecycle {player} ALIVE")
lines += ["screenshot_png gp2_siege_next_map", "echo GP2-05:NEXT-MAP-END",
          "condump gp2_siege_console.txt", "echo GP2-05:COMPLETE", "wait 20", "quit"]
print("\n".join(lines))
