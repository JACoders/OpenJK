# GP5-02: Three-Player Certification

## Objective

Certify a complete three-player customer journey on the frozen build.

## Exclusive ownership

- `tests/splitscreen/gameplay/certification/3p/**`
- Evidence under `tests/splitscreen/gameplay/results/certification/3p/**`

No production changes are allowed.

## Matrix

- Fresh-home local host, controlled dedicated server, and real public server.
- P1 keyboard/mouse and P2/P3 Controllers 1/2 with distinct UI-selected models,
  Force profiles, and sabers.
- Every mandatory stock mode, including full Power Duel 1v2 rounds.
- Terminal gameplay, spectate/rejoin, intermission, restart/next map, modal
  ownership, persistence, and clean exit.
- Controller hotplug/reorder and asymmetric P2/P3 network recovery.

## Acceptance

- All clients retain unique slots and correct userinfo.
- Device, cursor, HUD, camera, chat/console, and audio state remains
  player-correct.
- All required state, media, and resource oracles pass on the frozen hashes.

## Final `737f` evidence

The current client is
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The exact fresh-home visible-UI journey passed every base check and cleanly
exited:

- Main Menu → Multiplayer → Split Screen → 3 Players → player setup was
  captured from a clean home.
- Keyboard/mouse selected Kyle/Dark. Controllers 1 and 2 independently selected
  Desann/Light and Reborn/Light.
- Three clients became alive in distinct slots. P2 and P3 controller attacks
  remained isolated; Kyle's keyboard-routed Lightning killed both Sith players
  and reached the terminal intermission.
- All nineteen required frames are present and hash-indexed.

The corrected lifecycle run passed fourteen ordered assertions: initial
three-player life, P3 spectate/rejoin, P2 spectate/rejoin, all clients alive
after `map_restart`, and all clients alive after the next-map transition.

Evidence:

- Base ledger:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/base.tsv`
- Base result and frame hashes:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/result.json`
- Corrected lifecycle ledger:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/lifecycle.tsv`
- 3-player modal ownership:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modal.tsv`
- Virtual Controllers 1/2 before and after `in_restart`:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/virtual-controller.tsv`
- Secondary disconnect/rejoin with healthy identities preserved:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/asymmetric-network-recovery.tsv`
- Nine final-hash stock-mode cells:
  `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv`
- Checksum-sealed public run with nonlocal humans:
  `tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/3p/`
- Controlled whole-party and server-loss recovery:
  `tests/splitscreen/gameplay/results/network/recovery/matrix.json`
- Final supported-gate aggregate and verifier:
  `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`
  and
  `tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`

The modal, virtual-controller, and asymmetric-recovery reports all record
`passed: true`. The public run repeated visible Kyle/Dark, Desann/Light, and
Reborn/Light selection, occupied three distinct live slots, and passed isolated
movement and attack for keyboard/mouse and both controllers.

All nine final-hash mode rows pass with digest-verified manifests. The recovery
aggregate additionally passes baseline, secondary disconnect/rejoin, restart,
map transition, whole-party reconnect, and controlled server-loss recovery for
three players.

## Explicit limitations

- Physical SDL controller unplug/replug, GUID-preserving enumeration reorder,
  and persistent physical remapping remain unsupported.
- Explicit audio and resource oracles remain open.
- Credential, capacity, protocol, pure, administrative, and packet-loss
  rejection fixtures remain `BLOCKED_NOT_COVERED`; the aggregate does not claim
  them.
- Fifteen mouse/focus cells remain blocked, and Phase 6 soak, sanitizer,
  performance, sleep/wake, and packaged-build work is not sealed.

The exact-suite `result.json` remains `status: failed` because its suite-local
physical-hotplug and whole-party-loss booleans are false. Its mode errors are
empty, and the independently verified recovery aggregate supplies the
whole-party/server-loss runtime proof. The final supported-gate verifier records
`runtime_acceptance_met: true`, while full rejection-matrix acceptance remains
false.

## Status

SUPPORTED RUNTIME GATES PASS ON FINAL HASH — exact local visible-UI gameplay,
all nine stock-mode cells, corrected spectate/rejoin/restart/next-map
lifecycle, modal ownership, virtual-controller restart, supported recovery,
and public three-player play with nonlocal humans pass. Unsupported physical
device, rejection-fixture, audio/resource, stress, and packaging gates prevent
a formal full-ticket or release seal.
