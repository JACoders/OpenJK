# Phase 1: Frozen Cross-Module Contracts

## Entry gate

Phase 0 exit gate is met and its corpus, telemetry, image metrics, and audit are
available to all agents.

## Parallel tickets

- [P1-01: Per-client snapshot contract](P1-01-snapshot-contract.md)
- [P1-02: Cgame render-context contract](P1-02-render-context-contract.md)
- [P1-03: HUD and menu-context contract](P1-03-hud-menu-contract.md)
- [P1-04: Visual and functional acceptance contract](P1-04-acceptance-contract.md)
- [P1-05: Input and settings ownership contract](P1-05-input-settings-contract.md)
- [P1-06: Multiplayer workflow boundary contract](P1-06-multiplayer-workflow-contract.md)

## Exit gate

All interfaces needed by later phases are documented, represented as headers,
types, or fixtures, compile in isolation, and can be implemented without
same-phase agents modifying one another's ownership areas.
