# Split-Screen Network Implementation Plan

> Superseded by the renderer-first
> [Split-Screen Production Plan](../splitscreen-production-plan/README.md).
> Retained only as historical network-workflow detail.

This plan connects split-screen setup to the stock Jedi Academy server creation
and server browser workflows. It covers hosting a network-visible game with two
to four local players and joining a vanilla OpenJK internet server with the same
local party, all inside one game process.

## Execution rules

- Complete waves in order. Every ticket in a wave may run in parallel.
- A ticket may depend on completed earlier waves, but never on another ticket in
  its own wave.
- Agents must not edit files owned by another ticket in the same wave unless the
  wave README explicitly permits it.
- Keep stock menus stock. Split-screen code may prepare state before opening a
  stock menu and react to its result, but should not clone its controls.
- Player 1 and Players 2-4 must appear to a vanilla server as ordinary clients.
- All app-level tests run sequentially because OpenJK windows and virtual
  controllers share focus and simulator ports.
- ARM64 macOS is the primary build and test target. Do not substitute x86_64.

## Waves

| Wave | Goal | Exit gate |
| --- | --- | --- |
| [0](wave-0-foundations/README.md) | Establish versioned assets, lifecycle contracts, and isolated QA infrastructure | Contracts and harnesses build without changing gameplay |
| [1](wave-1-local-hosting/README.md) | Host through the stock Create Server workflow | 2-4 local clients can host a configurable LAN game |
| [2](wave-2-internet-joining/README.md) | Join through the stock server browser | 2-4 local clients can join one vanilla server reliably |
| [3](wave-3-system-validation/README.md) | Exercise complete local and internet workflows | Functional, visual, controller, and failure matrices pass |
| [4](wave-4-production-readiness/README.md) | Harden, document, package, and release | Release checklist passes with no critical defects |

## Global acceptance criteria

1. One OpenJK process owns all local viewports and input devices.
2. Input and UI focus never bleed between local players.
3. The stock Create Server and Join Server menus remain the authoritative UI.
4. Host settings, maps, bots, limits, passwords, and network visibility work as
   they do in unmodified multiplayer.
5. Split clients survive map changes, reconnects, death, team changes, and menu
   transitions.
6. A vanilla OpenJK server requires no split-screen-specific server changes.
7. Screenshots are stored under a path without spaces and visually inspected.
