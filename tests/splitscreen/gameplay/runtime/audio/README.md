# GP3-05 audio-enabled runtime discovery

The 2/3/4-player cases run with `s_initsound=1`, a listen-server network peer,
per-pane routed attack commands, death/respawn, chat, `snd_restart`, map
restart, and normal process shutdown under the Phase 0 contract.

The report deliberately separates `proven_log` from `attempted_smoke` and
`unsupported`. A sound-manager log line proves only that engine initialization
ran; it does not prove audible output, spatialization, mixer policy, event
attribution, or absence of dropped/duplicated sounds.

Default validation exits zero for an honest discovery report.
`--require-acceptance` fails while findings or unsupported cells remain.
