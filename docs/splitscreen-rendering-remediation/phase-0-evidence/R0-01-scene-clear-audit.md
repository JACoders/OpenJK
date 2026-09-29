# R0-01: Scene and Clear Audit

## Objective

Trace each cgame scene submission through renderer viewport/scissor setup and
frame clearing; identify stale pixels, full-frame clears, and viewport leakage.

## Ownership

Read-only audit of `codemp/client/cl_cgame.cpp` and `codemp/rd-vanilla/*`.
Write findings/tests only under `tests/splitscreen/rendering/scene/`.

## Acceptance

- A table covers 2/3/4 players and both layouts.
- Assertions detect scene coverage outside the expected rectangle.
- Every renderer-state transition has an explicit restore invariant.
