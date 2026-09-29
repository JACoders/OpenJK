# W4-01: Performance and Resource Budgets

## Objective

Set and meet measurable performance limits for four local clients plus remote
network traffic on representative Apple Silicon hardware.

## Work

1. Measure frame time, render submissions, prediction, packet processing, audio,
   memory, sockets, and input latency for 2-, 3-, and 4-player layouts.
2. Profile heavy combat, Force effects, bots, and dense maps.
3. Eliminate avoidable per-viewport duplication and unbounded queues.
4. Define quality defaults that preserve readability and responsive input.

## Acceptance criteria

- Published budgets include target hardware, resolution, and scenario.
- No sustained memory growth or reliable-command backlog occurs.
- Network processing for one client cannot starve other viewports.
- Any quality tradeoffs are explicit and user-configurable where practical.

## Deliverables

- Before/after profiles and a repeatable benchmark script.
- Performance regression thresholds suitable for release testing.
