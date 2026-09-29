# P0-03: Build Screenshot Validation Tooling

## Objective

Automatically reject obvious visual failures while preserving mandatory human
inspection for final evidence.

## Primary ownership

Standalone tools under `tests/splitscreen/visual`. Do not modify game code or
capture scenario configs owned by P0-01.

## Work

1. Segment screenshots according to 2/3/4-player layout definitions.
2. Measure black-pixel ratios, frame-to-frame movement, cross-viewport similarity,
   viewport boundary continuity, and HUD-region uniqueness.
3. Add configurable masks for legitimate letterboxing and dark map areas.
4. Produce annotated failure images and JSON results.
5. Define conservative thresholds from the failure corpus and known-good stock
   one-player frames.

## Acceptance criteria

- The current black-region and duplicated-view defects fail automatically.
- A stock one-player frame and deliberately distinct synthetic quadrants pass.
- Reports identify the failed viewport and metric.
- The tool never edits source screenshots.
