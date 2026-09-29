#!/usr/bin/env python3
"""Generate a bounded virtual-controller restart/ownership probe."""

import argparse


parser = argparse.ArgumentParser()
parser.add_argument("players", type=int, choices=(2, 3))
args = parser.parse_args()
p = args.players

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
    "devmap mp/ffa3",
    "wait 480",
    "closemenu",
    "cmd team free",
    "splitnet_connect 2 localhost",
]
if p == 3:
    lines.append("splitnet_connect 3 localhost")
lines += ["wait 360", "splitnet_cmd 2 team free"]
if p == 3:
    lines.append("splitnet_cmd 3 team free")
lines += [
    "wait 180",
    "splitscreen_bind_controller 2 4 +attack",
]
if p == 3:
    lines.append("splitscreen_bind_controller 3 5 +force_lightning")
lines += [
    "echo CERT-CONTROLLER:PRE-READY",
    "wait 240",
    "splitinput_assert_cmd 2 -998 0 0 1",
]
if p == 3:
    lines.append("splitinput_assert_cmd 3 0 -998 0 1024")
lines += [
    "echo CERT-CONTROLLER:PRE-CHECKED",
    "wait 120",
    "splitinput_assert_cmd 2 0 0 0 0",
]
if p == 3:
    lines.append("splitinput_assert_cmd 3 0 0 0 0")
lines += [
    "in_restart",
    "wait 240",
    "echo CERT-CONTROLLER:POST-READY",
    "wait 240",
    "splitinput_assert_cmd 2 -998 0 0 1",
]
if p == 3:
    lines.append("splitinput_assert_cmd 3 0 -998 0 1024")
lines += [
    "echo CERT-CONTROLLER:POST-CHECKED",
    "wait 120",
    "splitinput_assert_cmd 2 0 0 0 0",
]
if p == 3:
    lines.append("splitinput_assert_cmd 3 0 0 0 0")
lines += [
    "screenshot_png cert_virtual_controller_restart",
    "echo CERT-CONTROLLER:COMPLETE",
    "wait 30",
    "quit",
]
print("\n".join(lines))
