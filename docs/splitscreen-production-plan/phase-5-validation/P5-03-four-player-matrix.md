# P5-03: Validate Four-Player Play

## Objective

Stress the maximum local-player configuration across gameplay and UI workflows.

## Primary ownership

`tests/splitscreen/phase5/four-player`, dedicated runtime homepaths, and its result
manifest only. Do not modify production code or other Phase 5 artifacts.

## Matrix

- Four gamepads and keyboard/mouse plus three-gamepad assignments.
- FFA and team combat with all players moving, attacking, using Force, taking
  damage, dying, respawning, joining, and spectating concurrently.
- Per-player character/saber/Force changes, controls, console, virtual keyboard,
  top menu, scoreboard, votes, chat, and team selection.
- Repeated map/game-type rotations and 2-to-4/4-to-2 party reconfiguration.
- Four-player gameplay, menus, effects-heavy scenes, and intermission captures.

## Acceptance criteria

- All four views show independent current world state and correct HUD identity.
- Input-routing telemetry proves zero cross-slot delivery.
- Frame time, memory, entities, effects, and audio stay within frozen budgets.
- Visual and log review finds no duplication, black gaps, overflow, or crashes.
