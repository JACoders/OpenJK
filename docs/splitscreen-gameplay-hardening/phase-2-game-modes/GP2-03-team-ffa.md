# GP2-03: Team FFA Lifecycle

## Objective

Complete team assignment, combat, scoring, spectate/rejoin, intermission, and
restart for 2/3/4 local players.

## Exclusive ownership

- `tests/splitscreen/gameplay/modes/team_ffa/**`
- Evidence under `tests/splitscreen/gameplay/results/modes/team_ffa/**`

## Scenarios

- Select teams and Force/model profiles through each pane.
- Exercise uneven 2/3-player teams and balanced 4-player teams.
- Verify enemy kills, suicides, team kills with friendly fire off/on, team
  score, personal score, death, respawn, team switch, and spectate/rejoin.
- Reach scorelimit, observe team intermission, ready, run `map_restart`, and
  rotate to a second map.

## Acceptance

- Team and personal scoring follow server rules for every event.
- No team/model/Force state crosses local clients during restart or map change.
- Team HUD, scoreboard, announcer state, and spawn camera belong to each pane.

## Defect handling

Record findings for GP4-03, GP4-04, or GP4-05; do not patch production code.

## Status

FINAL-HASH SUPPORTED MODE GATES PASS.

The Team FFA row passes at 2p, 3p, and 4p. All three Phase 0 manifests are
hash-pinned to final client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`
and record `status: passed`.

Evidence:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/{modes.tsv,modes-result.json}`
- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`

This closes the implemented supported gate only; friendly-fire permutations or
presentation/audio assertions absent from the final manifests remain unclaimed.
