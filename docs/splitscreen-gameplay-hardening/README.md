# Split-Screen End-to-End Gameplay Hardening Plan

This plan turns the existing split-screen implementation into a gameplay-tested
release candidate. A case is not covered merely because clients connect, spawn,
or produce input. Coverage requires a complete player journey and a definitive
gameplay outcome.

## Execution rules

1. Phases run in order. Every ticket in one phase may run in parallel.
2. A phase starts only from the exact build and evidence manifest frozen by the
   previous phase.
3. Same-phase tickets own disjoint source, test, port, homepath, and evidence
   paths. They may not patch another ticket's ownership area.
4. Discovery phases add tests and evidence only. Production fixes are deferred
   to the component-owned repair lanes in Phase 4.
5. Validation phases never patch production code. A failure reopens Phase 4,
   produces a new regression, rebuilds, and restarts certification from Phase 5.
6. Every defect requires a deterministic reproduction, a production fix, a
   regression that fails without the fix, and before/after evidence.
7. Direct model, Force, saber, team, or connection cvar shortcuts may prepare a
   narrow unit probe, but they cannot satisfy an end-to-end acceptance case.
8. Public tests may join and play, but may not change or burden third-party
   server administration. Rejection and impairment scenarios use controlled
   servers.

## What “end to end” means

Unless a ticket explicitly tests an error path, each player-count scenario must:

1. Start from a clean homepath at the main menu.
2. Select split screen, player count, input devices, names, models, sabers, and
   Force profiles through visible UI owned by the assigned device.
3. Host or join through the stock UI.
4. Prove unique client slots and live player states.
5. Move, look, attack, use a Force power, and interact with the mode objective
   using the assigned device without input or cursor bleed.
6. Cause and observe scoring, death, respawn or round turnover.
7. Reach the mode's real terminal state: frag/capture/objective/win limit,
   intermission, and post-intermission restart or next map.
8. Open and close applicable pause, scoreboard, chat, and console surfaces.
9. Exit cleanly with no ghost clients, stuck inputs, leaked sockets, or crash.

## Phases

| Phase | Goal | Parallel tickets |
| --- | --- | ---: |
| [0](phase-0-foundation/README.md) | Establish an executable coverage ledger, isolated runners, and trustworthy oracles | 3 |
| [1](phase-1-ui-and-devices/README.md) | Close fresh-home UI, ownership, persistence, and device-lifecycle gaps | 4 |
| [2](phase-2-game-modes/README.md) | Play every stock multiplayer mode through its real lifecycle | 5 |
| [3](phase-3-network-and-runtime/README.md) | Exercise hosting, Internet compatibility, recovery, impairment, and audio runtime | 5 |
| [4](phase-4-repair/README.md) | Repair every reproducible defect in component-exclusive lanes | 5 |
| [5](phase-5-frozen-certification/README.md) | Replay complete 2-, 3-, and 4-player journeys on one frozen build | 4 |
| [6](phase-6-stress-and-compatibility/README.md) | Certify physical devices, soak, performance, and packaged compatibility | 4 |
| [7](phase-7-independent-audit/README.md) | Independently audit coverage/evidence and source/build integrity | 2 |
| [8](phase-8-seal/README.md) | Seal only a zero-open-defect, fully traceable release candidate | 1 |

## Mandatory coverage matrix

The generated ledger must distinguish `setup`, `spawn`, `input`, `combat`,
`score`, `death`, `respawn`, `objective`, `spectate/rejoin`, `intermission`,
`restart`, `next map`, and `clean exit`; “smoke” may never be reported as
“end-to-end”.

| Mode | 2 players | 3 players | 4 players |
| --- | --- | --- | --- |
| FFA | required | required | required |
| Holocron | required | required | required |
| Jedi Master | required | required | required |
| Duel | active duel plus queue/capacity behavior | queue behavior | queue behavior |
| Power Duel | capacity/error behavior | required 1v2 lifecycle | queue/capacity behavior |
| Team FFA | required | required | required |
| Siege | supported-count behavior | supported-count behavior | required lifecycle |
| CTF | required | required | required |
| CTY | required | required | required |

## Non-negotiable invariants

- Keyboard and mouse affect only their assigned player and pane.
- The cursor cannot enter, click, or focus another player's pane.
- Each controller affects only its assigned player, including disconnect,
  reconnect, held-input, and remap transitions.
- Every local network client has a unique client number, qport, snapshot,
  reliable-command stream, userinfo, and teardown.
- One player's menu, chat, console, scoreboard, death, spectate, download, or
  connection failure cannot corrupt another player's state.
- Viewport, scissor, UI, cgame, renderer, audio, and input state are restored
  between players.
- A map load, restart, intermission, VM restart, video/input restart, or process
  relaunch cannot leak stale per-player state.
- No reproducible split-screen defect of any severity remains open at seal.

## Defect loop

Phases 1–3 create immutable reproductions in the defect ledger. Phase 4 assigns
each defect to exactly one source-owner lane. Phases 5–7 may only pass or reopen
Phase 4; they do not waive failures or patch around them. Phase 8 requires every
ledger entry to be `fixed-and-replayed` or `invalid-with-reproduction-proof`.
