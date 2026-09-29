# GP4-05: Presentation, Renderer, and Audio Repairs

## Objective

Fix per-pane cgame/HUD/camera/effect defects plus renderer and audio state leaks
found by complete gameplay.

## Exclusive ownership

- `codemp/cgame/**`
- `codemp/rd-vanilla/**`
- `codemp/rd-rend2/**` when included by the frozen target
- `codemp/client/snd_*`
- `tests/splitscreen/gameplay/regressions/presentation/**`

This ticket does not edit input, UI, session/network, or game source.

## Work

- Add a minimal visual, camera, effect, or audio trace regression per defect.
- Repair HUD, scoreboard, spectate, objective, death, intermission, camera,
  Ghoul2/effects, viewport/scissor, listener, channel, and device-reset state.
- Restore all presentation state between local players and after VM/map restart.

## Acceptance

- Every assigned reproduction fails before and passes after the patch.
- Image/audio oracles prove pane uniqueness and correct player attribution.
- One-player output and performance remain within the Phase 0 baselines.

## Evidence

Attach before/after frames, media traces, renderer/audio diagnostics, and
resource counts.

## Implemented finding map

| Evidence | Classification | Repair or disposition | Regression |
| --- | --- | --- | --- |
| Phase 0 HUD audit: zoom, lagometer, crosshair, fixed-font glyphs and rotated primitives bypassed the split viewport boundary | Genuine presentation defect | Route axis-aligned HUD primitives through `CG_DrawPicUV`, including fixed-font glyphs; clip geometry and UVs; use a seam-safe representation for rotated split HUD artwork | `HudIsolationTests.test_axis_aligned_hud_primitives_use_clipped_draw_path`, `test_clipped_draw_path_clips_geometry_and_uvs` |
| Phase 0 HUD audit: renderer modulation could survive one local VM's HUD draw and tint the next pane | Genuine renderer-state leak | Reset renderer color before and after each split `CG_Draw2D` transaction and always clear the 2D transform afterward | `HudIsolationTests.test_hud_transaction_resets_color_and_transform` |
| Rendering scene contract and layout captures | Genuine geometry defect in the earlier implementation | Preserve remainder pixels on odd-sized framebuffers instead of rounding each pane down to an even dimension | `SceneOwnershipTests.test_cgame_partitions_cover_odd_frame_edges` |
| Renderer scene contract | Genuine ownership defect in the earlier implementation | Cgame owns the per-player refdef; vanilla renderer renders that submitted rectangle exactly once instead of subdividing it again from global `r_splitScreen` | `SceneOwnershipTests.test_renderer_does_not_split_partitioned_refdef_again` |
| Renderer failure marker from a model-less `RT_MODEL` submission | Genuine robustness defect | Drop the invalid refentity with one warning in all builds rather than relying on a debug-only model assertion | `SceneOwnershipTests.test_renderer_drops_invalid_model_without_debug_only_assert` |
| GP1-04 duplicated top-menu bar | UI-owned presentation composition | Fixed by GP4-01 in `codemp/ui`; no cgame/renderer duplicate fix is appropriate because this menu is submitted by UI | Reclassified to GP4-01 |
| GP3-05 `ORDERED_ALL_PANE_INPUT_COMMANDS`, including the Player 1 `buttons=2` mismatch | Input-owned | No presentation/audio change; routed input is GP4-02 | Reclassified to GP4-02 |
| GP3-05 audible output, listener mix policy, spatial attribution, duplicate/drop detection, channel counts, physical device loss, resource soak, intermission audio | Unsupported by captured evidence | No speculative `snd_*` modification. The logs prove memory initialization, device restart, ordered death/respawn, post-map-restart survival, joins, and clean shutdown only | Must be captured and certified in Phase 5/6 |
| GP2 duel camera/HUD inheritance, Siege scoreboard/intermission, modal death/respawn overlap | Explicitly unsupported by the Phase 1/2 runners | No claim and no speculative source repair | Remains a Phase 5 certification requirement |

## Files changed

- `codemp/cgame/cg_draw.c`
- `codemp/cgame/cg_drawtools.c`
- `codemp/cgame/cg_local.h`
- `codemp/cgame/cg_newDraw.c`
- `codemp/cgame/cg_view.c`
- `codemp/rd-vanilla/tr_scene.cpp`
- `tests/splitscreen/gameplay/regressions/presentation/test_presentation_repairs.py`

No `codemp/client/snd_*` or `codemp/rd-rend2/**` file was changed: the frozen
evidence did not demonstrate a defect in either implementation.

## Verification

This ticket was integrated without rebuilding, so verification is deliberately
limited to focused source contracts and whitespace checks:

```text
$ python3 -m unittest discover \
    -s tests/splitscreen/gameplay/regressions/presentation -p 'test_*.py' -v
Ran 6 tests
OK

$ git diff --check -- codemp/cgame codemp/rd-vanilla codemp/rd-rend2 \
    codemp/client/snd_* tests/splitscreen/gameplay/regressions/presentation
(no output)
```

The pre-repair evidence is the Phase 0 HUD/scene audit and the frozen
screenshots/logs referenced above. Post-repair frames, audio traces, resource
counts, and performance comparisons require the rebuilt frozen target; those
remain mandatory Phase 5/6 gates rather than being inferred from static tests.

## Status

IMPLEMENTED — six focused source regressions pass; rebuilt visual/audio
certification remains required.
