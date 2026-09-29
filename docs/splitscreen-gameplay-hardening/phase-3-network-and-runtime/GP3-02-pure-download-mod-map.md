# GP3-02: Pure Servers, Downloads, Mods, and Map Transitions

## Objective

Validate secondary-client handshake, checksums, content acquisition, VM reload,
and state restoration across real compatibility boundaries.

## Exclusive ownership

- `tests/splitscreen/gameplay/network/compatibility/**`
- Evidence under `tests/splitscreen/gameplay/results/network/compatibility/**`

## Scenarios

- Join controlled stock pure/non-pure servers with native and QVM modules.
- Exercise present content, missing-pak download success, cancel, corrupt hash,
  unavailable download, insufficient-space simulation, and clean retry.
- Test base game plus selected popular mod/fs_game configurations without
  modifying third-party servers.
- Rotate across maps and modes, use `map_restart`, reload cgame/ui/game VMs, and
  verify 2/3/4 clients before and after every transition.

## Acceptance

- Pure checksum commands precede first usermove for every local client.
- Downloads are singular, hash-verified, recoverable, and do not cross homepaths.
- Unique slots, userinfo, device routing, UI, and live state survive map/VM
  transitions with no stale snapshot or duplicate pane.

## Defect handling

Record findings for GP4-03 or GP4-05; do not patch production code.

## Status

DISCOVERY COMPLETE — ACCEPTANCE NOT MET on 2026-07-23.

- Six frozen-native stock cases covered `sv_pure=0` and `sv_pure=1` with
  2/3/4 local clients. All latest Phase 0 manifests record process-level
  `status=passed` and pass validate-only replay.
- The evidence matrix contains 30 narrow assertion-backed passes, 6 screenshot
  smoke observations, and 14 explicitly unsupported compatibility cells.
- Serverinfo matched the requested pure mode. Every client emitted a checksum
  command, and each secondary checksum diagnostic preceded its corresponding
  sent marker. No first-usermove oracle exists, so checksum-before-usermove is
  not claimed.
- All clients were `ALIVE` with expected client numbers initially, after
  `map_restart`, and after a stock `mp/ffa3` to `mp/ffa2` transition.
- QVM modules, downloadable-pak success/cancel/corruption/unavailability,
  insufficient-space recovery, third-party mods, external product/protocol
  endpoints, authoritative VM-reload ownership, per-pane userinfo/device/UI
  ownership, and stale/duplicate-pane detection remain
  `BLOCKED_UNSUPPORTED` because controlled fixtures are unavailable.
- This discovery certifies evidence honesty, not GP3-02 product acceptance. No
  production source changed and no rebuild was performed.

Evidence:

- `tests/splitscreen/gameplay/results/network/compatibility/matrix.json`
- `tests/splitscreen/gameplay/results/network/compatibility/findings.md`
