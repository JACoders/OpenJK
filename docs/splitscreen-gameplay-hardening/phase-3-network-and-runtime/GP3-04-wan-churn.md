# GP3-04: WAN Impairment and Slot Churn

## Objective

Exercise asymmetric latency, jitter, loss, reordering, outages, and repeated
per-player leave/rejoin without corrupting the party.

## Exclusive ownership

- `tests/splitscreen/gameplay/network/wan_churn/**`
- Evidence under `tests/splitscreen/gameplay/results/network/wan_churn/**`

## Scenarios

- Apply deterministic latency, jitter, loss, duplication, reordering, and
  bandwidth limits to all clients and to one secondary at a time.
- Interrupt connectivity during handshake, pure validation, active combat,
  intermission, download, and map change; restore it within and beyond timeout.
- Perform at least 100 leave/rejoin cycles in varying P1–P4 orders.
- Include partial party failure, remote fifth client, server restart, and final
  all-player quit.

## Acceptance

- Input never reroutes during packet loss or reconnect.
- Recovered players receive fresh unique slots/snapshots and correct profiles.
- No reliable-command corruption, ghost client, socket/FD growth, or crash.
- Recovery time and snapshot-gap budgets are recorded, not inferred.

## Defect handling

Record findings for GP4-02 or GP4-03; do not patch production code.

## Status

DISCOVERY COMPLETE — ACCEPTANCE NOT MET.

The frozen native client was exercised against three isolated controlled
dedicated servers with normal Phase 0 manifests. The 2-player and 3-player
cases completed all 12 secondary leave/rejoin cycles without an assertion
failure and ended with the party active. The 4-player case completed all 12
cycles and ended active, but nine cycle cells observed a secondary remaining
`SPECTATOR` while the cell required every local player to be `ALIVE`. The
failures form two persistent recovery episodes rather than nine independent
root causes: P2 first became stuck after cycle 1 and recovered in cycle 8; P3
again remained spectator after cycle 12.

The discovery matrix contains 30 narrow passes, 9 failed cycle cells, 18
unsupported impairment cells, and 13 not-covered instrumentation/stress cells.
It does not claim the required 100 cycles, P1 reconnect, packet impairment,
input non-rerouting during loss, fresh snapshot identity, recovery-time or
snapshot-gap budgets, FD/socket growth, a fifth client, or server restart.
Those remain explicit Phase 6 work.

Evidence and validation:

- `tests/splitscreen/gameplay/results/network/wan_churn/matrix.json`
- `tests/splitscreen/gameplay/results/network/wan_churn/findings.json`
- `tests/splitscreen/gameplay/network/wan_churn/validate.py`
