# W1-04: Make Hosted Games Network-Visible

## Objective

Allow a split-screen host to create a LAN-visible or internet-advertised listen
server that ordinary external clients can discover or connect to directly.

## Ownership

- Listen-server socket/advertising behavior and network diagnostics.
- Do not edit host UI or local split-client connection code.

## Work

1. Verify LAN heartbeat/discovery behavior for a listen server with split clients.
2. Preserve stock `dedicated`, `sv_pure`, password, master, and net-port semantics.
3. Provide useful bind, firewall, NAT, and master-server diagnostics.
4. Ensure split clients use distinct local sockets without occupying the public
   listen address incorrectly.
5. Verify direct connect remains available when discovery is blocked.

## Acceptance criteria

- A fifth vanilla client on another LAN machine can discover and join.
- Direct IP joining works when LAN discovery is unavailable.
- Remote clients see each local player as a normal scoreboard entry.
- The host can still run a private/passworded LAN match.

## Verification

- Two-machine LAN test with four local plus one remote player.
- Direct-connect test and LAN-browser test.
- Port collision, firewall denial, and wrong-password diagnostics.
