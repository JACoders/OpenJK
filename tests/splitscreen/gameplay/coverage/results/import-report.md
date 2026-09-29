# GP0-01 evidence import report

Imported on 2026-07-23 from committed frozen-build certification artifacts.

- Five evidence records were accepted: 2p FFA combat, 4p FFA respawn, 4p FFA
  spectate/rejoin, 4p FFA intermission, and 4p Team restart.
- Each record has a hash-pinned runner, config, log, and one or more PNGs.
- Claims are limited to literal markers found in the pinned log.
- Spawn-only Phase 5 mode scripts were not imported because their run logs and
  screenshots are not archived together under a hash-verifiable manifest.
- Routed 3p/4p proof bundles were not imported because their logs do not embed a
  binary/build hash; this avoids silently widening provenance claims.
- No evidence was found that satisfies the ledger rules for Holocron, Jedi
  Master, Duel, Power Duel, Siege, CTF, CTY, objectives, next-map, or clean exit.

The referenced build identity comes from the existing frozen certification
result manifests, but the historical logs do not embed its hash. All imports
therefore carry report-level `build_correlation=legacy-unverified`. The
generator makes their cells `seal_eligible=false`, so they cannot satisfy a
future frozen-build end-to-end or release-seal claim.
