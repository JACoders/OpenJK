# P5-04: Validate Network Faults and Endurance

## Objective

Validate local hosting, vanilla-server joining, failure recovery, and long-run
stability independently of the player-count gameplay matrices.

## Primary ownership

`tests/splitscreen/phase5/network-soak`, isolated servers/ports/homepaths, fault
profiles, and its result manifest only. Do not modify production code.

## Matrix

- Stock local host with 2/3/4 local clients plus remote fifth vanilla client.
- Stock browser joins to unmodified dedicated OpenJK servers with 2/3/4 clients.
- Password, full server, bad map, rejected slot, timeout, packet loss/latency,
  disconnect, reconnect, server restart, and map/download transitions.
- Four-hour mixed gameplay/menu soak and repeated 100-cycle connect/disconnect.
- Single-player and ordinary one-player multiplayer cold-start regressions.

## Acceptance criteria

- Remote servers observe standard independent clients with valid userinfo.
- Partial failures recover or fail clearly without contaminating healthy slots.
- No unbounded memory/file-descriptor growth, deadlock, crash, or stale process.
- All network, screenshot, regression, and teardown gates pass.
