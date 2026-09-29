# Split-screen menu clipping corpus

Capture a clean frame and menu frame at the same render resolution, without
moving the camera between them. Run `menu_clip_oracle.py CLEAN MENU --allowed
x0,y0,x1,y1`; coordinates are physical pixels and the rectangle is half-open.

Required pairs for each 2-, 3-, and 4-player layout:

| Pair | UI path | Active-player viewport |
|---|---|---|
| `top` | split-screen in-game top menu | each player |
| `profile` | `ingame_player` | each player |
| `controls` | `ingame_controls` | each player |
| `saber` | profile saber submenu | each player |
| `force` | profile Force submenu | each player |
| `console` | split-screen console overlay | each player |
| `osk` | split-screen keyboard, text/cvar/cheat variants | each player |

Use an isolated home path, software-identical UI assets, `r_fullscreen 0`, a
fixed `r_mode`, fixed map and spawn, and `timescale 0` before each pair. Retain
both PNGs and the stdout line from the oracle. The default tolerance permits a
three-level per-channel capture difference and no changed exterior pixels.

The existing deterministic interaction/capture sequence is:

```sh
OPENJK_HOMEPATH=/private/tmp/openjk-menu-corpus \
  tests/splitscreen/run_external_menu_qa.sh
```

Its `cfg/external_menu_probe.cfg` capture points already name the top, profile,
character, saber, Force, keyboard, cvar, cheat, controls, and console images.
For clipping certification, precede each named `screenshot_png` with a clean
same-frame capture (menu hidden, `timescale 0`) and evaluate the pair with this
directory's oracle. Keep each player-count run in a fresh home path so stale
screenshots and cvars cannot satisfy the corpus accidentally.

## Audit findings (2026-07-22)

The split-screen menu viewport is only a coordinate transform. `Text_Paint`
transforms a zero-area point and scales the font, but supplies no clip/scissor
rectangle to `R_Font_DrawString`. A glyph beginning inside a viewport may
therefore rasterize across its right or bottom edge. The same absence of a
renderer clip applies around `Menu_Paint`; rectangle helpers clamp geometry,
but cannot clip font glyphs or model/effect rasterization.

There is a second correctness hazard in `UI_TransformRect`: when source geometry
starts outside the virtual 640x480 bounds, it clamps destination coordinates
without adjusting texture coordinates. The visible remainder is rescaled over
the original UV range rather than clipped. Phase 1 needs a renderer-backed clip
region around every per-player menu paint, with transform state restored on all
exits; UV-preserving clipping should remain the draw helper's responsibility.

## Phase 1 implementation note

The current UI import table exposes no renderer scissor/clip-region operation.
R1-03 therefore enforces the pane in UI-owned primitives: textured quads clip
with proportional UV adjustment, font strings are shortened to the available
pane width (including shadow fringe), model refdefs use the clamped pane rect,
and nested menu transforms use a checked push/pop stack. Adding a true renderer
scissor remains a renderer/API follow-up; it cannot be wired solely in
`codemp/ui/*`.
