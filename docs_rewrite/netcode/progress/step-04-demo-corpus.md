# Step 4: demo corpus

Status: **done**, 5 November 2026. Plan: [PLAN.md](../PLAN.md), step 4.

## Data supplied

The owner supplied 3.8 GB of client demos from a friend, all in `data/demos/train/` (not committed; demo files contain player names, which are not copied into these documents). 1,116 demo files: 676 named `.dm_26` and 440 without extension (names cut by the archive tool; same format, checked by decoding). 11 archives (`.zip`, `.rar`) were not extracted and are skipped.

All decisions about which demos are used belong to the index and the selection below, not to the folder layout: the plan's `train/` and `holdout/` folders are replaced by a column of the list file.

## What was done

- `jkadec.py`:
  - `read_demo(fn)`: the `.dm_26` format (`CL_WriteDemoMessage`, `codemp/client/cl_main.cpp:206` in OpenJK): per message `int32` sequence, `int32` length, the bytes; end marker `-1 -1` (`CL_StopRecord_f`). Returns the messages and how the file ends: `marker`, `eof` (no marker), `truncated` (last record cut), `badlen`.
  - `parse_message(..., demo=True)`: a demo message ends with the `svc_EOF` the server appends after the capture point of the study. The parser requires it, checks that it is followed by 1 to 8 bits (the writer's `cursize = (bit >> 3) + 1`, `codemp/qcommon/msg.cpp:227`), else marks the message `BADEND`, and returns `size`, the message size without `svc_EOF`, which is what the study's captures measured.
  - The first message of a demo is the gamestate rebuilt by the recording client (`CL_Record`, `cl_main.cpp:341-396`, same layout as the server's including the `short 0` filler): it is used for decoding (configstrings, baselines), never for gamestate sizes.
- `demo_index.py` (new): decodes every demo in full (in parallel; about 1 MB/s per process in Python) and writes `data/demos/index.tsv`, one line per file: decode counts, file end, snapshot interval, duration, mean engine size of snapshot-only messages, entities and visible players per snapshot (medians), and from the first gamestate: server `version`, `protocol`, `gamename`, `fs_game`, map, game type, `sv_fps` when the server publishes it, players, bots, spectators.

## Results of the index

| item | value |
|---|---|
| Files decoded | 1,116 (674 truncated, 438 with end marker, 4 ending without marker); 0 exceptions |
| Messages | 14,185,846: 13,992,793 valid, 148 `BADEND`, 191,780 with a missing base, 1,125 gamestates |
| Game time | 148 hours |
| Protocol | 26 (1.01) for all |
| Game types | CTF 63 h (407 files), team FFA 46 h (270), siege 35 h (411), duel 3 h (28); no FFA, race or vehicles map apart from siege |
| Mods (`gamename`) | `base_enhanced` (all the CTF), `basejka` (team FFA and duel), `base_entranced` (all the siege), `base` (1 file); `fs_game` empty or `base`: no JA+ or JAPRO |
| Snapshot interval | 33 ms (`sv_fps` 30) in 801 files, 50 ms (`sv_fps` 20) in 286, others rare (66, 99, 132 ms) |
| Bots | none |
| Maps | 67 |
| Engine size of snapshot-only messages, time-weighted | CTF 242 bytes, team FFA 165, siege 140, duel 74 (study capture: 337.6 on `mp/ffa3` with bots) |
| Visible players per snapshot (median per file) | 0 to 9, mostly 2 to 5 |

All games come from the 1.01 base game module family (players sent as `TR_LINEAR_STOP`, as in the captures): the `TR_INTERPOLATE` prediction rule still has no real data (the open question on the game module stays open).

## Findings

1. **Missing bases.** 191,774 of the 191,780 messages that fail come from one server build, `JAmp: v1.0.1.0 linux-ARM-debug Sep 16 2022` (85 of its 404 files; 29 files lose more than half their messages). The first message after the recording's first full snapshot is a delta from a snapshot received **before** recording started (example: message 11336 against base 11332, the demo starting at 11335). The live client had that base; the demo does not, and every later message whose base chain goes through a failed one fails too. The standard 1.01 builds (over 7 million messages) show 6 such messages. Such messages cannot be measured; the selection excludes them and the files where they dominate.
2. **Bytes after `svc_EOF`.** 148 messages carry 10 to 22 bits after `svc_EOF` instead of 1 to 8. They decode completely (and so do the following messages), so the content before `svc_EOF` is intact. They come only from modified server builds (80 from the ARM build, 61 from `linux-i386 Oct 24 2026`, 7 from `linux-i386 Aug 31 2015`); the standard builds show none. OpenJK's `SV_Netchan_Transmit` writes nothing after `svc_EOF`. Accepted for measurement, with `size` taken before `svc_EOF`.
3. **The reader is validated** on the standard builds: every message ends 1 to 8 bits after `svc_EOF`.

4. **The codec dropped pilot fields.** A siege demo on `siege_hoth3_v6` (server `JAmp: v1.0.1.0 linux-i386 Feb 12 2022`, `base_entranced`) failed the round trip from its first pilot state on: that server sends pilot states with `lc` up to 96, not the 1.01 limit of 58 (it has the OpenJK fix), and `codec2.py` still applied the 1.01 rule (`size - 82`) and lost `bobCycle`, `groundEntityNum`, `inAirAnim`. `codec2.py` now codes all 140 fields of the pilot table, in line with the decision not to restore the rule (08 section 8.1); `fuzz.py` follows automatically. The captures contain no pilot state: `run_full.py` (139.1) and `run_pred.py pi --fuzz` (107.3, fuzz 0 failures) are unchanged, and so is `sim_loss.py` (the 20 runs of 03 section 3.9 give the same figures, 0 failures; rerun 5 November 2026).

## Selection (decided with the owner, 5 November 2026)

`demo_select.py` (new) writes `data/demos/selection.tsv` from the index:

- excluded: files where more than 5 % of the messages cannot be decoded, files shorter than 3 minutes: 660 of 1,116 files remain (most exclusions are short siege rounds);
- held-out set by map: in each game type about 20 % of the maps (at least one), drawn with seed 1; every file of those maps is held out. The script checks that no map is in both sets;
- 60 training demos (CTF 25, team FFA 18, siege 14, duel 3; 23 maps; 4 server builds) and 20 held-out demos (CTF 8, team FFA 6, siege 5, duel 1; 9 maps; 5 builds);
- only the first 5,400 messages of each demo are used (about 3 minutes at 30 snapshots per second; 5 demos are shorter and give all their messages): 427,468 messages in all, which the Python codec handles in minutes. A demo is used from its start because the codec needs the base chain from the first full snapshot.

## Loaders

- `jkadec.demo_capture(fn, limit)`: a demo in the shape of `read_capture`.
- `bdata.load_one(..., demo=True)` and `fullload.load_stream(..., demo=True)`: accept messages ending with `svc_EOF` (and `BADEND`), take sizes without `svc_EOF`; `fullload` adds `nbytes` (engine size) to each message.
- `bdata.load_demo(rel)` and `fullload.load_demo(rel)`: one selected demo (path relative to `data/demos`), cached per demo in `data/demos/cache/` (about 37 MB per demo; 2.9 GB for the 80). The three captures and their caches are unchanged, so every earlier script gives the study's numbers.
- `demo_select.selection()`: the list of (set, path).

## Check

| check | result |
|---|---|
| Round trip of the 80 selected demos with the study codec (tables trained on the capture, as `run_full.py`): `python run_demos.py [processes]` (default 16 processes, about 5 minutes; the first run also builds `data/demos/cache/`) | 427,468 messages, 0 failures (after the pilot fix; rerun 5 November 2026) |
| Every file decodes with the validation | yes, except the documented missing-base and trailing-bits cases of modified servers |
| List files | `data/demos/index.tsv` (all files), `data/demos/selection.tsv` (the sets) |
| Train and held-out sets separate | yes, by file and by map |

## Corpus scope (owner, 5 November 2026)

No race, JA+ or JAPRO demos will be added. FFA can be recorded with bots. The target game module is the base one (08 section 8.3) (players as `TR_LINEAR_STOP`, like the corpus), so no `TR_INTERPOLATE` recording is needed; a FFA bot demo on this branch with the base game module would only add the FFA game type.

To add such a demo: record it on a local game of this branch (`g_gametype 0`, bots added with `addbot`) with the client commands `record <name>` and `stoprecord`, at least 3 minutes long (shorter files are excluded by the selection); put the `.dm_26` file in `data/demos/`; run `python demo_index.py` to rebuild `index.tsv`. Do not run `demo_select.py` again unless the selection itself is to change: `selection.tsv` keeps the 80 demos above, and a new FFA demo is measured on its own.

## What step 5 needs from this one

- Iterate `demo_select.selection()`, load each demo with `fullload.load_demo` (whole messages) or `bdata.load_demo` (snapshot records for the ideal-byte benchmarks), train on `train`, measure on `holdout`.
- The scripts of the study use the keys `5min`, `bots`, `solo`; they need a demo mode (or new drivers) that train the per-field modes and tables on the training demos.
- Report by game type, map, server build and snapshot interval (33 / 50 ms), and by base age (`delta`).
