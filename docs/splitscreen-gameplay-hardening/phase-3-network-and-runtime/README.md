# Phase 3: Network and Runtime Resilience

## Entry gate

Phase 2 provides reusable terminal-state mode journeys and a complete list of
gameplay findings.

## Parallel tickets

- [GP3-01: Stock local hosting and LAN play](GP3-01-local-host-lan.md)
- [GP3-02: Pure servers, downloads, mods, and map transitions](GP3-02-pure-download-mod-map.md)
- [GP3-03: Rejection and recovery isolation](GP3-03-rejection-recovery.md)
- [GP3-04: WAN impairment and slot churn](GP3-04-wan-churn.md)
- [GP3-05: Audio-enabled network runtime](GP3-05-audio-runtime.md)

## Independence rule

Each ticket owns a separate controlled server, port block, proxy if applicable,
homepath prefix, and evidence directory. These are discovery tickets and do not
edit production source.

## Exit gate

Every scenario has a deterministic result; surviving players are proven healthy
after partial failures; all sockets/slots clean up; and all findings are routed
to a single Phase 4 owner.

## Result

DISCOVERY COMPLETE — REPAIR REQUIRED.

- GP3-01: three valid 2/3/4-player local-host/LAN manifests; runtime attachment,
  final-controller routing, restart survival, and an independent loopback
  client are proven. Visible hosting/browser paths and several policy cells
  remain unsupported.
- GP3-02: six valid pure/non-pure manifests; 30 narrow compatibility passes,
  six screenshot smoke cells, and 14 unsupported download/mod/QVM/content
  cases.
- GP3-03: 14 passes, five deterministic failures, three prerequisite blocks,
  and 26 not-covered cells. Whole-party reconnect after P1 disconnect stalls
  in `connecting` for every party size; a 4-player P4 rejoin also hits the
  same-IP connection limit.
- GP3-04: 30 passes, nine failed cycle cells, 18 unsupported impairment cells,
  and 13 not-covered stress/instrumentation cells. The 2p and 3p bounded churn
  journeys pass; 4p can leave a rejoined secondary in spectator state.
- GP3-05: three valid audio-enabled manifests with sound restart, respawn,
  restart, network peer, and shutdown evidence. P1 attack assertions fail in
  all party sizes (`buttons=2`, expected `1`); audible/spatial/mixer and
  long-run evidence remains unsupported.

All discovery ran against frozen client SHA-256
`223e8a126722a5728076fa1cbba2d4954f2cf51a849951c6af25a9ae25eabc33`.
No Phase 3 ticket rebuilt or edited production code.
