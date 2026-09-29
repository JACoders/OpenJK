# P7-01: Certify Performance

## Objective

Certify resource budgets on the frozen production candidate without modifying it.

## Primary ownership

`artifacts/certification/performance` and benchmark runtime homepaths only. Do
not modify source, packages, docs, or other certification reports.

## Work

1. Run frozen 1/2/3/4-player CPU, GPU, memory, effects, audio, and network tests.
2. Compare results to Phase 1 budgets and Phase 5 baselines.
3. Record hardware, OS, maps, settings, samples, variance, and raw profiles.
4. Fail on missing samples, unexplained regression, or budget violation.

## Acceptance criteria

- Every required budget passes with the prescribed repeat count and variance.
- Results identify the exact commit and executable hash.
- One-player and single-player regressions remain within limits.
- The signed report is self-contained and reproducible.
