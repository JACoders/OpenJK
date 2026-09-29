# P1-04: Visual and Functional Acceptance Contract

## Objective

Freeze machine-checkable success criteria before implementation begins.

## Primary ownership

Test schemas, manifests, and documentation only. Do not alter game code.

## Work

1. Define required telemetry and screenshot fields for every scenario.
2. Define tolerances for black regions, viewport duplication, movement, clipping,
   HUD identity, frame timing, and input isolation.
3. Define mandatory manual visual-review checklist and reviewer evidence.
4. Define severity and release-blocking policy.
5. Define a test as passing only when launch, state, visual, and teardown gates pass.

## Acceptance criteria

- The current broken four-player capture fails the contract.
- Each criterion maps to an automated assertion or named manual inspection.
- Flaky/missing artifacts produce failure, not “not applicable.”
- Criteria cannot be weakened inside implementation tickets.
