# W2-01: Hand Off to Stock Server Browser

## Objective

Make Server Party open and use the unchanged stock Join Server interface while
preserving the configured local party.

## Ownership

- Split-screen setup-to-browser UI transition and cancellation behavior.
- Do not edit connection fan-out, password handling, or protocol code.

## Work

1. Persist player count and device ownership before opening `joinserver`.
2. Preserve stock internet/LAN/favorites filters, refresh, server info, sorting,
   direct-connect, password prompt, and Back actions.
3. Mark the selected address as the party target only when Join is confirmed.
4. Return to party setup with assignments intact when browsing is cancelled.
5. Keep normal one-player server browsing behavior unchanged.

## Acceptance criteria

- Server Party visibly opens the same browser as normal multiplayer.
- No custom server list or duplicate browser controls exist.
- Refreshing or inspecting servers does not prematurely connect extra clients.
- Cancelling clears pending target state but preserves party configuration.

## Verification

- Screenshot comparison for normal and party entry paths.
- Exercise internet, LAN, favorites, server info, refresh, and Back.
- Assert state transitions without requiring a successful connection.
