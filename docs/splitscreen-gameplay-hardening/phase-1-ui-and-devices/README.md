# Phase 1: UI and Device Journeys

## Entry gate

Phase 0's runner, manifest, coverage ledger, and oracle self-tests pass.

## Parallel tickets

- [GP1-01: Fresh-home profiles, sabers, and persistence](GP1-01-profiles-sabers-persistence.md)
- [GP1-02: Mouse confinement and focus ownership](GP1-02-mouse-focus-confinement.md)
- [GP1-03: Controller hotplug, remap, and concurrent input](GP1-03-controller-lifecycle.md)
- [GP1-04: Pause, chat, console, and scoreboard ownership](GP1-04-modal-ownership.md)

## Independence rule

These are discovery tickets. Each owns only its named test/evidence directory;
all production defects enter the shared immutable ledger for Phase 4. This
allows all four tickets to run concurrently without source conflicts.

## Exit gate

Every journey has a deterministic pass or reproduction for 2/3/4 players, all
findings are assigned to a Phase 4 component lane, and no result depends on
direct per-player profile cvar writes.

## Result

PASS FOR DISCOVERY on 2026-07-23 against native executable SHA-256
`223e8a126722a5728076fa1cbba2d4954f2cf51a849951c6af25a9ae25eabc33`.

- GP1-01 retained 2p/3p passing journeys and a deterministic 4p lifecycle
  failure as `GP1-01-4P-001`.
- GP1-02 retained 68 covered/pass, 8 deterministic failure, and 15 explicitly
  blocked cells.
- GP1-03 retained 2 covered/pass, 4 deterministic failure, and 13 unsupported
  physical-device cells.
- GP1-04 retained 1 covered/pass, 17 failure, and 9 unsupported cells from the
  current matrix, plus earlier immutable samples demonstrating intermittent
  SDL bridge behavior.

All final run manifests pass Phase 0 hash/artifact validation. Product
acceptance is intentionally not claimed; every failure is routed to the
component-exclusive Phase 4 repair lanes.
