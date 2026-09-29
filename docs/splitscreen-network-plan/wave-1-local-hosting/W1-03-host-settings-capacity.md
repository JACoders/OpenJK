# W1-03: Preserve Stock Server Settings and Capacity

## Objective

Respect every stock Create Server selection while guaranteeing enough client
slots for the local party and optional remote players.

## Ownership

- Host-setting validation and capacity calculation around Start Server.
- Do not edit menu layout, connection orchestration, or network advertising.

## Work

1. Inventory all stock settings consumed by `StartServer`, including map,
   gametype, limits, warmup, bots, dedicated mode, password, and Force options.
2. Compute minimum `sv_maxClients` from local players, configured bots, and a
   configurable remote-slot reserve.
3. Surface a validation error instead of silently dropping players or bots.
4. Prevent split hosting from selecting dedicated mode, since viewports require
   the local renderer/client process.
5. Leave normal non-split Start Server calculations unchanged.

## Acceptance criteria

- Stock-selected settings reach the server unchanged except documented capacity
  correction and forced non-dedicated mode.
- Four local players plus at least one reserved remote slot are supported.
- Duel and Power Duel limits remain valid.
- Bot allocation never consumes a slot reserved for a configured local player.

## Verification

- Table-driven tests across player counts, bot counts, and `sv_maxClients`.
- Assert serverinfo after startup for each supported game type.
- Test boundary values, invalid values, and full-capacity rejection.
