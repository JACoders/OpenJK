# P2-02: Implement Cgame Context Switching

## Objective

Implement safe activation and restoration of the Phase 1 cgame render context.

## Primary ownership

New context implementation in `codemp/cgame`; do not edit client, renderer backend,
HUD draw routines, or UI code.

## Work

1. Snapshot all singleton fields named by the Phase 1 contract.
2. Activate selected client frame, prediction state, time, camera state, and
   per-view transient state.
3. Restore primary/global state on every exit path.
4. Add assertions for inactive, nested, stale, or mismatched contexts.

## Acceptance criteria

- Round-trip activate/restore leaves a checksum of global cgame state unchanged.
- Four contexts report distinct active clients in one physical frame.
- Missing client data produces a defined placeholder, never Player 1 fallback.
- Sanitizer runs show no stale pointers or lifetime violations.
