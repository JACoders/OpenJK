# R6-02: Three-Player Public Party

## Ownership

`tests/splitscreen/cfg/routed_public_3p_acceptance.cfg` and the `3p` public
proof folder.

## Acceptance

- Route P1 through keyboard/mouse and P2/P3 through Controllers 1/2.
- Select all three models and Force profiles through their visible panes.
- Join a real public server through the stock browser.
- Prove three distinct server client numbers and three live player states.
- Prove movement and attack isolation for every assigned device.

## Status

PASS on the final frozen client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The serialized public run on `3.142.74.57:29070` assigned local client numbers
`3`, `4`, and `5`, retained five nonlocal human players and no bots in its
preflight snapshot, and passed visible Kyle/dark, Desann/light, and Reborn/light
UI selection plus isolated keyboard/mouse and Controllers 1–2 movement/attack
assertions. The durable, checksum-sealed evidence is under
`tests/splitscreen/gameplay/results/certification/final/public-active-vanilla/3p/`.
