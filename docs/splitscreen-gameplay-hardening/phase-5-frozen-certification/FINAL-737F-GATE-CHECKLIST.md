# Final `737f` Certification Gate Checklist

Audit snapshot: 2026-07-23 after freezing client SHA-256
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.

Historical results remain useful defect evidence, but a result from
`22d3fef…`, `73fae846…`, or any other client hash cannot pass a gate below.

## Frozen artifacts

| Gate | State | Evidence |
| --- | --- | --- |
| Ten frozen client/server/module/renderer artifacts | PASSED | [`frozen-build.json`](frozen-build.json); client `737fcd8c…7c13`, UI `28a7982e…a2c7` |

Every runtime promotion must contain the current hashes in an immutable
manifest or belong to a checksum-sealed aggregate that verifies them before
launch.

## Passed on the final hash

| Gate | State | Final-hash evidence |
| --- | --- | --- |
| Final supported-gate aggregate and verifier | PASSED | `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`; `tests/splitscreen/gameplay/certification/verify_final_supported_gates.py` |
| Exact 2p clean-home visible-UI journey, routed combat, Kyle Lightning win, frames, and clean exit | PASSED | `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/{base.tsv,result.json}` |
| Exact 3p clean-home visible-UI journey, routed combat, Kyle Lightning wins, intermission, frames, and clean exit | PASSED | `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/{base.tsv,result.json}` |
| Corrected 3p spectate/rejoin, restart, and next-map lifecycle | PASSED | `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/lifecycle.tsv` |
| 4p local/controlled/visible-UI aggregate | PASSED FOR ITS SEVEN COVERED CELLS | `tests/splitscreen/gameplay/results/certification/4p/{matrix.json,runs/20260723T231247Z/}` |
| 2p stock-mode matrix | PASSED 9/9 | `tests/splitscreen/gameplay/results/certification/2p/local-suite/final-737f-2p/modes.tsv` |
| 3p stock-mode matrix | PASSED 9/9 | `tests/splitscreen/gameplay/results/certification/3p/local-suite/final-737f-3p/modes.tsv` |
| 4p stock-mode matrix | PASSED 9/9 | `tests/splitscreen/gameplay/results/certification/4p/local-suite/final-737f-4p/{modes.tsv,modes-result.json}` |
| Mouse/focus 2p/3p/4p covered matrix | PASSED FOR COVERED CELLS | `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/matrix.json` |
| Modal ownership at 2p and 3p | PASSED | `tests/splitscreen/gameplay/results/certification/{2p/local-suite/final-737f-2p/modal.tsv,3p/local-suite/final-737f-3p/modal.tsv}` |
| Modal ownership at 4p | PASSED | `tests/splitscreen/gameplay/results/certification/4p/runs/20260723T231247Z/modal-report.json` |
| Virtual-controller ownership and neutral release across `in_restart` at 2p and 3p | PASSED | `tests/splitscreen/gameplay/results/certification/{2p/local-suite/final-737f-2p/virtual-controller.tsv,3p/local-suite/final-737f-3p/virtual-controller.tsv}` |
| Asymmetric secondary disconnect/rejoin at 2p and 3p | PASSED | `tests/splitscreen/gameplay/results/certification/{2p/local-suite/final-737f-2p/asymmetric-network-recovery.tsv,3p/local-suite/final-737f-3p/asymmetric-network-recovery.tsv}` |
| Controlled 2p/3p/4p runtime recovery | PASSED 21/21 SUPPORTED CELLS | `tests/splitscreen/gameplay/results/network/recovery/matrix.json` |
| 4p varied slot churn and independent fifth client | PASSED | `tests/splitscreen/gameplay/results/certification/4p/runs/20260723T231247Z/owned/` |
| Public 2p/3p/4p visible-UI parties with isolated devices and nonlocal humans | PASSED | `tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/` |
| Serialized public aggregate integrity | PASSED | `tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/{frozen-artifacts.log,summary.tsv,SHA256SUMS}` |

The exact local journeys select Kyle/Dark through keyboard/mouse and
Desann/Light through Controller 1; the 3p journey adds Reborn/Light through
Controller 2. The 4p aggregate adds Tavion/Light through Controller 3. The
public aggregate reports `PASS_WITH_NONLOCAL_HUMAN` for all three local counts
on `3.142.74.57:29070` and repeats unique-slot, live-state, visible-profile,
movement, and attack-isolation checks.

The mouse matrix contains 76 `COVERED_PASS`, zero `DISCOVERED_FAIL`, and
passing 2p/3p/4p manifests. Its 15 `BLOCKED` cells remain unresolved and are
not upgraded by the passing summary.

Each player-count mode ledger contains nine passing rows whose manifests
resolve to the frozen client hash. The recovery matrix contains 21
`COVERED_PASS` cells: baseline, secondary disconnect, secondary rejoin,
restart, map transition, whole-party reconnect, and controlled server-loss
recovery for each of 2p, 3p, and 4p. It records
`runtime_acceptance_met: true`; its broader `acceptance_met: false` remains
correct because 27 rejection-fixture cells are explicitly
`BLOCKED_NOT_COVERED`.

Running the final verifier produces:

```text
FINAL_SUPPORTED_GATES_PASS client=737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13 public=2p,3p,4p recovery=21/21 mouse=76/76
```

## Pending final-hash gates

- The 15 blocked mouse cells remain: mouse-only pause, fullscreen transition,
  visible invalid reassignment rejection, exhaustive adjacent-control clicks,
  2p/3p Saber entry, and 4p Force entry.
- Physical SDL controller unplug/replug, GUID-preserving enumeration reorder,
  persistent physical remapping, and simultaneous keyboard/mouse plus three
  physical controllers remain unsupported by virtual-controller evidence.
- The recovery matrix's 27 blocked cells remain unclaimed: wrong/correct
  password, full/reserved capacity, same-IP/duplicate admission,
  ban/kick/admin, protocol mismatch, pure rejection/recovery,
  timeout/interrupt, per-pane rejection reason/no-secret-leak, and occupancy
  baseline fixtures for each player count.
- A dedicated 4p virtual-controller `in_restart` recovery ledger is absent.
- Audio attribution, captured-audio, and resource oracles remain open.
- One-player/independent-client controls, the complete local/LAN and
  pure/module/map compatibility matrices, long WAN churn, and final visual
  evidence sealing remain Phase 5/6 gates unless separately promoted.

## Phase 6-only or unsupported gates

- Physical controller and audio-device removal/reopen.
- Deterministic packet impairment and controlled download
  cancel/corruption/space failures.
- QVM and selected third-party mod compatibility where fixtures are absent.
- Thirty-minute rendering/audio baseline, 100-cycle network soak, sanitizer and
  performance budgets, sleep/wake, and cold packaged-build compatibility.

Simulator or source-contract coverage does not promote these to physical or
runtime passes.

## Phase decision

- Supported final-hash runtime gates: **PASSED**. The 2p/3p/4p mode matrices,
  supported recovery matrix, mouse covered cells, local journeys, 4p
  aggregate, and public same-IP runs are stable.
- Phase 5 formal exit: **IN PROGRESS**. Unsupported physical-device,
  rejection-fixture, audio/resource, and remaining compatibility gates prevent
  a complete ticket or phase seal.
- Phase 6: **NOT COMPLETE / UNSUPPORTED CELLS OPEN**.
- Phase 7 independent audits: **NOT COMPLETE**.
- Phase 8 release seal: **NOT SEALED**.

The generated coverage ledger is still `legacy-unverified`: it guards the final
binary hash, but its imported records do not embed that hash. It must not be
used as final-build seal evidence until current manifests/logs/screenshots are
added as `frozen-verified` and the ledger is regenerated.
