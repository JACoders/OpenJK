# GP3-04 WAN/churn discovery

`run.sh` launches three isolated controlled dedicated-server cases against the
Phase 0 frozen split-screen binary. Each case performs 12 ordered secondary
leave/rejoin cycles and records exact lifecycle, party-state, survivor-health,
and client-slot assertions.

This bounded discovery is deliberately not the ticket's 100-cycle impairment
acceptance suite. The matrix labels missing packet impairment, timing,
snapshot-gap, input-routing, resource-growth, P1 reconnect, fifth-client, and
server-restart evidence as unsupported or not covered.

Run discovery validation with:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tests/splitscreen/gameplay/network/wan_churn/validate.py
```

Use `--require-acceptance` to obtain a nonzero exit until every acceptance cell
has evidence.
