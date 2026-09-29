#!/usr/bin/env python3
"""Generate the bounded native GP3-04 leave/rejoin discovery journey."""

import argparse


parser = argparse.ArgumentParser()
parser.add_argument("players", type=int, choices=(2, 3, 4))
parser.add_argument("server")
parser.add_argument("--cycles", type=int, default=12)
args = parser.parse_args()

players = args.players
secondaries = list(range(2, players + 1))
order = [secondaries[(index * 2 + index // max(1, len(secondaries))) % len(secondaries)]
         for index in range(args.cycles)]

lines = [
    "set com_maxfps 125",
    "set con_notifytime 0",
    "set developer 1",
    "set cl_splitScreen 1",
    f"set ui_splitScreenPlayerCount {players}",
    "set cl_splitScreenLocalCmds 0",
    "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1",
    "set ui_splitScreenP3Input controller2",
    "set ui_splitScreenP4Input controller3",
    f"set name GP3_Churn_{players}_P1",
]
for player in range(2, players + 1):
    lines.append(f"set ui_splitScreenP{player}Name GP3_Churn_{players}_P{player}")

lines += [
    # The Phase 0 runner launches support and client processes together. Give
    # the dedicated server a bounded initialization window before the first
    # connection request.
    "wait 750",
    f"splitnet_party_connect {args.server}",
    f"connect {args.server}",
    "wait 900",
    "closemenu",
    "cmd team free",
]
for player in secondaries:
    lines.append(f"splitnet_cmd {player} team free")
lines += ["wait 360", "echo GP3-04:INITIAL-BEGIN",
          "splitui_assert ui_splitScreenPartyState active"]
for player in range(1, players + 1):
    lines += [f"splitnet_assert_lifecycle {player} ALIVE",
              f"splitnet_assert_stat {player} clientnum eq {player - 1}"]
lines += ["echo GP3-04:INITIAL-END"]

for cycle, target in enumerate(order, 1):
    lines += [
        f"echo GP3-04:CYCLE-{cycle:03d}-P{target}-BEGIN",
        f"splitnet_assert_lifecycle {target} ALIVE",
        f"splitnet_disconnect {target}",
        "wait 90",
        "splitui_assert ui_splitScreenPartyState partial",
    ]
    for survivor in range(1, players + 1):
        if survivor != target:
            lines.append(f"splitnet_assert_lifecycle {survivor} ALIVE")
    lines += [
        f"splitnet_rejoin {target} forcechanged free",
        "wait 420",
        "splitui_assert ui_splitScreenPartyState active",
    ]
    for player in range(1, players + 1):
        lines.append(f"splitnet_assert_lifecycle {player} ALIVE")
    lines += [
        f"splitnet_assert_stat {target} clientnum eq {target - 1}",
        f"echo GP3-04:CYCLE-{cycle:03d}-P{target}-END",
    ]

lines += [
    "splitnet_status",
    f"screenshot_png gp3_wan_churn_{players}p",
    "condump gp3_wan_churn_console.txt",
    "echo GP3-04:COMPLETE",
    "wait 30",
    "quit",
]
print("\n".join(lines))
