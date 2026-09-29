# R1-02: Cgame HUD Isolation

## Ownership

`codemp/cgame/cg_draw*.c`, cgame-local headers, and HUD regression tests.

## Work

Apply one scoped viewport transform/clip to all 2D drawing and restore every
mutable draw state after each player's HUD and overlay pass.

## Acceptance

- HUD/menu pixels never cross a seam.
- Scoreboard, spectator, death, zoom, and intermission are correct per player.
- Reversing player render order produces identical pixels.
