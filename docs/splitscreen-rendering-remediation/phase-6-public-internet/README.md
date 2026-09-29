# Phase 6: Public Internet Acceptance

## Entry gate

Phase 5 passes on the frozen binary, including routed character selection,
per-device input isolation, and local four-player lifecycle coverage.

## Parallel tickets

- [R6-01: Two-player public party](R6-01-two-player-public-party.md)
- [R6-02: Three-player public party](R6-02-three-player-public-party.md)
- [R6-03: Four-player public capacity integrity](R6-03-four-player-public-capacity.md)

These tickets have disjoint evidence homes/configs and can run concurrently.

## Exit gate

- Two, three, and four local players occupy independent slots on real public
  Internet servers.
- Every player is alive before gameplay checks begin.
- Every assigned keyboard/mouse or controller path passes isolated movement and
  attack assertions.
- Four-player attempts never report success unless all four server client
  numbers are unique.

All conditions pass in one serialized run on `3.142.74.57:29070` against
frozen client SHA-256
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
Each 2-, 3-, and 4-player capacity snapshot contained nonlocal humans, and the
checksum-sealed aggregate is
`tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/`.
