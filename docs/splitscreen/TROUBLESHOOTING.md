# Troubleshooting

## The Split Screen Entry Is Missing

Confirm that `ui/jamp/splitscreen_start.menu`, `ui/jamp/splitscreen_players.menu`,
and the native ARM64 UI module exist under
`~/Library/Application Support/OpenJK/base`. Run the release `install.sh` again
if they do not.

## A Viewport Is Blank Or Duplicated

Set `r_fullscreen 0`, restart the renderer, and reproduce with a stock map. Save
a game screenshot with `screenshot_png name_without_spaces`. Renderer evidence
must be inspected visually; a successful command or nonempty file is not enough.

## One Device Moves More Than One Player

Return to the split-screen party screen and reassign every device. Each row must
name a different device. For diagnostics, set `ui_splitScreenTraceInput 1` and
inspect the console for `SplitInputTrace` entries; each event identifies one
player and controller slot.

## A Controller Does Not Navigate Menus

Verify macOS sees it, restart OpenJK after connecting it, and confirm
`in_joystick 1`. The D-pad and left stick navigate; A accepts and B returns. A
controller being captured for a Controls binding intentionally sends raw button
events until that binding completes.

## A Party Member Cannot Join

Check free server slots, password, mod/download requirements, and per-IP limits.
The console for that viewport shows its own challenge, connection, and rejection
state. Return to Join or Profile and retry; other players do not need to leave.

## Console Or Virtual Keyboard Input Goes To The Wrong Player

Use Back+Start on the affected controller. The console border and virtual
keyboard appear only in its viewport. Close all overlays, press Start in that
viewport, and reopen the desired tool if menu ownership was interrupted by a
disconnect.

## Logs And Clean Recovery

User data is in `~/Library/Application Support/OpenJK`. Preserve the log and
screenshots before testing a clean homepath. To recover without touching retail
assets, move that OpenJK directory aside and run `install.sh` again. Do not
delete or edit the retail `assets*.pk3` files.
