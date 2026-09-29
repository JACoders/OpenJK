# Phase 2: Independent Context Foundation

## Entry gate

All Phase 1 contracts are frozen and compile. Changes to those contracts require
a separately reviewed integration change after Phase 2 branches are complete.

## Parallel tickets

- [P2-01: Store independent client frames](P2-01-client-frame-storage.md)
- [P2-02: Implement cgame context switching](P2-02-cgame-context-switching.md)
- [P2-03: Implement viewport/scissor primitives](P2-03-viewport-primitives.md)
- [P2-04: Implement per-player 2D state adapters](P2-04-2d-state-adapters.md)
- [P2-05: Implement local-input dispatch](P2-05-input-dispatch.md)
- [P2-06: Implement UI workflow registration](P2-06-ui-workflow-registry.md)

## Exit gate

Four distinct client frames can be acquired, cgame can activate/restore isolated
contexts, renderer clipping is deterministic, HUD/menu code can query player-
specific state, input has one dispatch boundary, and feature modules have frozen
registration points. Rendering remains feature-minimal until Phase 3.
