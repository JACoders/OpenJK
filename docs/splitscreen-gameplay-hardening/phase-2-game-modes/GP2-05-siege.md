# GP2-05: Siege Objectives and Rounds

## Objective

Exercise Siege as a real class/objective/round mode rather than a spawn-and-input
smoke test.

## Exclusive ownership

- `tests/splitscreen/gameplay/modes/siege/**`
- Evidence under `tests/splitscreen/gameplay/results/modes/siege/**`

## Scenarios

- Select teams and distinct legal classes through each player pane.
- Complete at least one stock objective chain, including use interactions,
  objective state changes, class combat, death, respawn wave, and class change.
- Reach round end, observe side swap, scoreboard/intermission, ready, restart,
  and next-map behavior.
- Cover the supported 2/3/4-player behavior with controlled bots or remote
  clients where a minimum team population is required.

## Acceptance

- Class, inventory, objective progress, wave timer, team, round score, and side
  swap match server state per player.
- Death/respawn and round turnover do not blank, duplicate, or misassign panes.
- Unsupported low-population cases fail visibly and recoverably.

## Defect handling

Record findings for GP4-03, GP4-04, or GP4-05; do not patch production code.

## Status

FINAL-HASH SUPPORTED MODE GATES PASS.

The Siege row passes at 2p, 3p, and 4p. All three manifests are hash-pinned to
final client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`
and record `status: passed`.

Evidence:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/{modes.tsv,modes-result.json}`
- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`

This promotes the implemented supported Siege gate. Class/inventory/objective
permutations, round-side swaps, controlled population, or presentation/audio
claims absent from the final manifests remain unclaimed.
