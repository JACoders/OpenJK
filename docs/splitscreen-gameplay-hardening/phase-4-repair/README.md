# Phase 4: Component-Owned Defect Repair

## Entry gate

Phases 1–3 finish discovery. Every reproducible defect has severity, owner,
minimal reproduction, failing assertion, affected matrix cells, and preserved
evidence.

## Parallel tickets

- [GP4-01: UI, menu, profile, and settings repairs](GP4-01-ui-settings-repair.md)
- [GP4-02: Input and device repairs](GP4-02-input-device-repair.md)
- [GP4-03: Session, network, and VM-transition repairs](GP4-03-session-network-repair.md)
- [GP4-04: Authoritative gameplay-rule repairs](GP4-04-gameplay-rules-repair.md)
- [GP4-05: Presentation, renderer, and audio repairs](GP4-05-presentation-audio-repair.md)

## Independence rule

The source and regression ownership below is exclusive. A cross-component bug
gets one primary owner; other agents provide diagnosis but do not patch the
primary's files. Shared build files are changed in a short serialized merge step
after all lane patches are ready.

## Exit gate

Every reproducible Phase 1–3 defect is fixed with a regression, the integrated
build passes all old and new suites, and the defect ledger contains no
unassigned or “cannot reproduce” entry without preserved counter-evidence.

## Result

COMPLETE — PHASE 5 FROZEN-BUILD CERTIFICATION IN PROGRESS.

All five component lanes are integrated. The focused repair regressions pass:

- GP4-01 UI/settings: 5
- GP4-02 input/device: 5
- GP4-03 session/network: 5
- GP4-04 gameplay rules: 6
- GP4-05 presentation/audio triage: 6

The existing rendering source contracts also pass (HUD 13, menu 6, scene 6).
The final integrated gate additionally passed the modal semantic analyzer (8),
controller binding contract (7), oracle fixtures (12), and harness self-test;
`git diff --check` reports no whitespace errors. Exact 2-, 3-, and 4-player
modal replays passed again after the quoted reliable-command repair on
executable
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
Phase 5 owns this rebuilt release candidate, the complete artifact manifest,
and every post-repair runtime assertion. A Phase 5 failure invalidates the
frozen results and reopens the responsible Phase 4 lane.
