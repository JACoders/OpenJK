# GP2-05 Siege discovery harness

`run.sh` executes the frozen OpenJK binary against stock `mp/siege_hoth` and
`mp/siege_desert` with two, three, and four local players. It records lifecycle
assertions and screenshots for class/team entry, routed use input, death and
respawn timing, class-change recovery, map restart, and next-map rotation.

Run:

```sh
./tests/splitscreen/gameplay/modes/siege/run.sh
./tests/splitscreen/gameplay/modes/siege/validate.py
```

The harness is a discovery probe. Command-driven class assignment is not proof
of per-pane class-menu routing, and a routed use button is not proof of stock
objective completion. The class/team capture is a full-screen Mission
Objectives/Join menu, not gameplay-pane evidence. Those limitations are
classified in `matrix.json`.
