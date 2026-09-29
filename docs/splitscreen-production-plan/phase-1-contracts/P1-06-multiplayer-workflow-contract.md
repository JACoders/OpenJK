# P1-06: Multiplayer Workflow Boundary Contract

## Objective

Define separate integration boundaries for stock local hosting and stock server
browsing so both implementations can proceed without editing the same files.

## Primary ownership

New declarations and design notes in multiplayer UI/client public headers. No
runtime implementation.

## Contract contents

- Party descriptor containing local slots, profiles, devices, and readiness.
- Stock Create Server handoff and local-host lifecycle callbacks.
- Stock server-browser handoff, password flow, connect, cancel, and reconnect.
- Capacity accounting for local clients plus remote clients.
- Join, spectate, team, respawn, map-change, and failure notifications.

## Acceptance criteria

- Hosting and joining implementations have disjoint registration modules.
- A vanilla OpenJK server needs no split-screen-specific protocol extension.
- A local host can accept a fifth player from another machine.
- Single-client stock host/browser callers remain source-compatible.
