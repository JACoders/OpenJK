# GP3-05: Audio-Enabled Network Runtime

## Objective

Replace sound-disabled acceptance assumptions with an explicit multi-player
audio policy and stability proof.

## Exclusive ownership

- `tests/splitscreen/gameplay/runtime/audio/**`
- Evidence under `tests/splitscreen/gameplay/results/runtime/audio/**`

## Scenarios

- Run representative 2/3/4-player FFA, team, objective, menu, death, respawn,
  announcer, chat, and intermission journeys with sound enabled.
- Trace per-player local/UI, weapon, Force, voice, ambient, announcer, and
  spatialized events.
- Exercise audio device loss/reopen, volume/settings changes, map/VM restart,
  process relaunch, and simultaneous effects in all panes.
- Record channel/voice counts and resources during a 30-minute network baseline.

## Acceptance

- The chosen listener/mix policy is documented and deterministic.
- No player event is silently dropped, duplicated excessively, spatialized from
  stale state, or attributed to the wrong pane.
- Audio reset and shutdown leak no device, thread, channel, or crash.

## Defect handling

Record findings for GP4-01 or GP4-05; do not patch production code.

## Status

DISCOVERY COMPLETE — FAILED (2026-07-23).

- Frozen client SHA-256 `223e8a1267...` ran audio-enabled 2p/3p/4p listen
  hosts with a separate audio-enabled loopback client. All three Phase 0
  manifests validate, both processes exit zero, and two screenshots are
  retained per case.
- Logs prove sound-memory initialization; ordered death→respawn; SDL audio
  device close, shutdown, and reinitialization across `snd_restart`; all local
  clients alive after map restart; the independent network client joining; and
  clean process shutdown.
- Each cell contains one deterministic P1 routed-input assertion mismatch:
  `MOUSE1` produced button value `2` while the probe expected `1`. P2–P4
  controller attack assertions pass. These are retained as discovery failures,
  not hidden behind process exit zero.
- Audio-enabled CVARs, weapon actions, network chat, volume settings, listen
  networking, and screenshots are smoke only.
- No captured audio or mixer telemetry exists, so audible output, listener
  policy, spatial position, pane attribution, drops/duplicates, complete event
  classes, channel/voice counts, physical device loss, device/thread/channel
  leaks, the 30-minute baseline, and intermission audio remain unsupported.
- Default discovery validation exits zero for an honest failed matrix;
  `--require-acceptance` exits nonzero.

Evidence:

- `tests/splitscreen/gameplay/results/runtime/audio/current/matrix.json`
- `tests/splitscreen/gameplay/results/runtime/audio/current/findings.md`
