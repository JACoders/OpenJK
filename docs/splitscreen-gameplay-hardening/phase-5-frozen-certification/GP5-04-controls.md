# GP5-04: One-Player and Independent-Client Controls

## Objective

Prove that split-screen hardening has not changed ordinary multiplayer or
independent-client/server behavior.

## Exclusive ownership

- `tests/splitscreen/gameplay/certification/controls/**`
- Evidence under `tests/splitscreen/gameplay/results/certification/controls/**`

No production changes are allowed.

## Matrix

- One-player fresh-home UI, local host, public join, all stock modes, input,
  profile/saber persistence, intermission, map change, and clean exit.
- Untouched upstream dedicated server with independent upstream clients.
- One split-screen process plus a remote fifth client for representative FFA,
  team, objective, pure, download, and map-transition scenarios.
- Compare authoritative gameplay results and one-player pixels/performance
  against the Phase 0 control manifest.

## Acceptance

- One-player and independent clients match upstream-defined behavior.
- No split-only cvar, menu, packet, renderer, or input state leaks into controls.
- All state/media/resource oracles pass on the frozen hashes.

## Candidate and runner

- Frozen executable SHA-256:
  `22d3fef485efd7cefe33aec1850b1a07c0f0876cc29b69fc5abb898f76b7cd70`
- Runner:
  `tests/splitscreen/gameplay/certification/controls/run.sh`
- Validator:
  `tests/splitscreen/gameplay/certification/controls/validate.py`
- Sealed result:
  `tests/splitscreen/gameplay/results/certification/controls/manifest.json`

The replacement-candidate run used the ordinary keyboard/mouse Main Menu → Play route,
then swept FFA, Holocron, Jedi Master, Duel, Power Duel, Team FFA, Siege, CTF,
and CTY with `cl_splitScreen=0` asserted after every map load. Eleven required
frames were captured, including the menu path and one frame per mode.

The independent-client control launched an unmodified upstream ARM64 dedicated
server and upstream ARM64 client, then joined the frozen candidate as a normal
one-player client. Both named clients were accepted, loaded their respective
cgame VMs, entered FFA, and produced distinct gameplay frames. No split state
or failure marker appeared. The replacement candidate hash matched before and
after. The sealed manifest records all checks true and hashes all 16 promoted
logs/screenshots.

An initial fixture attempt inherited a non-empty server password from the
machine's stock server configuration; both clients were rejected before game
state. The runner now explicitly clears the server and client passwords. The
corrected run passed every validator check.

## Status

IMPLEMENTED — deterministic one-player stock-mode and independent upstream
server/client controls pass. Public-server behavior remains separately covered
by the 2/3/4-player public lanes; resource and long-soak comparisons remain a
Phase 6 gate.
