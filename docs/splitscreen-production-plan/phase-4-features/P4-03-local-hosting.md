# P4-03: Complete Stock Local Hosting

## Objective

Start split-screen matches through the stock Create Server experience and allow
remote machines to join the same server.

## Primary ownership

Dedicated `ui_split_host.*` and server-side split-capacity modules implementing
the P1-06 hooks. Do not edit `ui_main.c`, browser/remote-connect, input, profile,
or rendering modules.

## Work

1. Reuse all stock game types, maps, limits, bots, passwords, and advanced rules.
2. Reserve server slots for 2/3/4 local clients without hiding remote capacity.
3. Support a fifth or later player from another machine.
4. Handle join/spectate/team, death/respawn, bot add/remove, map rotation, restart,
   shutdown, and return-to-menu for every local client.

## Acceptance criteria

- Stock Create Server settings produce the requested split-screen match.
- Remote vanilla OpenJK clients can discover/connect and play normally.
- Every local slot follows normal lifecycle rules in all supported game types.
- Hosting one-player multiplayer remains unchanged.
