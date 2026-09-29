# R2-03 stock and menu-state stress

`run.sh` exercises a frozen `build-x86_64` executable and UI module from an
isolated `/tmp` home path and dedicated port. It attaches four local clients,
alternates both split layouts, rotates menu ownership through all players, and
opens/closes the top menu for `OPENJK_STRESS_CYCLES` iterations. It records
binary hashes, the game log, a final screenshot, and periodic RSS observations.

Run:

```sh
OPENJK_STRESS_ROOT=/tmp/openjk-r2-03-stock-stress \
OPENJK_STRESS_PORT=29331 \
OPENJK_STRESS_CYCLES=200 \
tests/splitscreen/rendering/certification/stock-stress/run.sh
```

This practical run is not a substitute for the ticket's 30-minute soak. For
that gate, set `OPENJK_STRESS_DWELL_FRAMES=300`,
`OPENJK_STRESS_MIN_SECONDS=1800`, and a cycle count high enough to exceed the
minimum; the runner fails if measured process lifetime is short. Set
`OPENJK_STRESS_CAPTURE_INTERVAL` to retain periodic nested-stock-menu captures.
Use `OPENJK_STRESS_FINAL_DWELL_FRAMES` to add a conservative gameplay dwell
after the transition sequence; this avoids relying on estimated frame pacing.
Each cycle paints the top menu and then one of four nested stock menus before
restoring gameplay state. Retain the generated directory and report elapsed time.
The RSS summary reports only observed bounds and endpoint delta; it deliberately
does not infer the absence of an unbounded leak from a short run.
