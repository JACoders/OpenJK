# Phase 3: Correct Per-Player Rendering

## Entry gate

Phase 2 proves that four independent client frames, cgame contexts, viewport
primitives, and 2D adapters exist. The frozen Phase 1 acceptance contract is the
pass/fail authority.

## Parallel tickets

- [P3-01: Render independent 3D scenes](P3-01-independent-3d-scenes.md)
- [P3-02: Render independent HUDs](P3-02-independent-huds.md)
- [P3-03: Isolate effects and audio](P3-03-effects-audio.md)
- [P3-04: Compose layouts and stock menus](P3-04-layout-menu-composition.md)

## Exit gate

Two-, three-, and four-player in-game screenshots show distinct valid cameras,
world geometry, identities, and clipped stock menus with no unexplained black
regions or duplicated views. Automated image oracles pass and every required
capture has a recorded visual inspection.
