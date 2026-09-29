# Executed gameplay coverage ledger

Run:

```sh
python3 tests/splitscreen/gameplay/coverage/generate.py
python3 -m unittest discover -s tests/splitscreen/gameplay/coverage -p 'test_*.py' -v
```

`ledger.json` is deliberately conservative. A lifecycle state is marked passed
only when an immutable log contains its declared assertion marker and at least
one hash-validated PNG accompanies the evidence record. Hash changes, missing
files, wrong binaries, contradictory claims, and absent markers are fatal.

The imported historical logs are tagged `legacy-unverified`: they do not embed
their binary hash. Their behavioral assertions remain useful, but the generator
marks every such cell `seal_eligible: false`; they cannot satisfy a future
frozen-build end-to-end or release-seal claim.
