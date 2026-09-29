# Split-screen HUD draw-state audit

This directory records the phase-0 characterization for R0-02.  The tests are
deliberately source-level: they identify the unsafe call paths in the current
implementation without changing production code.  Later remediation phases
should turn the `known gap` assertions into invariants of the replacement HUD
viewport API.

Run with:

```sh
python3 -m unittest discover -s tests/splitscreen/rendering/hud -p 'test_*.py'
```

See [findings.md](findings.md) for the primitive inventory and exact call
paths.
