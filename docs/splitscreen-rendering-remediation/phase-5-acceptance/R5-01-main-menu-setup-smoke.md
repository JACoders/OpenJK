# R5-01: Main-Menu Setup Smoke

## Ownership

`tests/splitscreen/cfg/menu_to_setup_smoke.cfg` and
`tests/splitscreen/run_menu_to_setup_smoke.sh`.

## Acceptance

- The mouse selects Play and Split Screen, then selects 2 and Next.
- `cl_splitScreen=1`, `ui_splitScreenMenuMode=setup`, and the last paint path is
  `setup` before interaction resumes.
- The captured frame visibly contains two player configuration panes.

## Status

PASS: `/private/tmp/openjk-menu-to-setup.nr9v2U`.
