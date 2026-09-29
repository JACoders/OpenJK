# Assigned pre-fix evidence

- `GP4-02-INPUT-01`: GP1-02 recorded `ui_splitScreenInputTarget=0` immediately
  after mouse-owned P1 opened Force setup. The pre-fix key paths explicitly set
  that target to `"0"` on primary keyboard/mouse down and up.
- `GP4-02-INPUT-02`: GP1-03 recorded intermittent/repeated controller delivery.
  Source inspection found SDL cleared every player's buttons before repopulating
  connected devices on every poll. With pending-edge tracking, a held button
  therefore became a fresh edge every frame.
- `GP4-02-INPUT-03`: GP3-05 recorded P1 `actualButtons=2` for an expected attack
  bit of `1`, while controller panes produced `1`. Protocol constants prove
  value `2` is `BUTTON_TALK`; `CL_CmdButtons` adds it whenever P1 has an active
  catcher. This is a test-precondition/catcher-state finding, not evidence that
  MOUSE1 maps to the wrong attack bit.
