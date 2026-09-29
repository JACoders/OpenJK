# Split-screen scene/clear audit (R0-01)

## Render path

Each active cgame VM is selected in `CL_CGameRendering`, builds one `cg.refdef`,
and calls `CG_R_RENDERSCENE`. The syscall forwards that refdef unchanged to
`RE_RenderScene`. The renderer converts its top-origin rectangle to an OpenGL
bottom-origin viewport, and every `RB_BeginDrawingView` calls
`SetViewportAndScissor` before clearing depth, stencil, or color.

`RE_ClearScene` does **not** clear framebuffer pixels. It advances the first
entity/light/poly indices so the next submitted scene owns a distinct list.

## Findings

| ID | Root cause | Visible failure | Required repair invariant |
| --- | --- | --- | --- |
| SC-01 | `CG_ApplySplitScreenRect` partitions with integer halves and then applies `width &= ~1` and `height &= ~1` independently to every result. | An odd framebuffer dimension loses its final column/row; when a half is odd, an internal one-pixel strip can also be uncovered. Those pixels retain the prior frame or prior UI contents. | The integer rectangles for all active players must be in bounds, pairwise non-overlapping, and their union must equal the entire framebuffer for odd and even dimensions. Do not round partition results down. |
| SC-02 | `rd-vanilla` retains the prototype `r_splitScreen` path, which calls `R_RenderSplitScreenViews` inside every `RE_RenderScene`, while cgame already submits one partitioned refdef per player. The client only forces this cvar off during primary cgame initialization. | If `r_splitScreen` is nonzero later (including persisted/manual state), every player rectangle is split again and the same camera is rendered twice. This produces duplicated/squashed views and incorrect clears. | Exactly one layer owns viewport partitioning. `RE_RenderScene` must render the supplied refdef rectangle once. |
| SC-03 | Renderer 2D setup deliberately restores a full-frame viewport and scissor. There is no stack-based GL viewport/scissor restoration. | Code that assumes the preceding scene scissor remains active can draw across sibling views. | Every 3D view must set viewport and scissor before any clear; every clipped 2D sequence must establish its own clip. The canonical state after generic 2D setup is full-frame. |

## Coverage matrix

Coordinates below are top-origin half-open rectangles `(x, y, width, height)`.
`W0=floor(W/2)`, `W1=W-W0`, `H0=floor(H/2)`, and `H1=H-H0` preserve odd
edge pixels.

| Players | Layout | P1 | P2 | P3 | P4 |
| ---: | --- | --- | --- | --- | --- |
| 2 | horizontal | `(0,0,W,H0)` | `(0,H0,W,H1)` | - | - |
| 2 | vertical | `(0,0,W0,H)` | `(W0,0,W1,H)` | - | - |
| 3 | horizontal | `(0,0,W,H0)` | `(0,H0,W0,H1)` | `(W0,H0,W1,H1)` | - |
| 3 | vertical | `(0,0,W,H0)` | `(0,H0,W0,H1)` | `(W0,H0,W1,H1)` | - |
| 4 | horizontal | `(0,0,W0,H0)` | `(W0,0,W1,H0)` | `(0,H0,W0,H1)` | `(W0,H0,W1,H1)` |
| 4 | vertical | `(0,0,W0,H0)` | `(W0,0,W1,H0)` | `(0,H0,W0,H1)` | `(W0,H0,W1,H1)` |

The three-player layout is intentionally identical for both layout values in
the current product contract.

## State-transition invariants

| Transition | Restore/establish invariant |
| --- | --- |
| cgame `ClearScene` -> submissions | Scene list base indices advance; framebuffer and GL viewport/scissor do not change. |
| `RE_RenderScene` -> `R_RenderView` | Refdef bounds are copied exactly and Y is converted once to GL coordinates. |
| render command -> `RB_BeginDrawingView` | `SetViewportAndScissor` executes before all depth/stencil/color clears, so clears cannot escape the view. |
| 3D -> generic 2D | `RB_SetGL2D` explicitly establishes the full framebuffer viewport/scissor; callers must not rely on the former 3D clip. |
| one player's scene -> next player's scene | The next `RB_BeginDrawingView` re-establishes that player's viewport/scissor; no inherited viewport is trusted. |

Run the audit gates with:

```sh
python3 -m unittest discover -s tests/splitscreen/rendering/scene -p 'test_*.py'
```
