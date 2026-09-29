# P2-01: Store Independent Client Frames

## Objective

Implement the Phase 1 snapshot contract for each configured local client.

## Primary ownership

`codemp/client/cl_main.cpp`, packet/snapshot storage helpers, and contract tests.
Do not edit cgame, renderer, UI, or input code.

## Work

1. Preserve independent connection, gamestate, snapshots, reliable commands,
   server commands, configstrings, timing, and errors for slots 1-4.
2. Implement read-only frame acquisition with stable lifetime.
3. Unify local-host and vanilla-network paths at the contract boundary.
4. Handle map changes, reconnects, missing snapshots, and slot teardown.

## Acceptance criteria

- Four clients expose different client numbers, origins, angles, and snapshots.
- Packet processing for one slot cannot mutate another slot's frame.
- Snapshot history and memory remain bounded.
- Existing one-player networking is byte-for-byte behaviorally unchanged.
