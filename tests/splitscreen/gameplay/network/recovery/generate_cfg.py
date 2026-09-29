#!/usr/bin/env python3
"""Generate ordered GP3-03 controlled-server recovery probes."""

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("players", type=int, choices=(2, 3, 4))
parser.add_argument("address")
args = parser.parse_args()
p = args.players
address = args.address
target = p

lines = [
    "set cl_splitScreen 1",
    f"set ui_splitScreenPlayerCount {p}",
    "set cl_splitScreenLocalCmds 0",
    "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1",
    "set ui_splitScreenP3Input controller2",
    "set ui_splitScreenP4Input controller3",
    "set con_notifytime 0",
    "set password \"\"",
    "set rconpassword GP3Recovery",
    f"set rconAddress {address}",
    f"set name GP3_Recovery_{p}_P1",
]
for player in range(2, p + 1):
    lines.append(f"set ui_splitScreenP{player}Name GP3_Recovery_{p}_P{player}")
lines += [
    f"splitnet_party_connect {address}",
    f"connect {address}",
    # Party handshakes are serialized; wait until the highest local slot owns
    # an authoritative snapshot before issuing its first team command.
    "wait 4200",
    "closemenu",
    "cmd team free",
]
for player in range(2, p + 1):
    lines.append(f"splitnet_cmd {player} team free")
lines += ["wait 300", "echo GP3-03:BASELINE-BEGIN", "splitui_assert ui_splitScreenPartyState active"]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
        f"splitnet_assert_stat {player} team eq 0",
    ]
lines += [
    "screenshot_png gp3_recovery_baseline",
    "echo GP3-03:BASELINE-END",
    "echo GP3-03:SECONDARY-DISCONNECT-BEGIN",
    f"splitnet_disconnect {target}",
    "wait 120",
    "splitui_assert ui_splitScreenPartyState partial",
]
for player in range(1, p):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_partial",
    "echo GP3-03:SECONDARY-DISCONNECT-END",
    "echo GP3-03:SECONDARY-REJOIN-BEGIN",
    f"splitnet_rejoin {target} forcechanged free",
    "wait 720",
]
if p == 4:
    # The deferred command is delivered as soon as the fourth connection
    # becomes active, which can precede its first gameplay-ready snapshot.
    # Retry once after the serialized P4 handshake window.
    lines += [f"splitnet_cmd {target} forcechanged free", "wait 600"]
lines += [
    "splitui_assert ui_splitScreenPartyState active",
]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_rejoin",
    "echo GP3-03:SECONDARY-REJOIN-END",
    "echo GP3-03:RESTART-BEGIN",
    "rcon map_restart 0",
    "wait 600",
]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_restart",
    "echo GP3-03:RESTART-END",
    "echo GP3-03:MAP-TRANSITION-BEGIN",
    "rcon map mp/ffa2",
    "wait 720",
]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_second_map",
    "echo GP3-03:MAP-TRANSITION-END",
    "echo GP3-03:P1-PARTY-DISCONNECT-BEGIN",
    "disconnect",
    "wait 180",
    f"splitnet_party_connect {address}",
    f"connect {address}",
    "wait 4200",
    "closemenu",
    "cmd team free",
]
for player in range(2, p + 1):
    lines.append(f"splitnet_cmd {player} team free")
lines += ["wait 300", "splitui_assert ui_splitScreenPartyState active"]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_party_reconnect",
    "echo GP3-03:P1-PARTY-DISCONNECT-END",
    "echo GP3-03:SERVER-LOSS-BEGIN",
    "rcon quit",
    "wait 360",
    f"splitnet_party_connect {address}",
    f"connect {address}",
    # The server driver restarts after two seconds, then P2-P4 reconnect in
    # sequence. Keep this phase open through both delays.
    "wait 4800",
    "closemenu",
    "cmd team free",
]
for player in range(2, p + 1):
    lines.append(f"splitnet_cmd {player} team free")
lines += ["wait 300", "splitui_assert ui_splitScreenPartyState active"]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png gp3_recovery_server_loss",
    "echo GP3-03:SERVER-LOSS-END",
    "splitnet_status",
    "condump gp3_recovery_console.txt",
    "echo GP3-03:COMPLETE",
    "wait 30",
    "quit",
]
print("\n".join(lines))
