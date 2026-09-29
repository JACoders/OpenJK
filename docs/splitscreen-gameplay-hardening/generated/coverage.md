# Executed split-screen gameplay coverage

Generated from hash-validated logs and screenshots. `P` is an executed pass; `—` is unclaimed.

Build commit: `aed48b4164b863075209e172585d8a0c3029c5f7`
Binary SHA-256: `737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`

Build correlation: `legacy-unverified`. Historical logs do not embed the binary hash; these imports are not eligible for a frozen-build end-to-end or seal claim.

| Players | Mode | Class | setup | spawn | input | combat | score | death | respawn | objective | spectate_rejoin | intermission | restart | next_map | clean_exit | Evidence |
|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2 | ffa | partial | P | P | P | P | P | P | — | — | — | — | — | — | — | `ffa-2p-lightning` |
| 2 | holocron | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | jedimaster | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | duel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | powerduel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | team | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | siege | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | ctf | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 2 | cty | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | ffa | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | holocron | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | jedimaster | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | duel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | powerduel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | team | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | siege | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | ctf | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 3 | cty | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | ffa | partial | P | P | P | P | P | P | P | — | P | P | — | — | — | `ffa-4p-respawn`, `ffa-4p-spectate`, `ffa-4p-intermission` |
| 4 | holocron | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | jedimaster | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | duel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | powerduel | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | team | smoke | P | P | — | — | — | — | — | — | — | — | P | — | — | `team-4p-restart` |
| 4 | siege | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | ctf | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |
| 4 | cty | missing | — | — | — | — | — | — | — | — | — | — | — | — | — | none |

## Explicit gaps

- Empty cells (24): 2p holocron, 2p jedimaster, 2p duel, 2p powerduel, 2p team, 2p siege, 2p ctf, 2p cty, 3p ffa, 3p holocron, 3p jedimaster, 3p duel, 3p powerduel, 3p team, 3p siege, 3p ctf, 3p cty, 4p holocron, 4p jedimaster, 4p duel, 4p powerduel, 4p siege, 4p ctf, 4p cty
- Smoke-only cells (1): 4p team
- No imported evidence claims objective, next-map, or clean-exit coverage.

## Evidence links

- `ffa-2p-lightning`: [gameplay.stdout.txt](../../../tests/splitscreen/rendering/certification/gameplay/artifacts/gameplay.stdout.txt), [acceptance_kyle_lightning_kill.png](../../../tests/splitscreen/rendering/certification/gameplay/artifacts/acceptance_kyle_lightning_kill.png)
- `ffa-4p-respawn`: [respawn_flow.stdout.txt](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/respawn_flow.stdout.txt), [splitqa_respawn_flow_after.png](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/splitqa_respawn_flow_after.png)
- `ffa-4p-spectate`: [join_spectate_flow.stdout.txt](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/join_spectate_flow.stdout.txt), [splitqa_join_spectate_flow.png](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/splitqa_join_spectate_flow.png)
- `ffa-4p-intermission`: [intermission.stdout.txt](../../../tests/splitscreen/rendering/certification/intermission/artifacts/intermission.stdout.txt), [cert_intermission_4p.png](../../../tests/splitscreen/rendering/certification/intermission/artifacts/cert_intermission_4p.png)
- `team-4p-restart`: [map_team_matrix.stdout.txt](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/map_team_matrix.stdout.txt), [cert_lifecycle_team_before_restart.png](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/cert_lifecycle_team_before_restart.png), [cert_lifecycle_team_after_restart.png](../../../tests/splitscreen/rendering/certification/lifecycle/artifacts/cert_lifecycle_team_after_restart.png)
