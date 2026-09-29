# GP6-02: Networked Lifecycle Soak

## Objective

Run sustained four-player real gameplay with network transitions, sound, and a
remote fifth client to expose time-dependent failures.

## Exclusive ownership

- `tests/splitscreen/gameplay/stress/network_soak/**`
- Evidence under `tests/splitscreen/gameplay/results/stress/network_soak/**`

No production changes are allowed.

## Matrix

- At least 120 minutes with renderer and sound enabled.
- Rotate FFA, Duel/Power Duel, Team FFA, CTF/CTY, and Siege maps.
- Repeated combat/objectives, death/respawn, spectate/rejoin, menus/modals,
  controller churn, network leave/rejoin, map/VM restart, and controlled WAN
  impairment.
- Periodic real screenshots plus continuous state/resource sampling.

## Acceptance

- No crash, hang, assertion, ghost client, duplicate slot, stuck input, stale
  pane, audio loss, or unrecovered network state.
- RSS, descriptors, sockets, threads, render/audio resources, and frame-time
  slopes remain within the committed budgets.
- Final all-player quit returns server and process resources to baseline.

## Status

PLANNED.
