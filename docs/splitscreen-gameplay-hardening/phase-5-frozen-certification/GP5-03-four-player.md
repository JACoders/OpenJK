# GP5-03: Four-Player Certification

## Objective

Certify the maximum four-player journey on the frozen build.

## Exclusive ownership

- `tests/splitscreen/gameplay/certification/4p/**`
- Evidence under `tests/splitscreen/gameplay/results/certification/4p/**`

No production changes are allowed.

## Matrix

- Fresh-home local host, controlled dedicated server with an independent fifth
  client, and a real public server accepting four same-IP clients.
- P1 keyboard/mouse and P2–P4 Controllers 1–3 with four distinct UI-selected
  profiles.
- Every mandatory stock mode with scoring/objective/round lifecycle.
- Simultaneous input, kill/respawn, modal ownership, intermission,
  restart/next map, network slot churn, and clean teardown.
- Physical controller removal/reconnect and complete media/resource oracles.

## Acceptance

- Four unique slots, qports, snapshots, command streams, userinfos, and panes
  remain valid.
- No keyboard, mouse, cursor, controller, modal, camera, effect, or sound bleed.
- All required state, media, and resource oracles pass on the frozen hashes.

## Final `737f` aggregate

The accepted local aggregate is
`tests/splitscreen/gameplay/results/certification/4p/runs/20260723T231247Z/`.
Its matrix is
`tests/splitscreen/gameplay/results/certification/4p/matrix.json` and records
seven `COVERED_PASS` cells with zero local failures:

- Local profile, attack, Force, and simultaneous-input assertions passed.
- Twenty-four profile assertions proved distinct Kyle, Desann, Reborn, and
  Tavion userinfo; the visible UI route selected Kyle/Dark and three
  Sith/Light profiles.
- The controlled server proved four live slots and secondary rejoin.
- The separate visible-UI runner reached its terminal success marker.
- Varied secondary slot churn passed.
- An independent controlled fifth client joined while all four local slots
  remained stable.
- Four-player per-pane modal ownership passed eight positive cases.

The local, controlled, and visible-UI manifests all record `status: passed`
against client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`
and UI module `28a7982e…a2c7`. The slot-churn and controlled-fifth Phase 0
manifests also pass.

The nine-row stock-mode ledger at
`tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/modes.tsv`
passes every mode cell with digest-verified manifests. Its aggregate result is
`tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/modes-result.json`.

The aggregate's own public cell was deliberately skipped because its runner
authorization gate was closed. That cell is superseded by the separately
serialized, checksum-sealed public run at
`tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/4p/`.
It joined `3.142.74.57:29070` with nonlocal humans present, occupied four
distinct live slots, repeated visible profile/Force selection, and passed
isolated movement and attack for keyboard/mouse and Controllers 1–3.

The recovery aggregate at
`tests/splitscreen/gameplay/results/network/recovery/matrix.json` passes
baseline, secondary disconnect/rejoin, restart, map transition, whole-party
reconnect, and controlled server-loss recovery for four players. The final
supported-gate aggregate and verifier are
`tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`
and
`tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`.

## Explicit limitations

- Physical SDL controller unplug/replug, GUID-preserving enumeration reorder,
  and persistent physical remapping remain unsupported.
- A dedicated four-player `in_restart` virtual-controller recovery ledger is
  not included in the supported aggregate and is not claimed.
- Audio attribution, captured-audio, and resource oracles remain open.
- Credential, capacity, protocol, pure, administrative, and packet-loss
  rejection fixtures remain `BLOCKED_NOT_COVERED`; the aggregate does not claim
  them.
- Fifteen mouse/focus cells remain blocked, and Phase 6 soak, sanitizer,
  performance, sleep/wake, and packaged-build work is not sealed.

The local matrix's `certification_passed: false` reflects its intentionally
skipped public cell. The separate final-hash public aggregate closes that
specific Internet proof. The final supported-gate verifier records
`runtime_acceptance_met: true`, while full rejection-matrix acceptance remains
false and the unsupported gates above prevent formal GP5-03 completion.

## Status

SUPPORTED RUNTIME GATES PASS ON FINAL HASH — the four-player
local/controlled/visible-UI aggregate, all nine stock-mode cells, modal
ownership, varied slot churn, independent fifth client, supported recovery,
and public four-player same-IP play all pass. Unsupported physical-device,
dedicated four-player input-restart, rejection-fixture, audio/resource, stress,
and packaging gates prevent a formal full-ticket or release seal.
