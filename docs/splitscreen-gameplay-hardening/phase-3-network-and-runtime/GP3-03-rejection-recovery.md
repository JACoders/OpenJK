# GP3-03: Rejection and Recovery Isolation

## Objective

Prove precise, pane-safe recovery from authentication, capacity, protocol, pure,
administrative, timeout, and server-loss failures.

## Exclusive ownership

- `tests/splitscreen/gameplay/network/recovery/**`
- Evidence under `tests/splitscreen/gameplay/results/network/recovery/**`

## Scenarios

- Password required/wrong/correct, full server, reserved slots, same-IP limit,
  duplicate slot, ban/kick, protocol/version mismatch, pure rejection, timeout,
  connection interrupted, and server shutdown.
- Fail P2, P3, or P4 during initial join while earlier players stay active.
- Cancel, retry, correct credentials/content, and rejoin only the failed player.
- Disconnect P1, recover the whole party, then quit all players cleanly.

## Acceptance

- Each pane displays the authoritative reason without leaking credentials.
- Healthy players remain alive and controllable during a secondary failure.
- Retry cannot duplicate slots/qports or reuse stale snapshots/reliable commands.
- Server occupancy returns to baseline after every cancellation or quit.

## Defect handling

Record findings for GP4-01 or GP4-03; do not patch production code.

## Status

FINAL-HASH SUPPORTED RUNTIME ACCEPTANCE MET; FULL REJECTION MATRIX NOT COVERED.

The current controlled-server matrix is
`tests/splitscreen/gameplay/results/network/recovery/matrix.json`. It records:

- `runtime_acceptance_met: true`
- 21 `COVERED_PASS`
- 27 `BLOCKED_NOT_COVERED`
- zero runtime failures or blocked prerequisites

At 2p, 3p, and 4p, seven supported runtime phases pass: baseline, secondary
disconnect, secondary rejoin, restart, map transition, P1-triggered whole-party
disconnect/reconnect, and authoritative server loss/restart/recovery. Ordered
live state and stable client numbers are asserted after recovery. The three
Phase 0 manifests record `status: passed`, exit code zero, final client hash
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`,
and matching manifest digests in the matrix.

The 27 blocked cells remain explicit—nine per player count:

- password required/wrong/correct;
- full server and reserved slots;
- authoritative same-IP/duplicate-slot rejection;
- ban/kick/admin reason;
- protocol/version mismatch;
- pure-content rejection and recovery;
- deterministic timeout/connection interruption;
- semantic per-pane reason with no secret leak;
- occupancy returning to baseline.

Accordingly, `runtime_acceptance_met` is true while the broader ticket field
`acceptance_met` remains false. Unsupported rejection fixtures are not waived
or reported as passes.

Aggregate evidence:

- `tests/splitscreen/gameplay/results/certification/final/final-supported-gates.json`
- `tests/splitscreen/gameplay/certification/verify_final_supported_gates.py`
