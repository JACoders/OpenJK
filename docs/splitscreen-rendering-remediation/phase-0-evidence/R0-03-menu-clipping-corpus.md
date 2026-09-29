# R0-03: Menu and Clipping Corpus

## Objective

Build a deterministic screenshot corpus for stock and split-screen menus and
define clipping/oracle tolerances at viewport seams and screen edges.

## Ownership

Read-only audit of `codemp/ui/*`; writes limited to
`tests/splitscreen/rendering/menu/` and corpus documentation.

## Acceptance

- Captures cover top menu, profile, controls, saber, Force, console, and OSK.
- Seam and out-of-bounds pixel oracles fail on the preserved defect images.
- Baseline capture commands are deterministic and isolated.
