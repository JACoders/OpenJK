# P6-01: Meet Performance Budgets

## Objective

Profile and optimize the validated implementation without changing public
behavior or contracts.

## Primary ownership

Performance instrumentation, benchmark configs, and isolated optimizations in
hot rendering/network paths. Do not edit packaging, docs, or release manifests.

## Work

1. Measure CPU/GPU frame time, memory, allocations, snapshots, effects, and audio
   for 1/2/3/4 players on representative stock maps.
2. Identify repeated per-viewport work that is contractually shared once.
3. Optimize only measured hotspots and preserve visual output.
4. Rerun performance and visual regression baselines after each change.

## Acceptance criteria

- Frozen frame-time, memory, and resource budgets pass for all player counts.
- No optimization changes screenshot-oracle results or input/network behavior.
- One-player performance has no material regression.
- Profiling evidence and before/after measurements are archived.
