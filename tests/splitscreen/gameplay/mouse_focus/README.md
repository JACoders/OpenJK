# GP1-02 mouse-focus discovery

`run.sh` generates and launches isolated 2/3/4-player cases through the Phase 0
runner. The configs exercise pointer extremes, repeated deltas, drag, layout,
count, resize/restart, and supported modal surfaces. `analyze.py` writes an
immutable, hash-linked `matrix.json` and preserves failures as
`DISCOVERED_FAIL`.

The test intentionally makes no production changes. Unsupported or unsafe
automation is listed in the result limitations rather than inferred as passing.
