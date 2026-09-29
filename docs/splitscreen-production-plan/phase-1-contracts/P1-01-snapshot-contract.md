# P1-01: Per-Client Snapshot Contract

## Objective

Define the engine-facing API that exposes a complete local client frame to cgame.

## Primary ownership

New declarations in `codemp/client`/shared public headers. No implementation.

## Contract contents

- Local slot and server client number.
- Connection and gamestate lifecycle.
- Current/next snapshot and interpolation fraction.
- Player state, areamask, server commands, configstring generation, time, ping,
  prediction input, and disconnect/error status.
- Read-only acquisition/release or context-switch lifetime rules.

## Acceptance criteria

- Supports local-command and vanilla-network clients through one abstraction.
- Does not expose mutable global `cl`, `clc`, or `cls` references to cgame.
- Defines behavior during connect, map load, missing snapshot, and disconnect.
- Header-level contract compiles without changing runtime behavior.
