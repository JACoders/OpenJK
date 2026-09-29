# P3-01: Render Independent 3D Scenes

## Objective

Build each viewport's world scene from its own frame, prediction state, camera,
areamask, and cgame context.

## Primary ownership

3D scene generation and camera paths in `codemp/cgame/cg_view.c` plus dedicated
scene helpers. Do not edit local effects/audio, HUD drawing, UI composition,
input, or networking.

## Work

1. Generate refdef, view origin/angles, FOV, frustum, and areamask per slot.
2. Add entities, world geometry, weapons, and camera state from that slot's
   snapshot and prediction context.
3. Remove Player 1 fallbacks and fail visibly/diagnostically on missing context.
4. Cover first/third person, spectate, death, respawn, zoom, and intermission.

## Acceptance criteria

- Deliberately separated players produce measurably different camera frames.
- Every active viewport contains valid world coverage without black wedges.
- Rendering one viewport does not mutate the next viewport's scene inputs.
- One-player rendering remains visually unchanged.
