# W2-02: Orchestrate Vanilla Multi-Client Joins

## Objective

Connect Players 2-4 to Player 1's selected server through the vanilla protocol,
using independent client state inside one process.

## Ownership

- Split-network client lifecycle, sockets, packet routing, and reconnect logic.
- Do not edit browser UI, credential prompts, or profile editors.

## Work

1. Replace timing-only connect fan-out with challenge/connect state transitions.
2. Give each local client independent sequence, reliable command, download,
   gamestate, snapshot, and timeout state.
3. Route packets by socket/address/client context without cross-client parsing.
4. Follow Player 1 through map changes and server-directed reconnects.
5. Disconnect all party members coherently when leaving the server.
6. Reject attempts to send local players to different servers.

## Acceptance criteria

- 2-4 clients connect to an unmodified server from one process.
- No server-side patch, custom command, or custom protocol field is required.
- Packet loss or delayed challenge for one client does not corrupt another.
- Map changes preserve all accepted local clients.

## Verification

- Vanilla dedicated-server integration tests with 2, 3, and 4 local clients.
- Injected latency, loss, reordering, delayed challenge, and one-client timeout.
- Long map-cycle run with per-client state assertions.
