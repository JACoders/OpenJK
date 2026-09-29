# GP3-03 controlled-server recovery discovery

`run.sh` uses the Phase 0 runner to create separate normal manifests for the
2/3/4-player cases. Each case launches the frozen native client plus the
existing native dedicated server, with isolated homes, ports, process groups,
logs, screenshots, and hashes.

Recovery passes require ordered party-state, lifecycle, and stable `clientnum`
assertions. An `ALIVE` observation by itself is never treated as identity or
rejoin proof.

```sh
./tests/splitscreen/gameplay/network/recovery/run.sh
./tests/splitscreen/gameplay/network/recovery/validate.py
```
