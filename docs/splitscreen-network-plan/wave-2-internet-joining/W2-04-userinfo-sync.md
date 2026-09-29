# W2-04: Synchronize Per-Player Userinfo

## Objective

Maintain independent names, models, sabers, colors, teams, and Force settings
for every local network client before connection and during a match.

## Ownership

- Per-client userinfo/CVAR synchronization and server update commands.
- Do not edit profile UI, connection sockets, or browser transitions.

## Work

1. Define the authoritative per-player values and serialization boundaries.
2. Build each connect userinfo from that player's profile, not global CVAR state.
3. Route in-game profile, saber, Force, team, and name changes to the correct
   client connection.
4. Respect server restrictions and sanitize names/userinfo lengths.
5. Reapply accepted values after map changes without reverting other players.

## Acceptance criteria

- Four simultaneous clients may use four distinct profiles.
- Changing Player 3 cannot alter Player 1, 2, or 4 locally or server-side.
- Server-enforced changes are reflected only in the affected viewport.
- Userinfo remains valid for vanilla limits and escaping rules.

## Verification

- Server-side assertions at connect, after live changes, and after map change.
- Duplicate-name, maximum-length, color-code, and invalid-character cases.
- Profile changes from each player's controller-driven stock menu.
