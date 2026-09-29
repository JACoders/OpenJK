# P2-03: Implement Viewport and Scissor Primitives

## Objective

Provide renderer-level rectangles, clears, clipping, and aspect calculations for
2-, 3-, and 4-player layouts.

## Primary ownership

Renderer/client screen primitives only. Do not edit cgame scene construction,
HUD code, client snapshots, or UI menus.

## Work

1. Define integer pixel rectangles for every supported layout and resolution.
2. Add viewport-local clear and scissor boundaries.
3. Calculate aspect/FOV inputs without cross-viewport state.
4. Prevent scene, post-process, and 2D draws from escaping their rectangles.
5. Add canvas pixel tests for coverage, overlap, and gaps.

## Acceptance criteria

- Rectangles cover the framebuffer exactly except intentional dividers.
- No unexplained black wedges or stale pixels remain between scenes.
- Odd dimensions and Retina/windowed resolutions are deterministic.
- One-player rendering is unchanged when split screen is disabled.
