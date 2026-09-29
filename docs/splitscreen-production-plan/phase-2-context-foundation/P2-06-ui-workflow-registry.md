# P2-06: Implement UI Workflow Registration

## Objective

Implement shared stock-menu and multiplayer workflow hooks once so Phase 4
features can live in disjoint modules.

## Primary ownership

Shared registration changes in `codemp/ui/ui_main.c` plus new
`codemp/ui/ui_split_registry.*`. Do not implement controls, profiles, hosting,
browsing, network connections, or rendering behavior.

## Work

1. Register separate controls, profile/settings, local-host, and server-join
   providers through the frozen Phase 1 contracts.
2. Pass active slot, viewport context, party descriptor, and stock menu result.
3. Keep provider lifecycle and cancellation deterministic across map/UI restarts.
4. Supply compile-only stub providers and registration tests.

## Acceptance criteria

- Each Phase 4 provider has an exclusive module and no need to edit `ui_main.c`.
- Missing providers fail explicitly without invoking another player's provider.
- Stock one-player menu paths remain unchanged when no provider is registered.
- All providers can be linked simultaneously without circular dependencies.
