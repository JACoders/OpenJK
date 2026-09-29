# GP2-01: FFA, Holocron, and Jedi Master Lifecycles

## Objective

Complete the individual-scoring modes for 2/3/4 local players.

## Exclusive ownership

- `tests/splitscreen/gameplay/modes/individual/**`
- Evidence under `tests/splitscreen/gameplay/results/modes/individual/**`

## Scenarios

- FFA: every player kills and is killed, scores, respawns, spectates/rejoins,
  reaches fraglimit, sees intermission, readies, and enters the next match/map.
- Holocron: acquire powers, kill the holder, drop/transfer pickups, score with
  the acquired powers, respawn, hit the limit, and restart.
- Jedi Master: acquire the saber, score as Master, kill the Master, transfer the
  role/saber, respawn, reach intermission, and restart.
- Repeat with 2/3/4 players and a remote fifth-client control where supported.

## Acceptance

- Score, holder/role, pickup, death, and respawn assertions identify the correct
  client in every pane.
- Intermission resets or preserves exactly the mode-defined state.
- All split clients remain uniquely attached after restart and next map.

## Defect handling

Record findings for GP4-03, GP4-04, or GP4-05; do not patch production code.

## Status

FINAL-HASH SUPPORTED MODE GATES PASS.

The current 2p, 3p, and 4p ledgers each contain exactly nine stock-mode rows.
Their FFA, Holocron, and Jedi Master rows provide nine relevant
player-count/mode manifests; every manifest is hash-pinned to client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`
and records `status: passed`.

Evidence:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`
- Verifier:
  `tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`

This promotes the implemented final supported-mode contracts. It does not
invent unasserted holder/pickup/role permutations, remote-fifth behavior, or
Phase 6 stress coverage beyond those manifests.
