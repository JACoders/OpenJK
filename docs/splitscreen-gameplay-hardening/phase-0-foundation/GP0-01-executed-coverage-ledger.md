# GP0-01: Executed Coverage Ledger

## Objective

Replace prose claims with a machine-generated player-count × mode × lifecycle
ledger derived from passing logs and artifact manifests.

## Exclusive ownership

- `tests/splitscreen/gameplay/coverage/**`
- `docs/splitscreen-gameplay-hardening/generated/coverage.*`

No production source changes are allowed.

## Work

- Define explicit states for setup, spawn, input, combat, score, death, respawn,
  objective, spectate/rejoin, intermission, restart, next map, and clean exit.
- Import existing evidence without upgrading spawn-only cases to end-to-end.
- Record build hash, binary hash, runner, config, player count, mode, endpoint
  class, device assignment, and evidence paths.
- Generate Markdown and JSON views from the same source.
- Identify every empty or smoke-only matrix cell for Phases 1–3.

## Acceptance

- Every claimed pass links to a log assertion and required screenshot.
- Missing, stale, contradictory, or wrong-build artifacts fail generation.
- The generated matrix explicitly exposes the current objective-mode and
  transition gaps.

## Evidence

Archive the generated JSON, Markdown, import report, and invalid-fixture test
under `tests/splitscreen/gameplay/coverage/results/`.

## Status

COMPLETE (2026-07-23).

- Generator accepted 5 conservatively imported evidence records and emitted 27
  player-count × stock-mode cells from one source.
- Validation passed with hash-pinned runners, configs, logs, screenshots, and
  binary metadata.
- Nine self-tests passed, including generated-link existence, legacy seal
  exclusion, and missing, stale, wrong-build, missing-marker, contradictory-
  claim, and missing-correlation rejection.
- The generated ledger exposes 24 empty cells, one smoke-only cell, and no
  imported objective, next-map, or clean-exit claim.
- Imported historical evidence is explicitly `legacy-unverified` because its
  logs do not embed a binary hash; it is behaviorally informative but cannot
  qualify a frozen-build end-to-end or seal claim.
- The ledger's binary guard was refreshed to the current Phase 5 frozen
  candidate SHA-256
  `737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
  Generation and all nine negative/positive validation tests pass; the imported
  historical evidence remains `legacy-unverified`.
