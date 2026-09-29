# P5-01: Validate Two-Player Play

## Objective

Run the complete two-player functional, visual, input, and lifecycle matrix.

## Primary ownership

`tests/splitscreen/phase5/two-player`, dedicated runtime homepaths, and its result
manifest only. Do not modify production code or other Phase 5 artifacts.

## Matrix

- Keyboard/mouse plus gamepad, and two-gamepad device assignments.
- FFA, Team FFA, Duel, Power Duel, Siege, CTF, and CTY where supported by maps.
- Profiles, models, sabers, Force powers, controls, consoles, CVAR/cheat keyboards.
- Combat, kills in both directions, respawn, spectate/join, team change, voting,
  scoreboard, top menus, map restart, and return to menu.
- Horizontal and vertical layouts with before/after in-game screenshots.

## Acceptance criteria

- No input, menu, HUD, profile, console, or camera state crosses players.
- Both players can damage, kill, respawn, and resume normal control repeatedly.
- Screenshot oracles and manual inspection pass every representative state.
- Logs contain no crashes, asserts, renderer errors, or protocol violations.
