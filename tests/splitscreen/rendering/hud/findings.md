# R0-02 HUD and draw-state findings

## Phase 1 remediation status

R1-02 now routes the audited axis-aligned bypasses through `CG_DrawPicUV`,
which transforms and software-clips them to the active HUD rectangle.  Rotated
HUD artwork falls back to its clipped axis-aligned representation during split
screen because the cgame render API has no scissor import.  Text measurement is
converted back to viewport-local units so alignment and clipping calculations
match the uniformly scaled glyphs.  The HUD boundary resets renderer color both
before and after every player draw, making sequential render order independent.

## Executive finding

`CG_DrawActive2D` installs a coordinate transform around `CG_Draw2D`, but it
does not install a renderer clip/scissor rectangle.  The transform is honored
only by a subset of cgame draw helpers.  Stock HUD paths which call renderer
imports directly therefore retain full-screen coordinates and can draw outside
the active player's viewport.  Text has a second inconsistency: placement is
scaled independently on X and Y while glyph size uses the smaller viewport
scale, but width/height queries remain unscaled.

The cgame globals are isolated when each local player has its own cgame VM, but
renderer state (`R_SetColor`) is process-global and survives between sequential
VM draws unless a path resets it.  A viewport transaction must therefore own
both clipping and renderer-state cleanup even when a HUD path exits early.

## Stock primitive inventory

| Primitive/family | Current behavior under `CG_DrawActive2D` | Classification |
| --- | --- | --- |
| `CG_DrawPic`, `CG_DrawRotatePic`, `CG_DrawRotatePic2` | Calls `CG_Transform2DRect` | transformed, not clipped |
| `CG_FillRect` | Transforms then brackets color with `R_SetColor(color/NULL)` | transformed, color-safe, not clipped |
| `CG_DrawRect` via `CG_DrawSides`/`CG_DrawTopBottom` | Transforms geometry; border thickness is scaled with global `screenXScale`/`screenYScale`, not viewport scale | partially transformed, not clipped |
| `CG_DrawChar` / `CG_DrawStringExt` | Character rects transform; string helper resets color on its non-Asian path | transformed, not clipped |
| `CG_Text_Paint` | Transforms origin and scales font by `min(viewportXScale, viewportYScale)` | transformed with inconsistent metrics/layout, not clipped |
| `CG_Text_Width`, `CG_Text_Height` | Return renderer metrics at the caller's untransformed scale | untransformed measurement |
| `CG_DrawProportionalString`, `CG_DrawScaledProportionalString` | Alignment offset uses untransformed width before `CG_Text_Paint` transforms the origin | misaligned when X scale differs from the chosen font scale |
| `CG_Text_Paint_Limit` | Clipping decision and `maxX` feedback use untransformed metrics; final paint is transformed | logical clip does not match rendered viewport |
| `CG_FillRect2` | Explicit real-coordinate helper; no transform | bypass (currently no stock caller) |
| direct `R_DrawStretchPic` / `R_DrawRotatePic*` | No automatic cgame transform or per-player clip | unsafe bypass |
| `R_Font_DrawString` | Safe only through `CG_Text_Paint`; direct callers bypass the transform | unsafe bypass |
| `R_SetColor` | Global renderer modulation, unrelated to viewport transform | must be reset at viewport boundary |
| `Menu_PaintAll` / owner draw | Legacy status-menu call is disabled in `CG_Draw2D`; `CG_OwnerDraw` body is compiled out | dormant; menu ownership is not the active stock HUD path |
| 3D icon/model refdefs | Build independent `refdef_t` rectangles | requires explicit viewport composition and clipping |

There is no cgame render import for a clip/scissor operation in the audited
transaction, and `CG_Set2DViewportTransform` stores only X/Y/W/H mapping data.
Consequently even transformed rotated geometry and font effects are not
guaranteed to stay inside a viewport.

## Required regression cases and exact call paths

### Scoreboard and intermission

Call paths:

* normal/death: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawScoreboard -> CG_DrawOldScoreboard`
* intermission: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawIntermission -> CG_DrawScoreboard -> CG_DrawOldScoreboard`

`CG_DrawOldScoreboard` mixes `CG_FillRect`, `CG_DrawPic`,
`CG_DrawProportionalString`, and `CG_Text_Paint`.  Geometry transforms, but
column positions and centering are computed with unscaled `CG_Text_Width`.
For a horizontal two-player viewport `(xScale=1, yScale=.5)`, glyphs render at
`.5` scale while pre-paint centering offsets remain full width, shifting
centered text left.  For a vertical viewport, baselines retain full Y spacing
while glyphs shrink to `.5`.  Intermission takes an early return from
`CG_Draw2D`; cleanup currently happens only because the outer wrapper executes
after that return, so cleanup must remain outer-scope/RAII-like.

### Spectator

Call path: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawSpectator` (then crosshair
and crosshair names).  Spectator labels use `CG_Text_Width` for centering and
`CG_Text_Paint` for rendering, reproducing the metric/paint mismatch.  The
team-spectator marquee in `cg_newDraw.c` additionally makes its limit decision
in untransformed units in `CG_Text_Paint_Limit`.

### Death

Call path: `CG_DrawActive2D -> CG_Draw2D`; a dead player suppresses ordinary
status, then `CG_DrawScoreboard` displays because `CG_DrawOldScoreboard` treats
`PM_DEAD` as fully visible.  `fallingToDeath` also calls full-screen
`CG_FillRect(0,0,640,480)`, which is mapped correctly but is not scissored.
The death case therefore exercises both the scoreboard metric bug and the lack
of a hard viewport boundary.

### Zoom

Call path: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawZoomMask`.
Most mask pieces use transformed helper functions, but the disruptor charge bar
calls `R_DrawStretchPic(257,435,...)` directly, leaving it in full-screen
coordinates.  `CG_DrawZoomMask` also sets several modulation colors and has no
unconditional final `R_SetColor(NULL)`.  Thus its last modulation can affect a
later primitive or the next local player's VM draw.  A viewport-end reset is
required even after individual paths are repaired.

### Crosshair (additional high-impact stock path)

Call path: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawCrosshair`.
The crosshair and corona call `R_DrawStretchPic` directly and combine virtual
constants (`640`, `480`) with physical-pixel `cg.refdef.x/y`.  This is neither
the helper transform nor a coherent physical-coordinate calculation.  Health,
hacking, and siege bars receive those mixed-space coordinates afterward.

### Lagometer (additional stock path)

Call path: `CG_DrawActive2D -> CG_Draw2D -> CG_DrawLagometer`.  Its background
uses `CG_DrawPic`, but graph samples call `R_DrawStretchPic` directly with
untransformed `ax/ay/aw/ah`; the graph and its background therefore diverge in
split screen.

## Reset and clipping requirements for remediation

1. Begin every local-player 2D draw with one viewport object that defines the
   coordinate mapping and an actual renderer clip/scissor.
2. Route every 2D primitive, font draw, rotated draw, and embedded 3D icon
   through that object; prohibit raw renderer 2D calls in stock HUD code.
3. Expose transformed text measurement or perform alignment in viewport-local
   coordinates using one consistent aspect policy.
4. End every local-player draw by disabling the clip and calling
   `R_SetColor(NULL)`, including information, intermission, level-shot, and
   disabled-HUD early exits.
5. Keep mutable HUD animation/timer/menu state VM-local.  Do not use renderer
   state or process statics as implicit communication between player draws.
