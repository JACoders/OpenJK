# R6-03: Four-Player Public Capacity Integrity

## Ownership

`codemp/client/cl_main.cpp`,
`codemp/client/cl_input.cpp`,
`tests/splitscreen/cfg/routed_public_4p_acceptance.cfg`,
`tests/splitscreen/cfg/public_4p_probe.cfg`, and the `4p` public proof
folder.

## Acceptance

- Complete the four-pane character and Force-profile route through assigned
  keyboard/controller input.
- Never treat a placeholder snapshot as a successful local player.
- Require four unique server client numbers and four live player states before
  declaring the party active.
- Preserve explicit server same-IP errors when a server rejects a connection.
- Pass the complete gameplay assertions on a real public Internet endpoint.

## Status

PASS on the final frozen client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.

The complete UI-driven run passed on the public Internet server
`3.142.74.57:29070`. P1–P4 occupied distinct server client numbers `3`, `4`,
`5`, and `6`; all four were alive; five nonlocal human players and no bots were
present in the preflight snapshot; and keyboard/mouse plus Controllers 1–3
passed isolated movement and attack assertions. Visible UI selection produced
Kyle/dark, Desann/light, Reborn/light, and Tavion/light profiles. The durable,
checksum-sealed evidence is under
`tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/4p/`.

The earlier apparent capacity failure was a secondary-client handshake
deadlock. The client withheld user commands until cgame and pure validation
were ready, but also withheld the empty sequenced packet that a vanilla server
needs before it sends gamestate. `CL_SplitNetSendCmds` now transmits that empty
pre-cgame acknowledgement without creating a user command. Cgame then starts,
pure checksums are sent, and normal routed user commands begin. The
duplicate-client-number guard remains active and rejects genuine placeholder
connections after a five-second grace period.
