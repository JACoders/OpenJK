# P4-04: Complete Stock Internet Joining

## Objective

Join all configured local clients to one ordinary OpenJK server through the
stock browser without requiring server-side modifications.

## Primary ownership

Dedicated `ui_split_join.*` and client remote-connect modules implementing the
P1-06 hooks. Do not edit `ui_main.c`, local-host/server, input, profile, or
rendering modules.

## Work

1. Reuse stock LAN/internet/favorites browsers, refresh, filters, details, and
   password prompts.
2. Connect each local slot as an ordinary distinct network client.
3. Handle partial failure, cancellation, duplicate names, full servers, bans,
   downloads, timeouts, reconnect, map change, and server disconnect.
4. Keep the party tied to one server while preserving per-client userinfo.

## Acceptance criteria

- Two, three, and four local clients play on an unmodified OpenJK server.
- The server sees ordinary unique clients and no custom protocol dependency.
- Failure of one slot is reported and recovered without corrupting the others.
- Single-client stock browsing and connecting remain unchanged.
