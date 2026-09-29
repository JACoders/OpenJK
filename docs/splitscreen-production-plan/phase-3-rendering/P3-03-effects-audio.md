# P3-03: Isolate Effects and Audio

## Objective

Make local effects and audio deterministic when cgame is evaluated for several
viewports during one physical frame.

## Primary ownership

Local-entity, marks, particles, camera-effect, and client-audio policy modules.
Do not edit base scene generation, HUD/UI drawing, input, or network state.

## Work

1. Classify effects as shared-once, per-client, or per-view.
2. Prevent simulation, expiry, decals, and one-shot sounds from advancing once
   per viewport.
3. Define listener selection/mixing for 2/3/4 local players.
4. Add counters and deterministic tests for repeated frame evaluation.

## Acceptance criteria

- Effects advance once per physical frame unless explicitly per-view.
- No viewport loses or duplicates weapon, saber, Force, or damage effects.
- Audio does not multiply volume with player count or leak menu sounds.
- Long matches keep local-entity and audio-channel usage bounded.
