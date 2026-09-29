# Phase 5: Frozen-Build Validation

## Entry gate

Phase 4 produces a release-candidate ARM64 build. Production code and acceptance
thresholds are frozen before these tickets begin.

## Parallel tickets

- [P5-01: Validate two-player play](P5-01-two-player-matrix.md)
- [P5-02: Validate three-player play](P5-02-three-player-matrix.md)
- [P5-03: Validate four-player play](P5-03-four-player-matrix.md)
- [P5-04: Validate network faults and endurance](P5-04-network-fault-soak.md)

## Independence rule

Each ticket owns separate configs, ports, homepaths, logs, screenshot directories,
and result manifests. Validation agents do not modify production code. A failed
case opens a defect assigned to the owning implementation area after all parallel
runs finish; the release candidate is then rebuilt and the full phase reruns.

## Exit gate

All required matrices pass on the same frozen build, every screenshot is visually
reviewed, all processes terminate cleanly, and no release-blocking defect remains.
