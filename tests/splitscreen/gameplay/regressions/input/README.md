# GP4-02 input regressions

`test_source_regressions.py` locks the repaired source semantics without
requiring a rebuild:

- keyboard/mouse UI ownership follows its explicit player assignment and never
  silently falls back to another pane;
- SDL polling does not manufacture a new controller press edge every frame;
- detach/reassignment neutralizes axes, buttons, pending edges, console chord,
  and respawn suppression;
- GP3-05's primary value `2` is identified as `BUTTON_TALK` from an active key
  catcher, not mistaken for an attack-bit mapping defect.

Physical SDL detach/re-enumeration and process-level verification remain for a
post-build hardware run.
