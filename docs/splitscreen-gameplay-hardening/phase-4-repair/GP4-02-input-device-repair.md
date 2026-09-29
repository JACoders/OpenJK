# GP4-02: Input and Device Repairs

## Objective

Fix all defects in keyboard/mouse ownership, controller routing, hotplug,
remapping, concurrent input, and stuck-command cleanup.

## Exclusive ownership

- `codemp/client/cl_input.cpp`
- `codemp/client/cl_keys.cpp`
- Platform input files explicitly assigned in the defect ledger
- `tests/splitscreen/gameplay/regressions/input/**`

This ticket does not edit UI, session/network, game, cgame, renderer, or audio
source.

## Work

- Add a minimal failing command/focus trace for every assigned defect.
- Repair mouse confinement, device identity, disconnect/reconnect, enumeration
  reorder, held-input neutralization, remap persistence, and simultaneous input.
- Preserve the invariant that no fallback device silently takes another
  player's ownership.

## Acceptance

- Every assigned reproduction fails before and passes after the patch.
- Per-player command streams remain isolated under all chord/release orders.
- Input restart, map/VM restart, modal transition, and process quit leave every
  key/button/axis neutral and release all device resources.

## Evidence

Attach before/after input traces, ownership maps, command assertions, and
hotplug lifecycle logs.

## Status

IMPLEMENTED AND RUNTIME VERIFIED FOR COVERED CELLS on 2026-07-23 against final
frozen executable
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.

Implemented repairs:

- `GP4-02-INPUT-01` — keyboard/mouse UI ownership bleed. Primary key and mouse
  paths previously wrote `ui_splitScreenInputTarget=0`, discarding the
  explicitly assigned keyboard/mouse player and reproducing GP1-02's Force
  target loss. They now resolve the configured keyboard owner, update both
  input/profile targets, identify mouse buttons as mouse input, and retain zero
  only when the device is genuinely unassigned. No fallback pane is selected.
- `GP4-02-INPUT-02` — repeated controller edges and incomplete detach cleanup.
  SDL previously cleared all controller states and then repopulated connected
  devices on every poll. With edge tracking, a held button therefore became a
  new press every frame. Polling now updates each assigned device in place and
  clears only players without an assigned live device. The shared clear path
  neutralizes axes, buttons, pending edges, console chords, and respawn/attack
  suppression after detach, reassignment, split disable, or explicit QA clear.
  Invalid clear-player indices are rejected.
- `GP4-02-INPUT-03` — GP3-05 primary MOUSE1 diagnostic. Source/protocol
  regression proves observed value `2` is `BUTTON_TALK`, added because P1 had
  an active key catcher; it is not a remapped `BUTTON_ATTACK` bit (`1`).
  Changing attack constants would be incorrect. A post-build runtime must
  establish and assert the intended catcher state before comparing attack-only
  button values.
- `GP4-02-INPUT-04` — external virtual-controller bridge state disappeared
  before usercmd generation. The bridge opened a second temporary SDL joystick,
  applied virtual setters, and immediately closed it while gameplay polled the
  persistent `splitSticks[]` handles. Corrected GP5-03 logs consequently
  acknowledged distinct Controller 1/2/3 packets but emitted neutral P2-P4
  commands. Bridge packets now mutate the already-open persistent handle.
  Bridge-only virtual slots read raw JOY0..JOY15 indices so JOY10 remains
  JOY10; physical devices retain SDL's standardized GameController mapping.

Focused regression and runtime evidence:

- `tests/splitscreen/gameplay/regressions/input/test_source_regressions.py`
  passes and locks explicit keyboard/mouse ownership, stable held-button edges,
  detach neutralization, clear bounds, the GP3-05 button-bit diagnosis,
  persistent bridge-handle mutation, raw virtual JOY indexing, and preservation
  of the physical GameController branch.
- The assigned client and SDL translation units pass the existing macOS
  compiler invocation in `-fsyntax-only` mode. Existing unrelated writable-
  string warnings in `cl_keys.cpp` remain warnings.
- `git diff --check` passes for the assigned source, regression, and ticket
  paths.
- The final 2p/3p visible-UI journeys prove that keyboard/mouse changes only P1
  while Controllers 1/2 change only their assigned players in setup and live
  gameplay.
- The final virtual-controller ledgers prove stable ownership and neutral
  release before and after `in_restart`: four required input assertions pass at
  2p and eight pass at 3p. Their reports record `passed: true`.
- The final four-player aggregate proves isolated keyboard/mouse and
  Controllers 1–3 movement/attack in local and public gameplay.
- The final mouse-focus matrix records 76 covered passes, zero discovered
  failures, and passing 2/3/4-player manifests. It covers resize,
  `vid_restart`, live layout/count changes, boundary movement, and all stable
  routed surfaces.
- Current-hash runtime indexes:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/virtual-controller.tsv`,
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/virtual-controller.tsv`,
  `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/matrix.json`,
  and `tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/`.

Remaining unsupported hardware/runtime cases:

- Physical SDL device removal/addition and GUID-preserving enumeration reorder.
- Physical remap persistence and simultaneous keyboard/mouse plus three
  physical controllers.
- Dedicated four-player virtual-controller recovery across `in_restart`.
- The mouse matrix's 15 blocked surface/fullscreen/reassignment/inventory cells
  remain explicit in GP1-02; the covered subset is not an exhaustive click
  inventory.
- Whole-party/server-loss recovery and audio/resource behavior are separate
  unresolved gates, not input-ticket passes.

No UI, session/network, game, cgame, renderer, or audio source was edited by
this ticket.

## Controller-binding completeness contract

An independent source contract now lives at
`tests/splitscreen/gameplay/contracts/controller_bindings/test_binding_completeness.py`.
It enumerates every row in `cl_splitScreenBindCommands`, explicitly separates
the eight settings-only rows from executable bindings, and requires every
advertised executable row to have a route in
`CL_SplitScreenApplyButtonBindings`.

The contract now has three explicit classes. The eight settings-only rows and
the two stock actions whose implementations are process-global (`+mlook` and
`voicechat`) are nonbindable. Both the engine
`splitscreen_bind_controller` CLI and the split-controls UI capture path must
reject that exact set before they can evict an existing executable binding.
The remaining executable rows must have dispatch routes, including the four
actions repaired with player-local semantics: `+strafe`, `centerview`,
`automap_toggle`, and `cg_thirdperson !`.

This closes two separate failure modes: a dead advertised executable action,
and a settings/global-only row consuming a controller button while silently
unbinding the action that previously owned it. Weapon, inventory, and Force
cycling remain required player-routed cgame dispatch.
