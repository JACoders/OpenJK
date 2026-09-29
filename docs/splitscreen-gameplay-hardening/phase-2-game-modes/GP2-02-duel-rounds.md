# GP2-02: Duel and Power Duel Rounds

## Objective

Exercise real multi-round Duel and Power Duel rules, queues, scoring, turnover,
and terminal states.

## Exclusive ownership

- `tests/splitscreen/gameplay/modes/duel/**`
- Evidence under `tests/splitscreen/gameplay/results/modes/duel/**`

## Scenarios

- Two-player Duel: both players win rounds, die, re-enter, reach winlimit,
  observe intermission, ready, and begin the next match.
- Three- and four-player Duel: prove spectator queue order, promotion, voluntary
  spectate/rejoin, disconnect/rejoin, and unsupported-capacity messaging.
- Three-player Power Duel: assign single/double roles via UI, win in both
  directions, rotate roles, score, reach winlimit, intermission, and restart.
- Two-/four-player Power Duel: prove defined capacity/queue/error behavior
  without ghost roles or frozen panes.

## Acceptance

- Round winner, score, duel role, queue order, death, and promotion match the
  server's authoritative state.
- All devices regain correct control after every round transition.
- No player inherits another player's camera, role, model, HUD, or command.

## Defect handling

Record findings for GP4-03, GP4-04, or GP4-05; do not patch production code.

## Status

FINAL-HASH SUPPORTED MODE GATES PASS.

The Duel and Power Duel rows pass at 2p, 3p, and 4p: six relevant manifests
record `status: passed` against final client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The 2p/3p exact-suite mode error maps are empty, and the 4p nine-mode result
records every check true.

Evidence:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/{modes.tsv,modes-result.json}`
- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`

This is a supported-gate promotion, not a claim for queue indices, every role
rotation, or every multi-round permutation absent from the final manifests.
