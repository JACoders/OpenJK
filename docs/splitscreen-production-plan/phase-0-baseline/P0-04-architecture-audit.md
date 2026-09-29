# P0-04: Audit Existing Architecture and Tests

## Objective

Document the actual state flow and identify tests that gave false confidence.

## Primary ownership

Architecture documentation only. Do not edit production or test code.

## Work

1. Trace Player 1 and Players 2-4 through server entities, client connections,
   snapshots, cgame parsing, prediction, scene construction, HUD, and input.
2. Identify every global singleton reused while rendering extra viewports.
3. Classify existing tests as connection, simulation, functional, visual, or
   end-to-end and list what each does not prove.
4. Map local-command fake clients versus vanilla network split clients.
5. Record file ownership and risky shared functions for Phase 1 contracts.

## Acceptance criteria

- The audit explains the repeated camera, Player 1 HUD, and black-region failures.
- Every existing split-screen test has an honest coverage classification.
- Unknowns are explicit and assigned to later tickets.
- No “complete” status is inferred from unsupported evidence.
