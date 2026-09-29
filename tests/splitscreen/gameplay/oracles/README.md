# Gameplay proof oracles

`oracle.py` accepts only a versioned manifest. Runtime manifests use
`artifact.kind: evidence_bundle`; the bundle must name separately hashed
on-disk state, image-analysis, audio-trace, and resource records, and may name
hashed raw attachments such as screenshots, logs, recordings, and samples.
Missing or altered records fail before semantic checks. Synthetic
`artifact.kind: fixture` manifests are reserved for oracle self-tests.

The oracle validates per-player network state, lifecycle and gameplay state,
pane identity and ownership, deterministic audio traces, and process/resource
cleanup. Every failure records a code, player, phase, expected value, and actual
value.

Run all positive and mutation fixtures:

```sh
python3 tests/splitscreen/gameplay/oracles/selftest.py
```

The machine-readable report is written to `selftest-report.json`.
