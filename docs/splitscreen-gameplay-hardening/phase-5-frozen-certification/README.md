# Phase 5: Frozen-Build End-to-End Certification

## Entry gate

Phase 4 closes every discovered defect and produces one release-candidate binary
plus module hashes. Production code and thresholds freeze before this phase.

## Parallel tickets

- [GP5-01: Two-player certification](GP5-01-two-player.md)
- [GP5-02: Three-player certification](GP5-02-three-player.md)
- [GP5-03: Four-player certification](GP5-03-four-player.md)
- [GP5-04: One-player and independent-client controls](GP5-04-controls.md)

## Independence rule

Each ticket owns separate configs, port blocks, homepaths, logs, screenshots,
and result manifests. Certification agents never edit production code or
thresholds.

## Exit gate

All four tickets pass against identical binary/module hashes. Any failure
creates a ledger entry, reopens the correct Phase 4 lane, and invalidates every
Phase 5 result until the entire phase reruns on the new frozen build.

## Frozen candidate

The release-candidate executable and native modules are frozen in
[`frozen-build.json`](frozen-build.json). The candidate was built successfully
from source commit `aed48b4164b863075209e172585d8a0c3029c5f7`; the working
tree's split-screen repairs are intentionally uncommitted and are represented
by the artifact hashes. Every certification runner verifies those hashes before
and after launch.

The current candidate executable is
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
It supersedes the invalidated `e2f55d9…dbaf2` candidate. The manifest freezes
the client, dedicated server, UI, game, four cgame instances, and both renderer
copies; all ten hashes were independently verified before Phase 5 began.

Current pass, pending-rerun, and explicitly unsupported states are tracked in
[`FINAL-737F-GATE-CHECKLIST.md`](FINAL-737F-GATE-CHECKLIST.md). That checklist,
not historical per-ticket prose, controls promotion of this candidate.
