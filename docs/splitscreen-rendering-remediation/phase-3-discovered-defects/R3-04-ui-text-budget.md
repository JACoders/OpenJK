# R3-04: Preserve UI Text Pixel Budgets

## Ownership

`codemp/ui/*` text painting and focused menu rendering tests.

## Acceptance

- The font renderer receives a pixel-width budget, never a character count.
- Full labels remain legible in 2/3/4-player top menus.
- Text remains bounded to the active pane without changing stock full-screen UI.
