# R2-01 frozen-build layout results

Execution date: 2026-07-22 (Pacific/Honolulu)

The frozen x86_64 client and matching cgame/UI/game modules completed all 12
gameplay cases: 2/3/4 players, horizontal/vertical configuration, at 1280x720
and 1024x768. Every run used an isolated `/private/tmp` homepath and unique UDP
port.

All 12 gameplay PNGs were manually inspected. Each contains the requested
number of independent scenes and HUDs in the expected rectangles. No blank or
duplicated panels, stale strips, cross-viewport clears, or visible seam gaps
were observed. All automated variance/difference/dimension oracles passed.

Twelve menu-state PNGs were also captured, but they were not manually inspected
during the bounded run. They are retained as evidence; no visual-pass claim is
made for those menu captures.

Artifacts are in `artifacts/`, with one PNG, log, and oracle report per gameplay
case plus one menu PNG per case.
