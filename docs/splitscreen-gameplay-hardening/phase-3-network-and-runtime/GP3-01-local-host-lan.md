# GP3-01: Stock Local Hosting and LAN Play

## Objective

Prove that the visible Create Server workflow launches a real playable host for
2/3/4 local players and independent LAN clients.

## Exclusive ownership

- `tests/splitscreen/gameplay/network/local_host/**`
- Evidence under `tests/splitscreen/gameplay/results/network/local_host/**`

## Scenarios

- Route stock map, mode, limits, bots, password, dedicated/listen choice,
  public/LAN setting, and capacity through the host UI.
- Attach 2/3/4 local clients, then an independent LAN client through its browser.
- Complete representative FFA, team, and objective terminal-state journeys.
- Exercise host map change/restart, bot add/remove, password change, graceful
  shutdown, primary disconnect, and defined no-host-migration behavior.

## Acceptance

- Serverinfo exactly matches visible host choices.
- LAN discovery, capacity, password, client slots, and gameplay are correct.
- Shutdown removes all clients/listeners and leaves no stale advertised server.

## Defect handling

Record findings for GP4-01, GP4-03, or GP4-04; do not patch production code.

## Status

DISCOVERY COMPLETE — PARTIAL (2026-07-23).

- Frozen executable SHA-256 `223e8a1267...` ran three isolated listen-host
  cases with 2/3/4 local clients plus a separate one-player loopback process.
- All three Phase 0 manifests validate; both processes exit zero, assertion
  failure count is zero, and two hash-pinned screenshots are retained per case.
- Ordered runtime evidence proves all requested local clients alive after
  attach, the last controller's routed input, all local clients alive after
  `map_restart`, and the independent loopback client joining each listen host.
- Direct `devmap` hosting, configured FFA settings, process exit, and screenshot
  existence are smoke only. They do not prove the visible Create Server UI,
  exact serverinfo, browser-based LAN discovery, or listener cleanup.
- Team/objective terminal play, bot mutation, password mutation, primary
  disconnect, defined no-host-migration behavior, and a post-shutdown listener
  probe remain unsupported and unclaimed.
- Default discovery validation exits zero for this structurally honest partial
  matrix. `--require-acceptance` exits nonzero while unsupported acceptance
  cells remain.

Evidence:

- `tests/splitscreen/gameplay/results/network/local_host/current/matrix.json`
- `tests/splitscreen/gameplay/results/network/local_host/current/findings.md`
