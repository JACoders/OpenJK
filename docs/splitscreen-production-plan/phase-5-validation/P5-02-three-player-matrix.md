# P5-02: Validate Three-Player Play

## Objective

Run the complete three-player functional, visual, input, and lifecycle matrix.

## Primary ownership

`tests/splitscreen/phase5/three-player`, dedicated runtime homepaths, and its
result manifest only. Do not modify production code or other Phase 5 artifacts.

## Matrix

- Keyboard/mouse plus two gamepads, and three-gamepad assignments.
- Free-for-all and team modes across small, medium, and large stock maps.
- Independent profiles, controls, menus, consoles, loadouts, and Force powers.
- Simultaneous movement, saber/ranged combat, Force use, deaths, respawns,
  spectating, joining, voting, scoreboard, and map changes.
- Three-player layout under normal gameplay and every modal/top-menu state.

## Acceptance criteria

- All three cameras and HUD identities remain distinct throughout the run.
- Simultaneous events route to exactly one intended local client each.
- No viewport has black geometry, stale frames, clipping, or overlay collisions.
- Logs, telemetry, screenshot oracles, and manual review all pass.
