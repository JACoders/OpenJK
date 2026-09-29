# R3-05: Per-Player Intermission State

## Ownership

Client/cgame intermission dispatch and focused lifecycle/rendering tests.

## Acceptance

- Every active viewport enters intermission and paints its scoreboard.
- No viewport retains gameplay HUD, weapon, or center-print overlays.
- Ordinary one-player intermission behavior remains unchanged.
