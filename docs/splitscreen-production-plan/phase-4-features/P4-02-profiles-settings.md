# P4-02: Complete Per-Player Profiles and Settings

## Objective

Let every local player independently configure and apply all personal state from
stock multiplayer screens inside that player's viewport.

## Primary ownership

Dedicated `ui_split_profile.*`, `cl_split_cvar.*`, console, and virtual-keyboard
provider modules. Do not edit `ui_main.c`, input transport, host/browser,
network protocol, or rendering composition.

## Work

1. Reuse stock character, name, team, saber, color, and Force-power menus.
2. Persist separate profile and userinfo values for slots 1-4.
3. Provide per-player Quake consoles with command/CVAR and cheat keyboards.
4. Correct virtual-keyboard navigation, focus, text-field placement, centering,
   validation, cancellation, and controller-only operation.

## Acceptance criteria

- Each slot can change name, model, saber, Force powers, and CVARs independently.
- Changes reach that slot's server userinfo and survive reconnect/restart.
- Console history, autocomplete, modal state, and text never cross slots.
- Every workflow is completable without a physical keyboard.
