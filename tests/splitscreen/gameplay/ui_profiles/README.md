# GP1-01 visible profile matrix

`run_matrix.sh` drives the existing routed stock-UI journeys under the Phase 0
isolated runner. Each case starts with an empty home, uses keyboard/mouse and
controller UI events for model and Force selection, joins a live listen server,
then launches a second process against the same home for persistence,
`vid_restart`, `in_restart`, and map-change checks.

The source validator rejects direct name/model/Force/saber/color/team profile
CVAR writes. A Phase 0 manifest must validate for every player count.

The current configs intentionally make no claim for requirements they do not
yet exercise: visible name, skin, saber, team editing; count switching
`2→3→4→2`; Back/Cancel; reopen isolation; and separate fresh-home defaults.
Those cells remain failures until supported by routed UI evidence.
