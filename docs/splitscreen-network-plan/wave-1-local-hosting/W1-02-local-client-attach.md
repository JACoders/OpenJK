# W1-02: Attach Local Clients After Host Startup

## Objective

When the stock Start Server action succeeds, attach Players 2-4 to the embedded
server as distinct clients in the same process.

## Ownership

- Party lifecycle reaction to server startup and local client connection code.
- Do not edit stock menu assets or server option calculations.

## Work

1. Replace fixed `wait` chains with event/state-driven attachment.
2. Wait for the server and Player 1 to reach valid connection states.
3. Connect only configured local slots and apply each profile/userinfo atomically.
4. Assign initial teams according to game type without overriding server rules.
5. Handle partial failure with retry, cancel, and readable per-player errors.
6. Preserve clients across map restart, next map, and full map load.

## Acceptance criteria

- Exactly the requested local clients connect once; no duplicate ghosts appear.
- Names, models, sabers, colors, Force settings, and input owners remain distinct.
- Failed Player 3 attachment does not silently disconnect Players 1, 2, or 4.
- All attached clients respawn and rejoin according to normal game rules.

## Verification

- Automated 2-, 3-, and 4-player attach scenarios for FFA, Team, Duel, and CTF.
- Forced delayed startup and one injected client failure.
- Map restart and map-change assertions for every local slot.
