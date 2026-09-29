# GP2-01 individual-scoring discovery matrix

The matrix runs FFA, Holocron, and Jedi Master with 2/3/4 local clients against
the frozen executable SHA-256 `223e8a1267…`. `proven` contains only literal
runtime assertions. Transitions such as respawn and rejoin require ordered
before/after assertions for the same player. Input receipt, a command present
in config text, and screenshot existence are only `attempted_smoke`.

Holocron pickup/holder state and Jedi Master role/saber state are explicitly
unsupported because no deterministic assertion hook exists. Intermission and a
remote fifth-client control are also unclaimed. The validator never upgrades
these partial cells to end-to-end coverage.

Discovery validation succeeds when the frozen manifests, arithmetic, and
classifications are structurally honest, even when the generated matrix status
is `failed`:

```sh
python3 tests/splitscreen/gameplay/modes/individual/validate.py \
  --results tests/splitscreen/gameplay/results/modes/individual
```

Use `--require-acceptance` for a release-style gate that exits nonzero while
any product-or-harness finding remains.
