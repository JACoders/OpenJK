# Phase 0: Truthful Baseline

## Entry gate

Current branch builds on ARM64. No assumptions are made about feature correctness.

## Parallel tickets

- [P0-01: Capture rendering failure corpus](P0-01-render-failure-corpus.md)
- [P0-02: Add viewport telemetry](P0-02-viewport-telemetry.md)
- [P0-03: Build screenshot validation tooling](P0-03-screenshot-oracles.md)
- [P0-04: Audit existing architecture and tests](P0-04-architecture-audit.md)

## Exit gate

The broken 2/3/4-player output is reproducible, each viewport reports its actual
state, automated image checks detect known failures, and the architectural debt
and false-positive tests are documented without changing gameplay behavior.
