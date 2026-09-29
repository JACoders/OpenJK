# P1-03: HUD and Menu-Context Contract

## Objective

Define how 2D drawing and stock menus resolve the active local player.

## Primary ownership

Declarations/design in `codemp/ui`, HUD-facing `codemp/cgame` headers, and no
runtime implementation.

## Contract contents

- Active local slot, client number, profile CVAR namespace, input owner, catcher,
  cursor/focus, console, virtual keyboard, HUD player state, score, vote/chat,
  and viewport clip/transform.
- Rules for shared server information versus per-player information.
- Modal ownership and simultaneous per-viewport menu behavior.

## Acceptance criteria

- HUD identity never comes from a global Player 1 CVAR by default.
- Stock menu reuse is possible without cloning menu contents.
- Exactly one owner receives each input event.
- The contract handles 2/3/4 layouts and controller-only text entry.
