# Player Guide

## Device Assignment

The party screen offers keyboard and mouse plus every detected controller for
each player. A device can belong to only one player. The default two-player
device assignment is keyboard and mouse for Player 1 and Controller 1 for
Player 2.

Controllers are assigned by stable SDL slot for the session. Disconnecting and
reconnecting a device triggers the normal SDL discovery path; reopen the party
screen if the physical order changed.

## Screen Layout

Two-player parties can choose **Top / Bottom** or **Left / Right** on the party
screen. The choice persists between launches and applies to player
configuration, in-game menus, pointer confinement, and gameplay. Three- and
four-player parties use the fixed grid layouts shown by the party screen.

## Player Configuration

The setup screen renders one stock Player Configuration menu per viewport. It
is not a shared custom character picker.

- D-pad or left stick: move through the stock portrait and menu controls.
- A: activate the focused control.
- B: cancel or return.
- X: open that player's virtual keyboard for the focused name field.
- Left shoulder: open that player's stock saber setup.
- Right shoulder: open that player's stock Force setup.
- Y: spectate from the profile action row.
- Start: apply/join from initial setup, or open that player's top menu in game.

The virtual keyboard has three modes: normal text, a curated CVAR list, and a
cheat-command list. Cheats still obey the server's `sv_cheats` setting. D-pad
navigates, A selects, B cancels, and the labeled action keys switch mode,
backspace, accept, or close.

## In-Game Menus

Press Start to open the stock in-game top menu inside only that player's
viewport. About, Join, Profile, Add Bot, Controls, Setup, Vote, Call Vote, and
Exit use the stock menu implementations and act in the owning player's context.

Press Back and Start together to toggle that player's Quake console. Controller
players type with the virtual keyboard. Commands and archived split-player
CVARs are routed to that player's client context; engine-global renderer and
audio settings remain process-wide.

## Controls

Top Menu > Controls opens the complete stock controls setup in that viewport,
including movement, weapons, mouse/look, Force powers, and other actions.
Controller bindings have defaults and can be rebound by selecting an action and
pressing the desired button. Reset affects only the selected player's binding
table.

## Death And Spectating

Death follows stock Jedi Academy behavior in every viewport. When a mode shows
Player Configuration after death, that player can change profile or Force setup
and rejoin. Spectate and rejoin use the same server cooldown and team rules as an
ordinary network client. Other local players continue playing while one player
uses a menu, spectates, reconnects, or respawns.
