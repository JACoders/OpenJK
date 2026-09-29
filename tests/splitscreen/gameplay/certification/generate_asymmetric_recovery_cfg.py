#!/usr/bin/env python3
"""Generate the supported secondary-slot disconnect/rejoin recovery probe."""

import argparse


parser = argparse.ArgumentParser()
parser.add_argument("players", type=int, choices=(2, 3))
parser.add_argument("address")
args = parser.parse_args()
p = args.players
target = p

lines = [
    "set cl_splitScreen 1",
    f"set ui_splitScreenPlayerCount {p}",
    "set cl_splitScreenLocalCmds 0",
    "set ui_splitScreenP1Input keyboard",
    "set ui_splitScreenP2Input controller1",
]
if p == 3:
    lines.append("set ui_splitScreenP3Input controller2")
lines += [
    "set con_notifytime 0",
    "set password \"\"",
    f"splitnet_party_connect {args.address}",
    f"connect {args.address}",
    "wait 720",
    "closemenu",
    "cmd team free",
]
for player in range(2, p + 1):
    lines.append(f"splitnet_cmd {player} team free")
lines += [
    "wait 300",
    "echo CERT-ASYMMETRIC:BASELINE-BEGIN",
    "splitui_assert ui_splitScreenPartyState active",
]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png cert_asymmetric_baseline",
    "echo CERT-ASYMMETRIC:BASELINE-END",
    "echo CERT-ASYMMETRIC:DISCONNECT-BEGIN",
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
    "screenshot_png cert_asymmetric_partial",
    "echo CERT-ASYMMETRIC:DISCONNECT-END",
    "echo CERT-ASYMMETRIC:REJOIN-BEGIN",
    f"splitnet_rejoin {target} forcechanged free",
    "wait 720",
    "splitui_assert ui_splitScreenPartyState active",
]
for player in range(1, p + 1):
    lines += [
        f"splitnet_assert_lifecycle {player} ALIVE",
        f"splitnet_assert_stat {player} clientnum eq {player - 1}",
    ]
lines += [
    "screenshot_png cert_asymmetric_rejoined",
    "echo CERT-ASYMMETRIC:REJOIN-END",
    "echo CERT-ASYMMETRIC:COMPLETE",
    "wait 30",
    "quit",
]
print("\n".join(lines))
