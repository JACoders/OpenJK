# Modal ownership discovery

`run.sh` launches isolated 2/3/4-player local games with three SDL virtual
controllers. The external driver routes Start, Back, the Back+Start console
chord, scoreboard, and chat buttons through the same SDL path as a controller.
Each surface uses an adjacent pre-event frame; top-menu and console
classifications also require runtime state markers. The config asserts live
players and verifies that P1 keyboard input remains P1-only while P2's menu is
open.

The analyzer compares all panes against the adjacent frame. A surface passes
only when the target pane changes materially more than every other pane and any
required state marker passes. Feature failures are recorded in
`results/modal_ownership`; infrastructure failures make the runner exit
nonzero.

After `run.sh all`, run `build_matrix.py` and `validate.py`. Each generated
manifest must also pass the Phase 0 runner's `--validate-only` check.
