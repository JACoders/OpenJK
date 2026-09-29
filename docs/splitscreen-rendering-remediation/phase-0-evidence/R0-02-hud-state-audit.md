# R0-02: HUD and Draw-State Audit

## Objective

Inventory all cgame 2D entry points and state that can leak between HUD draws,
including transform, color, shader, text scale, and menu ownership.

## Ownership

Read-only audit of `codemp/cgame/*`. Write findings/tests only under
`tests/splitscreen/rendering/hud/`.

## Acceptance

- Every draw primitive used by stock HUD paths is classified.
- Regression cases cover scoreboard, spectator, death, zoom, and intermission.
- Missing reset/clip operations are tied to exact call paths.
