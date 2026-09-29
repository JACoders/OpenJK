# R6-01: Two-Player Public Party

## Ownership

`tests/splitscreen/cfg/routed_public_2p_acceptance.cfg`,
`tests/splitscreen/run_routed_public_acceptance.sh`, and the `2p` public proof
folder.

## Acceptance

- Select the player count, models, and Force profiles through visible UI input.
- Join a real public server through the stock Favorites browser.
- Prove two unique server client numbers and two live player states.
- Prove keyboard/mouse affects only P1 and Controller 1 affects only P2.
- Capture the setup, browser, gameplay, and occupied public scoreboard.

## Status

PASS on the final frozen client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The serialized public run on `3.142.74.57:29070` assigned local client numbers
`3` and `4`, retained three nonlocal human players in its preflight snapshot,
and passed visible Kyle/dark and Desann/light UI selection plus isolated
keyboard/mouse and Controller 1 movement/attack assertions. The durable,
checksum-sealed evidence is under
`tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/2p/`.
