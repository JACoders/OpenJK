# GP5-01: Two-Player Certification

## Objective

Certify a complete two-player customer journey on the frozen build.

## Exclusive ownership

- `tests/splitscreen/gameplay/certification/2p/**`
- Evidence under `tests/splitscreen/gameplay/results/certification/2p/**`

No production changes are allowed.

## Matrix

- Fresh-home local host, controlled dedicated server, and real public server.
- Kyle/Dark via keyboard/mouse and a Sith/Light via Controller 1, selected only
  through visible UI.
- Every mandatory stock-mode lifecycle.
- Combat, terminal state, modal ownership, restart/recovery, and clean exit.
- Controller disconnect/reconnect and network secondary failure/recovery.

## Acceptance

- All required state, media, and resource oracles pass.
- Keyboard/mouse and cursor remain P1-only; Controller 1 remains P2-only.
- Public proof includes a nonlocal human; controlled networking remains the
  deterministic recovery gate.

## Final `737f` evidence

The current client is
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The exact fresh-home visible-UI journey passed every base check and cleanly
exited:

- Main Menu → Multiplayer → Split Screen → 2 Players → player setup was
  captured from a clean home.
- Keyboard/mouse selected P1 `kyle/default` with Dark profile
  `7-2-012320333000030321`.
- Controller 1 selected P2 `desann/default` with Light profile
  `7-1-332300000330003230`.
- Both clients became alive in distinct slots. Controller 1 damaged Kyle;
  Kyle's keyboard-routed Lightning killed P2 and awarded P1 score 1.
- All twelve required frames are present and hash-indexed.

Evidence:

- Base ledger:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/base.tsv`
- Base result and frame hashes:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/result.json`
- 2-player modal ownership:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modal.tsv`
- Virtual Controller 1 ownership before and after `in_restart`:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/virtual-controller.tsv`
- Secondary disconnect/rejoin with the healthy primary identity preserved:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/asymmetric-network-recovery.tsv`
- Nine-of-nine final stock-mode ledger:
  `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv`
- Checksum-sealed public run with nonlocal humans:
  `tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/2p/`
- Controlled whole-party and server-loss recovery:
  `tests/splitscreen/gameplay/results/network/recovery/matrix.json`
- Fail-closed supported-gate aggregate:
  `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`
- Aggregate verifier:
  `tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`

The modal, virtual-controller, and asymmetric-recovery reports all record
`passed: true`. The public run selected the same Kyle/Dark and Desann/Light
profiles through visible UI, occupied two distinct live slots, and passed
keyboard/mouse-versus-Controller-1 movement and attack isolation.

All nine required mode rows—FFA, Holocron, Jedi Master, Duel, Power Duel, CTF,
CTY, Team FFA, and Siege—point to hash-matching `status: passed` manifests.
The separate recovery matrix passes baseline, secondary leave/rejoin, restart,
map transition, P1-triggered whole-party reconnect, and authoritative
server-loss/recovery at 2p.

## Explicit limitations

- Physical SDL controller unplug/replug, GUID-preserving enumeration reorder,
  and persistent physical remapping are unsupported by the virtual bridge.
- Audio attribution, captured-audio, and resource oracles remain open.
- Credential, capacity, protocol, pure, admin, timeout, semantic-reason, and
  occupancy rejection fixtures remain explicitly not covered in GP3-03.
- Mouse cells classified `BLOCKED` and Phase 6 soak, sanitizer, performance,
  sleep/wake, and package gates are not promoted here.

`result.json` therefore remains `status: failed` even though its base journey
and supported mode/auxiliary ledgers are clean: its two suite-local false checks
are physical hotplug and whole-party/server-loss recovery. The fail-closed
aggregate keeps those exact limitations, then independently requires the
passing GP3-03 recovery matrix before reporting
`supported_runtime_gates_passed: true`.

## Status

SUPPORTED RUNTIME GATES PASS ON FINAL HASH — exact local visible-UI gameplay,
all nine supported stock-mode rows, modal ownership, virtual-controller
restart, asymmetric and whole-party/server-loss recovery, and public
two-player play with nonlocal humans are verified by the final aggregate.
Physical device, unsupported rejection, audio/resource, and Phase 6–8 gates
prevent a formal full-ticket or release seal.
