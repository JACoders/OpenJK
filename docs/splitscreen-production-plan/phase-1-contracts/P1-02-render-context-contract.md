# P1-02: Cgame Render-Context Contract

## Objective

Define all cgame state that must be selected or isolated before rendering one
viewport.

## Primary ownership

New `codemp/cgame` context declarations and design notes. No implementation.

## Contract contents

- Snapshot/prediction source, client number, time, camera, refdef, frustum,
  areamask, rendering flags, local entities/effects policy, marks, test models,
  zoom/damage state, and restore semantics.
- Begin-frame, render-3D, render-2D, and end-frame boundaries.
- Explicit list of state shared once per physical frame versus isolated per view.

## Acceptance criteria

- A viewport cannot silently fall back to Player 1 state.
- Nested context activation is rejected or safely supported by contract.
- Restore semantics cover early return and renderer error paths.
- Phase 2 agents can implement against the declarations independently.
