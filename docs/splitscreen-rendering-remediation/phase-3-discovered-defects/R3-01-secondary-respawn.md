# R3-01: Secondary-Player Respawn Routing

## Ownership

Secondary input/usercmd routing in `codemp/client/*`, `shared/sdl/*`, and focused
lifecycle tests. Do not edit rendering or UI code.

## Acceptance

- Players 2–4 respawn from a controller attack edge after death.
- Held buttons do not auto-respawn without a new edge.
- Player 1 and ordinary single-player input remain unchanged.
