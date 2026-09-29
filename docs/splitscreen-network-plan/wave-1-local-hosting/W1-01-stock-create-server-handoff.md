# W1-01: Hand Off to Stock Create Server UI

## Objective

Replace the simplified local-match launch with a transition from split-screen
party setup into the unchanged stock `createserver` menu.

## Ownership

- Split-screen setup menu actions and UI transition glue.
- Do not edit server startup, local-client attachment, or socket code.

## Work

1. Retain only player count, mutually exclusive input assignment, and play-type
   selection on the split-screen party screen.
2. On Local Match, persist party state and open stock `createserver`.
3. Remove split-specific map and game-type controls from the setup screen.
4. Preserve normal stock Back, Advanced, map feeder, bot, and game-type actions.
5. Define cancellation: Back returns to party setup with assignments intact.
6. Ensure entering Create Server normally, without split screen, is unchanged.

## Acceptance criteria

- Local Match visibly opens the same Create Server menu as normal multiplayer.
- Every stock game type and compatible map remains selectable.
- No cloned or homegrown host controls are introduced.
- Returning to setup does not reset player count or device assignment.

## Verification

- Compare screenshots and menu identifiers for normal and split entry paths.
- Navigate every stock host submenu with keyboard and controller-owned Player 1.
- Assert no server starts until the stock Begin action is activated.
