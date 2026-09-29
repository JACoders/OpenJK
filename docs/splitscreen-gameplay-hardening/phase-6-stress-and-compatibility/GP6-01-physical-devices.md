# GP6-01: Physical-Device Certification

## Objective

Confirm simulator results with real keyboard/mouse and three physical
controllers across supported connection types.

## Exclusive ownership

- `tests/splitscreen/gameplay/stress/physical_devices/**`
- Evidence under `tests/splitscreen/gameplay/results/stress/physical_devices/**`

No production changes are allowed.

## Matrix

- USB and Bluetooth controllers where available, mixed controller models, cold
  launch, sleep/wake, disconnect/reconnect, enumeration reorder, and battery/
  link interruption.
- UI setup, profile/saber editing, combat, objectives, modal surfaces, held-input
  removal, remapping, `in_restart`, and process relaunch.
- 2/3/4 players with simultaneous physical input and every supported layout.

## Acceptance

- Device identity and explicit ownership remain stable or recover through a
  visible assignment flow.
- No unplug, reconnect, sleep/wake, or input chord causes bleed or a stuck
  command.
- Simulator traces and physical-device traces agree at the command boundary.

## Evidence

Record device identifiers without personal data, OS/runtime versions, ownership
maps, event traces, screenshots/video checkpoints, and operator checklist.

## Status

PLANNED.
