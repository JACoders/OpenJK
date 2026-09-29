# GP2-04: CTF and CTY Objective Lifecycles

## Objective

Prove the complete flag/Ysalamiri objective lifecycle in 2/3/4-player splits.

## Exclusive ownership

- `tests/splitscreen/gameplay/modes/ctf_cty/**`
- Evidence under `tests/splitscreen/gameplay/results/modes/ctf_cty/**`

## Scenarios

- CTF: take, carry, drop on death, manual return, timed return, enemy pickup,
  capture, carrier respawn, team score, capturelimit, intermission, and restart.
- CTY: acquire and lose the Ysalamiri/objective state, prove Force restrictions,
  carrier death/transfer, score, intermission, and restart.
- Exercise uneven and balanced teams, friendly carrier interaction, spectate/
  rejoin, a remote fifth player, and a next-map rotation.

## Acceptance

- Objective owner, location, timer, Force restriction, capture, team score, and
  announcement are authoritative and pane-correct.
- A carrier's death or disconnect cannot duplicate or strand the objective.
- All clients preserve unique slots and recover correct HUD state after restart.

## Defect handling

Record findings for GP4-03, GP4-04, or GP4-05; do not patch production code.

## Status

FINAL-HASH SUPPORTED MODE GATES PASS.

CTF and CTY pass at 2p, 3p, and 4p: six relevant manifests record
`status: passed` against final client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.

Evidence:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/{modes.tsv,modes-result.json}`
- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`

The supported-gate promotion does not add direct objective-location/timer,
remote-fifth, Force-restriction, or other permutations not asserted by these
final manifests.
