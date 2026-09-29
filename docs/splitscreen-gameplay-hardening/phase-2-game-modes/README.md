# Phase 2: Complete Stock-Mode Gameplay

## Entry gate

Phase 1 proves that all scenarios can be entered and controlled through the
correct visible UI and assigned devices, or records deterministic defects.

## Parallel tickets

- [GP2-01: FFA, Holocron, and Jedi Master lifecycles](GP2-01-ffa-pickup-modes.md)
- [GP2-02: Duel and Power Duel rounds](GP2-02-duel-rounds.md)
- [GP2-03: Team FFA lifecycle](GP2-03-team-ffa.md)
- [GP2-04: CTF and CTY objectives](GP2-04-ctf-cty.md)
- [GP2-05: Siege objectives and rounds](GP2-05-siege.md)

## Independence rule

Each ticket owns a separate mode directory, server port block, homepath prefix,
and evidence tree. These tickets add tests and immutable findings only; they do
not edit shared production code.

## Exit gate

Every mandatory mode/player-count cell has either a full terminal-state proof
or a deterministic defect. Spawn-only and input-only evidence is reported as
smoke, never as end-to-end coverage.

## Final-hash supported-mode promotion

SUPPORTED MODE GATES PASS on client SHA-256
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The final 2p, 3p, and 4p ledgers each contain nine passing, digest-verified
stock-mode rows:

- `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/modes.tsv`

The cross-gate verification record is
`tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`;
its executable verifier is
`tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`.
This promotion covers the asserted terminal mode checks in those manifests. It
does not retroactively promote every historical queue, role, class, team,
objective permutation, or unsupported physical-device scenario.

## Historical discovery result

PASS FOR DISCOVERY on 2026-07-23 against frozen executable SHA-256
`223e8a126722a5728076fa1cbba2d4954f2cf51a849951c6af25a9ae25eabc33`.

- GP2-01 executed all nine FFA/Holocron/Jedi Master count combinations. All
  nine retain findings; proven transitions and smoke attempts are separated.
- GP2-02 records 6 narrow passes, 24 smoke observations, 3 failures, and 5
  unsupported Duel/Power Duel cells.
- GP2-03 records 15 narrow passes, 16 failures, 2 unsupported, and 12
  not-covered Team FFA cells.
- GP2-04 records 40 narrow passes, 8 failures, and 11 unsupported CTF/CTY
  cells. The staged two-player objective lifecycle reaches capture-limit
  intermission and identity-preserving restart in both modes.
- GP2-05 records 9 narrow passes, 9 failures, and 24 not-covered Siege cells.

Independent audit removed false respawn, round-win, class-change,
pane-uniqueness, and identity-preservation claims before sealing the phase.
Production acceptance is not claimed; reproducible findings are deferred to
the Phase 4 component lanes.
