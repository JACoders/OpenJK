# P2-04: Implement Per-Player 2D State Adapters

## Objective

Implement read-only adapters that let HUD and UI code resolve state for a local
slot without rewriting stock drawing yet.

## Primary ownership

New adapter modules in `codemp/cgame` and `codemp/ui`. Do not edit existing HUD
draw functions, menu assets, input routing, or renderer primitives.

## Work

1. Resolve player state, client info, score, team, vote/chat, profile, cursor,
   console, keyboard, and input owner by local slot.
2. Separate shared server data from per-player mutable data.
3. Return explicit unavailable state rather than global fallback.
4. Add table-driven adapter tests for slots 1-4.

## Acceptance criteria

- Distinct seeded data returns distinct values for all four slots.
- No adapter mutates global CVARs during read operations.
- Invalid/disconnected slots are represented explicitly.
- Existing stock callers compile unchanged until Phase 3 integration.
