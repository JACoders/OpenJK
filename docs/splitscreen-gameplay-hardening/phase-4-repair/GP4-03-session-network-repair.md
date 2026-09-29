# GP4-03: Session, Network, and VM-Transition Repairs

## Objective

Fix defects in split-client connection state, handshakes, snapshots, reliable
commands, errors, downloads, reconnect, teardown, and map/VM transitions.

## Exclusive ownership

- `codemp/client/cl_main.cpp`
- `codemp/client/cl_cgame.cpp`
- `codemp/client/cl_cgameapi.cpp`
- `codemp/client/client.h`
- `tests/splitscreen/gameplay/regressions/session/**`

This ticket does not edit input, UI, game, cgame module, renderer, or audio
source.

## Work

- Add protocol/state regressions for every assigned defect.
- Repair per-client connection state, qports, pure ordering, download ownership,
  error/retry isolation, slot uniqueness, reconnect, reliable disconnect, and
  map/VM restoration.
- Preserve healthy local clients when one secondary fails.

## Acceptance

- Every assigned reproduction fails before and passes after the patch.
- No case can report active without unique valid slots and live snapshots.
- Rejection, timeout, shutdown, map change, restart, and quit leave no ghost
  client, stale VM context, socket, fragment, or reliable command.

## Evidence

Attach packet/state timelines, server occupancy, before/after logs, and resource
cleanup data.

## Status

SOURCE REPAIRS COMPLETE — INTEGRATED RUNTIME VERIFICATION PENDING.

### Repaired findings

- Phase 2 CTF/CTY 3p/4p team attachment: `splitnet_cmd` and
  `splitnet_rejoin` extracted the unmodified command text with
  `Com_SkipTokens`. Commands later in a `vstr` expansion retained their player
  number (`3 team red`, `4 team blue`) and the server ignored them. Both paths
  now use the tokenized tail beginning at argument 2.
- GP3-03 whole-party reconnect at 2p/3p/4p: an immediate primary reconnect
  reused the retiring same-qport server slot and consumed queued fragments,
  producing `CL_ParseServerMessage: Illegible server message` before the party
  remained `connecting`. Split-party reconnects to the endpoint just torn down
  now stay ordered in the command buffer and observe a three-real-second
  reconnect grace period before resetting the primary connection.
- Phase 0 controlled-server support teardown: the runner now records the
  intentional support-process signal without treating it as the authoritative
  client result.

The GP3-03 four-player same-IP rejection was traced to authoritative admission
counting of a stale disconnected slot and is repaired by GP4-04 in
`codemp/game/g_client.c`. Packet impairment, download ownership, external
server policy, and physical network-loss cases remain unsupported rather than
closed.

### Regression

`tests/splitscreen/gameplay/regressions/session/run.sh` checks five source
contracts: token-tail routing, 2/3/4 command reconstruction, live-party
disconnect capture, pre-reset reconnect deferral, and endpoint/party scoping.
The pre-fix frozen logs remain under
`tests/splitscreen/gameplay/results/network/recovery/`; Phase 5 must supply the
post-build 2/3/4 runtime timeline.
