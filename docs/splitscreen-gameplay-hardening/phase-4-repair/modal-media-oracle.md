# Modal ownership media oracle

The analyzer now samples semantic pane-local ROIs for the top menu, scoreboard,
console, and chat surfaces instead of averaging each moving 3D viewport. It
classifies materially changed pixel coverage and requires both a visible owner
surface and absence of the same surface in every non-owner pane.

This prevents unrelated camera motion from dominating the result and prevents
small overlays from being diluted by a whole-pane average. Isolation remains
strict: synthetic owner-plus-non-owner overlays explicitly fail as
`nonowner_bleed`.

Offline tests cover owner-only surfaces, missing owner surfaces, synthetic
bleed, material modal pixels, and sub-material camera noise.

The four-player pane order is row-major: P1 top-left, P2 top-right, P3
bottom-left, P4 bottom-right. A retained-r4 fixture assertion pins P2 chat's
local-top origin to global `y=0` at 640×480. Top-menu measurement uses the
paired orange navigation-text and blue-separator signature; white join
notifications and sky/camera changes cannot satisfy it.
