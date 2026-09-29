# GP6-03: Performance, Memory, and Sanitizer Certification

## Objective

Measure representative 1/2/3/4-player cost and detect memory, lifetime, bounds,
thread, and undefined-behavior failures.

## Exclusive ownership

- `tests/splitscreen/gameplay/stress/performance/**`
- `tests/splitscreen/gameplay/stress/sanitizers/**`
- Evidence under `tests/splitscreen/gameplay/results/stress/performance/**`

No production changes are allowed.

## Matrix

- Supported resolutions/layouts on representative maps with dense combat,
  sabers, Force effects, objectives, UI overlays, public-like latency, and sound.
- Capture CPU/GPU frame distributions, pacing, RSS/allocations, network traffic,
  loading/restart latency, and 1-player regression.
- Run ASan/UBSan/LSan or platform equivalents where supported through setup,
  gameplay, death/respawn, map change, hotplug, reconnect, and quit.

## Acceptance

- Budgets are set before measurement and cannot be relaxed after seeing results.
- No sanitizer finding, unbounded resource slope, or unexplained 1-player
  regression remains.
- Reports include distributions and traces, not only averages.

## Status

PLANNED.
