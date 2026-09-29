# R3-04 top-menu capture

Captured on 2026-07-22 with the rebuilt x86_64 UI module and client, isolated
homepath `/private/tmp/openjk-r3-04-menu`, and ports 29402/29404.

Artifacts:

- `artifacts/cert_text_budget_top_menu_2p.png`
- `artifacts/cert_text_budget_top_menu_4p.png`
- matching `top_menu_2p.stdout.txt` and `top_menu_4p.stdout.txt`

Both runs asserted `ui_splitScreenMenuMode=top`. Visual inspection confirms
that every stock top-menu label from **About** through **Exit** is complete and
legible in each active pane. No label crosses a blue pane seam. The 4-player
labels are compact by design but are not reduced to one or two characters.
