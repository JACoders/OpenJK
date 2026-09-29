# GP1-01: Fresh-Home Profiles, Sabers, and Persistence

## Objective

Prove the complete visible customization journey and durable per-player state.

## Exclusive ownership

- `tests/splitscreen/gameplay/ui_profiles/**`
- Evidence under `tests/splitscreen/gameplay/results/ui_profiles/**`

## Scenarios

- From a fresh home, switch player counts `2 → 3 → 4 → 2`, exercise Back and
  Cancel, then complete setup without a blank or substituted transition frame.
- Assign distinct names, models, skins, Force alignments/powers, saber
  hilts/colors/styles, teams, and input devices through each pane's owner.
- Join gameplay and verify exact server userinfo and visible models/sabers.
- Reopen setup, change one player, and prove the other profiles are unchanged.
- Cleanly quit and relaunch the same home; verify persistence. Then run
  `vid_restart`, `in_restart`, map change, and a separate fresh-home default.

## Acceptance

- Every selection is visible before Apply/Join and matches live userinfo.
- P1 cannot edit P2–P4 and controllers cannot edit another pane.
- Persistence is per player, stable across relaunch/restarts, and absent from a
  truly fresh home.

## Defect handling

Record immutable reproductions only; route fixes to GP4-01 or GP4-02.

## Status

FAILED CERTIFICATION (2026-07-23).

- The stable rerun used binary SHA-256 `223e8a12...` from launch through
  validation. All three Phase 0 manifests and their 13/20/25 screenshots pass
  hash validation.
- 2p and 3p exited zero with 31/47 passing assertions and no failed assertions.
  The 4p process exited zero but contains 12 failed combat/lifecycle assertions:
  P1 dies instead of P2, scores zero, and the clients do not reach intermission.
- Full certification therefore fails. Immutable manifests, screenshots, the
  failing log, and ledger-ready defect `GP1-01-4P-001` are retained under
  `results/ui_profiles`.
- The current suite still lacks visible evidence for names, skins, sabers,
  teams, count switching, Back/Cancel, reopen isolation, and independent
  fresh-home defaults. No product defect or release claim is inferred from
  absent test coverage.
