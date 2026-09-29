# Phase 1: Parallel Rendering Repairs

## Entry gate

Phase 0 freezes viewport rectangles, render-state invariants, and image oracles.

## Parallel tickets

- [R1-01: Renderer viewport isolation](R1-01-renderer-viewport-isolation.md)
- [R1-02: Cgame HUD isolation](R1-02-cgame-hud-isolation.md)
- [R1-03: Stock UI clipping isolation](R1-03-ui-clipping-isolation.md)

## Exit gate

All three component suites pass independently and their source changes combine
without altering the frozen Phase 0 contract.
