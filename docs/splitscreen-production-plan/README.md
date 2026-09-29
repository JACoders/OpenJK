# Split-Screen Production Plan

This plan supersedes completion claims based only on connection-state tests. Its
first non-negotiable goal is to replace the current Player-1-derived rendering
approximation with independently correct client, camera, scene, HUD, and menu
state for every viewport in one OpenJK process.

## Execution model

- Phases execute in numerical order. A phase starts only after the previous
  phase's exit gate is met.
- All tickets within one phase are independent and may be assigned to separate
  agents simultaneously.
- Same-phase tickets have disjoint primary ownership. Shared interfaces are
  frozen by the preceding phase.
- Agents may consume earlier-phase interfaces but may not change them without a
  new integration ticket in a later phase.
- Every defect fix includes a regression test. Connection assertions alone are
  never evidence that a viewport rendered correctly.
- Gameplay and menu screenshots must come from OpenJK's `screenshot_png` command,
  be stored under a path without spaces, and be visually inspected.
- ARM64 Apple Silicon is the primary platform. The feature remains one process;
  launching multiple game instances is prohibited.

## Phases

| Phase | Purpose | Parallel tickets |
| --- | --- | ---: |
| [0](phase-0-baseline/README.md) | Establish truthful diagnostics and failure baselines | 4 |
| [1](phase-1-contracts/README.md) | Freeze client, render, HUD, input, workflow, and QA interfaces | 6 |
| [2](phase-2-context-foundation/README.md) | Implement independent per-player state and dispatch foundations | 6 |
| [3](phase-3-rendering/README.md) | Produce correct 3D, HUD, layout, and effects rendering | 4 |
| [4](phase-4-features/README.md) | Complete input, profiles, local hosting, and internet joining | 4 |
| [5](phase-5-validation/README.md) | Run independent functional and visual test matrices | 4 |
| [6](phase-6-release/README.md) | Harden performance, packaging, docs, and audit tooling | 4 |
| [7](phase-7-certification/README.md) | Certify one frozen production candidate | 4 |
| [8](phase-8-seal/README.md) | Seal the approved artifacts and manifest | 1 |

## Global definition of done

1. Two, three, and four players receive independent cameras, snapshots, HUDs,
   identities, menus, controls, profiles, consoles, and audio listeners/policies.
2. No viewport duplicates another player's frame or contains unexplained black
   regions, stale geometry, unclipped overlays, or another player's HUD data.
3. Local hosting uses the stock Create Server workflow and accepts remote players.
4. Internet joining uses the stock browser and works with an unmodified OpenJK
   server for every local player in the party.
5. Input never bleeds across players.
6. Death, respawn, spectate, join, team change, map change, reconnect, and server
   failure follow normal multiplayer behavior for every local client.
7. Cold start, single-player, and ordinary one-player multiplayer still work.

## Parallel-work rule

Each ticket names its exclusive source ownership. An agent may read any file but
may modify only the files assigned to its ticket. Shared-interface changes are
queued for the next phase instead of being negotiated between same-phase agents.
Validation tickets run against a frozen build and never patch production code.
