#!/usr/bin/env python3
"""Generate GP2-02 Duel and Power Duel discovery probes."""

from pathlib import Path

HERE = Path(__file__).resolve().parent


def common(players: int, gametype: int) -> list[str]:
    lines = [
        "set developer 1", "set con_notifytime 0", "set cl_splitScreen 1",
        f"set ui_splitScreenPlayerCount {players}", "set cl_splitScreenLocalCmds 0",
        "set ui_splitScreenP1Input keyboard", "set ui_splitScreenP2Input controller1",
        "set ui_splitScreenP3Input controller2", "set ui_splitScreenP4Input controller3",
        f"set g_gametype {gametype}", "set duel_fraglimit 1",
        "set g_saberDamageScale 20", "set ui_splitScreenPartyState configuring",
        "splitnet_party_connect localhost", "devmap mp/duel1",
        f"wait {2400 if players == 4 else 900}", "closemenu",
        "splitui_assert ui_splitScreenPartyState active", "cmd team free",
    ]
    for player in range(2, players + 1):
        lines.append(f"splitnet_cmd {player} team free")
    lines += ["wait 600", f"splitui_assert g_gametype {gametype}",
              "splitnet_status", "echo GP2-DUEL:INITIAL", "wait 30"]
    return lines


def finish(lines: list[str], name: str) -> None:
    lines += [f"screenshot_png {name}_final", "splitnet_status",
              "echo GP2-DUEL:COMPLETE", f"condump {name}_console.txt",
              "wait 20", "quit"]


for players in (2, 3, 4):
    lines = common(players, 3)
    lines += [
        "echo GP2-DUEL:DUEL-CAPACITY",
        "splitnet_assert_lifecycle 1 ALIVE",
        "splitnet_assert_lifecycle 2 ALIVE",
    ]
    for player in range(3, players + 1):
        lines.append(f"splitnet_assert_lifecycle {player} SPECTATOR")
    lines += [
        f"screenshot_png gp2_duel_{players}p_initial",
        "echo GP2-DUEL:ROUND-P1-WIN",
        "splitnet_stage_pair 1 2 44", "cmd giveother 1 health 1", "wait 60",
        "splitinput_device_key keyboard MOUSE1 1", "wait 180",
        "splitinput_device_key keyboard MOUSE1 0", "wait 300",
        "splitnet_assert_stat 1 score ge 1",
        "splitnet_status", f"screenshot_png gp2_duel_{players}p_round1",
        "echo GP2-DUEL:TURNOVER", "wait 420", "splitnet_status",
    ]
    if players >= 3:
        lines += [
            "echo GP2-DUEL:VOLUNTARY-SPECTATE-REJOIN",
            "splitnet_cmd 3 team spectator", "wait 90",
            "splitnet_assert_lifecycle 3 SPECTATOR",
            "splitnet_cmd 3 team free", "wait 180", "splitnet_status",
            "echo GP2-DUEL:DISCONNECT-REJOIN",
            "splitnet_disconnect 3", "wait 90", "splitnet_connect 3 localhost",
            "wait 300", "splitnet_cmd 3 team free", "wait 120", "splitnet_status",
        ]
    finish(lines, f"gp2_duel_{players}p")
    (HERE / f"duel_{players}p.cfg").write_text("\n".join(lines) + "\n")

for players in (2, 3, 4):
    lines = common(players, 4)
    lines += ["echo GP2-DUEL:POWER-CAPACITY"]
    if players >= 1:
        lines += ["cmd duelteam single"]
    if players >= 2:
        lines += ["splitnet_cmd 2 duelteam double"]
    if players >= 3:
        lines += ["splitnet_cmd 3 duelteam double"]
    if players >= 4:
        lines += ["splitnet_cmd 4 duelteam single"]
    lines += ["wait 600"]
    if players == 4:
        for player in range(1, 5):
            lines.append(f"splitnet_assert_lifecycle {player} INTERMISSION")
    lines += ["splitnet_status", f"screenshot_png gp2_powerduel_{players}p_roles"]
    if players == 3:
        lines += [
            "echo GP2-DUEL:POWER-ROUND-SINGLE-WIN",
            "splitnet_stage_pair 1 2 44", "cmd giveother 1 health 1",
            "splitnet_cmd 3 kill", "wait 60",
            "splitinput_device_key keyboard MOUSE1 1", "wait 180",
            "splitinput_device_key keyboard MOUSE1 0", "wait 420",
            "splitnet_assert_stat 1 score ge 1", "splitnet_status",
            "echo GP2-DUEL:POWER-ROLE-ROTATION", "wait 420", "splitnet_status",
        ]
    else:
        lines += ["echo GP2-DUEL:POWER-UNSUPPORTED-CAPACITY", "wait 180",
                  "splitnet_status"]
    finish(lines, f"gp2_powerduel_{players}p")
    (HERE / f"powerduel_{players}p.cfg").write_text("\n".join(lines) + "\n")
