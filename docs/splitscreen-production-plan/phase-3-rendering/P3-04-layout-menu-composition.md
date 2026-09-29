# P3-04: Compose Layouts and Stock Menus

## Objective

Compose 2/3/4 viewport layouts and render the stock top menu, player setup,
saber, Force, and modal UI independently inside each viewport.

## Primary ownership

Screen/layout composition in `codemp/client/cl_scrn.cpp` and stock-menu viewport
entry points in dedicated `codemp/ui` composition modules. Do not edit menu
content, HUD drawing, 3D scene generation, or input routing.

## Work

1. Implement horizontal, vertical, three-player, and four-player rectangles.
2. Apply viewport-relative coordinates, scissor, aspect correction, and restore.
3. Invoke stock menu rendering once per active menu context.
4. Correctly place avatars, spectator/join states, modal overlays, and virtual
   keyboard containers without cross-viewport overlap.

## Acceptance criteria

- Every pixel from a player's menu is clipped to that player's viewport.
- Stock menu contents are reused rather than recreated as custom substitutes.
- Text, borders, portraits, buttons, and keyboard labels are visually aligned.
- Opening or closing one menu does not hide or redraw another player's menu.
