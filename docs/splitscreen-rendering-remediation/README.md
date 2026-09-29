# Split-Screen Rendering Remediation Plan

This plan exists because the July 22 live test exposed rendering defects after
the broader production plan had been treated as complete. Completion now means
pixel evidence from the exact shipping renderer, not connection or log success.

## Execution rules

1. Phases run in order; every ticket in a phase may run in parallel.
2. Same-phase tickets have exclusive source ownership listed in the ticket.
3. Every fix adds an automated regression assertion and a real PNG capture.
4. A later phase consumes the frozen output of the previous phase.
5. One-player multiplayer is a mandatory control in every visual gate.

## Phases

| Phase | Goal | Parallel tickets |
| --- | --- | ---: |
| [0](phase-0-evidence/README.md) | Reproduce and classify every visible rendering failure | 3 |
| [1](phase-1-repair/README.md) | Repair disjoint 3D, 2D, and UI composition paths | 3 |
| [2](phase-2-certification/README.md) | Certify the integrated build across layouts and player counts | 3 |
| [3](phase-3-discovered-defects/README.md) | Close defects and coverage gaps found by certification | 5 |
| [4](phase-4-routed-game/README.md) | Repair the fresh-home UI route and exact two-player profile/gameplay path | 2 |
| [5](phase-5-acceptance/README.md) | Certify transition, device isolation, and the complete routed duel | 3 |
| [6](phase-6-public-internet/README.md) | Certify routed parties on independent public Internet servers | 3 |


## Global exit gate

- 2/3/4-player horizontal and vertical layouts contain no pixels outside their
  assigned viewport, no stale/duplicated scene, and no unexplained black area.
- HUD, console, top menu, profile, controls, saber, Force, scoreboard, death,
  spectate, and intermission UI remain clipped and legible per player.
- Scene, UI, scissor, color, and viewport state are restored between players.
- The automated suite and manually inspected contact sheet agree.
