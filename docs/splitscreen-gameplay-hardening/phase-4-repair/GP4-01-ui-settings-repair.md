# GP4-01: UI, Menu, Profile, and Settings Repairs

## Objective

Fix all defects whose authoritative owner is the split-screen UI workflow,
menus, profile editing, settings persistence, or UI hit-testing.

## Exclusive ownership

- `codemp/ui/**`
- `assets/splitscreen/base/ui/**`
- `tests/splitscreen/gameplay/regressions/ui/**`

This ticket does not edit client input, session/network, game, cgame, renderer,
or audio source.

## Work

- Convert each assigned ledger item into a minimal failing UI regression.
- Repair fresh-home transitions, Back/Cancel, count switching, modal focus,
  profile/Force/saber editing, persistence, error presentation, and UI clipping.
- Preserve visible-device routing; do not substitute helper or direct cvar paths
  for the failing user journey.

## Acceptance

- Every assigned reproduction fails on the pre-fix binary and passes after the
  patch.
- Unrelated player profiles and panes remain byte-for-byte/state-for-state
  unchanged.
- Existing menu, routed selection, rendering, and one-player controls pass.

## Evidence

Attach before/after screenshots, UI trace, saved-cvar diff, and regression log to
each closed ledger item.

## Implemented finding map

| Upstream finding | UI-owned defect | Repair | Regression |
| --- | --- | --- | --- |
| GP1-04 `results/modal_ownership/findings.md`: the logical top-menu owner is correct but its bar appears in other panes | `UI_PaintSplitScreenIngameMenus` painted the shared full-screen menu once for every pane | Paint the top-level overlay only inside `activeTarget`'s viewport; retain the owner's saved cursor and viewport push/pop | `ModalOwnershipTests.test_top_menu_is_painted_only_for_active_owner` |
| GP1-04 modal stock-menu path | `UI_PaintSplitScreenStockMenu` painted the top-menu background in every pane around a single-owner stock modal | Gate the background/top-menu paint on `player == activeTarget`; the stock modal was already owner-gated | `ModalOwnershipTests.test_stock_modal_top_bar_is_not_duplicated_into_other_panes` |
| GP1-04 clipping requirement | Nested menu drawing could escape a pane if a caller cleared the global transform | Preserve transforms with a bounded push/pop stack and clip pictures/text to the active pane | `ModalOwnershipTests.test_modal_paints_remain_viewport_scoped` |
| GP1-01 setup persistence requirements | Reopening the start menu reset count/session state, and raw menu writes could leave selection visuals inconsistent with cvars | Treat validated cvars as source of truth; refresh count/session presentation on open and route buttons through UI scripts | `SetupPersistenceTests` |
| GP1-01 profile selection requirements | Saving a stock portrait rebuilt the model from whichever custom-character state had most recently painted | Preserve the selected stock model/index and only rebuild custom-character models for a custom selection | Covered by source review here; runtime visible-UI profile matrix remains required in Phase 5 |
| Setup-to-stock menu transition | Split setup compositing could remain active after handing off to browser/create-server UI | Clear configuring/menu/input/pending setup state before the stock full-screen transition | Covered by source review here; runtime transition certification remains required in Phase 5 |

## Files changed

- `codemp/ui/ui_main.c`
- `codemp/ui/ui_atoms.c`
- `codemp/ui/ui_local.h`
- `assets/splitscreen/base/ui/jamp/splitscreen_start.menu`
- `assets/splitscreen/base/ui/jamp/splitscreen_players.menu`
- `tests/splitscreen/gameplay/regressions/ui/test_ui_repairs.py`

## Verification

Static/source verification was intentionally used because this ticket was
integrated without rebuilding:

```text
$ python3 tests/splitscreen/gameplay/regressions/ui/test_ui_repairs.py
.....
Ran 5 tests in 0.017s
OK

$ git diff --check -- codemp/ui assets/splitscreen/base/ui \
    tests/splitscreen/gameplay/regressions/ui
(no output)
```

The modal findings provide the pre-repair runtime evidence. New post-repair
screenshots, UI traces, and saved-cvar diffs require a rebuilt frozen binary and
are explicitly deferred to Phase 5. GP1-01's existing run did not exercise all
profile/persistence requirements, so this ticket does not misclassify those
unobserved cases as repaired runtime failures.

## Status

IMPLEMENTED — source/static verification complete; rebuilt-binary visual
certification remains a Phase 5 gate.
