# P3-02: Render Independent HUDs

## Objective

Render stock multiplayer HUD and scoreboard content from the active local slot.

## Primary ownership

HUD, scoreboard, vote, chat, and status drawing in `codemp/cgame/cg_draw.c` and
HUD-specific helpers. Do not edit 3D scenes, menu UI, input, or audio.

## Work

1. Resolve identity, portrait, health, armor, ammo, Force, weapons, score, team,
   duel, vote, chat, damage, and spectator state through Phase 2 adapters.
2. Apply per-viewport transforms and clipping to every stock HUD element.
3. Preserve shared match data while isolating mutable player state.
4. Add deterministic HUD fixtures for slots 1-4.

## Acceptance criteria

- Four seeded identities render four correct names, portraits, and status sets.
- Player 2-4 never display Player 1 data through a global fallback.
- Scoreboards and shared announcements remain coherent in every viewport.
- HUD pixels remain inside their assigned viewport in all layouts.
