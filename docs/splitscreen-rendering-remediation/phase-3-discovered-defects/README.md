# Phase 3: Certification-Discovered Defects

## Entry gate

Phase 2 evidence is frozen, including failures and unexercised requirements.

## Parallel tickets

- [R3-01: Secondary-player respawn routing](R3-01-secondary-respawn.md)
- [R3-02: Deterministic intermission oracle](R3-02-intermission-oracle.md)
- [R3-03: Full-duration state-leak soak](R3-03-state-leak-soak.md)
- [R3-04: Preserve UI text pixel budgets](R3-04-ui-text-budget.md)
- [R3-05: Per-player intermission state](R3-05-per-player-intermission.md)

## Exit gate

The respawn failure is green, intermission is exercised for every viewport, and
the 30-minute soak passes on the same rebuilt binary with bounded RSS evidence.
