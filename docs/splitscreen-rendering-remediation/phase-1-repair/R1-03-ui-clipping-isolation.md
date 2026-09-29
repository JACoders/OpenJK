# R1-03: Stock UI Clipping Isolation

## Ownership

`codemp/ui/*`, UI-facing renderer syscall wiring if required, and menu tests.

## Work

Pair stock-menu coordinate transforms with real scissor clipping, use a scoped
push/pop model, and restore full-screen UI state after every viewport.

## Acceptance

- Top menu, profile, controls, saber, Force, console, and OSK stay in-pane.
- Nested/modal menus cannot inherit another player's transform or clip.
- Ordinary full-screen menus remain pixel-compatible.
