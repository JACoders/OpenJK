# GP3-01 local-host/LAN discovery

This discovery launches a real listen server with 2/3/4 attached split clients
and a separate one-player loopback client under the Phase 0 process contract.
Only ordered runtime assertions enter `proven`. Direct `devmap`, configured
server settings, process exit, and screenshot existence remain smoke.

```sh
tests/splitscreen/gameplay/network/local_host/run.sh
python3 tests/splitscreen/gameplay/network/local_host/validate.py \
  --results tests/splitscreen/gameplay/results/network/local_host
```

Default validation exits zero for an honest discovery report.
`--require-acceptance` fails while findings or unsupported acceptance cells
remain.
