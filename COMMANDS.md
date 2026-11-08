# OpenJK-rpmod commands

The console commands, keys and settings this client adds. What each feature does is in [FEATURES.md](FEATURES.md).

- [Model Manager](#model-manager)
- [Model Manager keys](#model-manager-keys-free-camera)
- [NPC Manager](#npc-manager)
- [Shader Manager](#shader-manager)
- [Effect Manager](#effect-manager)
- [Effect Manager keys](#effect-manager-keys-free-camera)
- [Storyteller](#storyteller)
- [Storyteller keys](#storyteller-keys)
- [Cinematic Manager](#cinematic-manager-playing-it-for-others-is-experimental)
- [Cinematic Manager keys](#cinematic-manager-keys)
- [Map Editor](#map-editor)
- [Map Editor keys](#map-editor-keys)
- [Sound and Music Managers](#sound-and-music-managers)
- [Languages](#languages)
- [Radial Menus](#radial-menus)
- [Saber Styles](#saber-styles)
- [Special Moves](#special-moves)
- [Other tools](#other-tools)
- [Console and scripts](#console-and-scripts)
- [Settings](#settings)

## Model Manager

- `/modelmanager` opens or closes the manager (I recommend binding it to a key; `/modelplacer` still works)
- `/modelmanager <search>` opens the browser already filtered
- `/modelmanager edit` flies the free camera over the map's models
- `/modelmanager save <name>` saves the selected models as a construct
- `/modelmanager load <name>` picks up a saved construct to place it
- `/modelmanager names` lists the names you've given to models
- `/modelmanager remove <name>` removes the models with that name from the map
- `/modelmanager savestate <name>` saves the models you placed or named, and their names
- `/modelmanager reloadstate <name>` gives the models of a state their names back, after `rpdump load`
- `/modelmanager scale <percent>` sets the scale of the selection, or of new models
- `/modelmanager solid <0|1>` sets whether the selection, or new models, are solid
- `/modelmanager angles <pitch> <yaw> <roll>` sets the angles of the selection, or of new models
- `/modelmanager origin <x> <y> <z>` moves the selection there
- `/modelmanager list` reads the map's models from the server again
- `/modelmanager reindex` rescans for models, e.g. after installing a new pk3
- `/modelmanager close` closes the manager

## Model Manager keys (free camera)

- `Left click` select (Shift adds), or put down
- `Enter` put down and send all changes; `Shift+Enter` name it first
- `Right click / F` pick up; lock or follow the crosshair
- `P` follow onto what you aim at / stay in front of you; `Ctrl+Wheel` nearer or farther (Shift for 4x)
- `Wheel, Z, X, Q, E` turn: yaw, pitch, roll
- `R / Backspace` reset the angles / the angles and scale
- `[ ] or - +` scale (Shift for 5x)
- `T` toggle solid
- `Arrows, Page Up/Down` nudge
- `G` cycle the snapping presets
- `B` outlines on/off; hold `O` or the middle mouse button to outline the model you aim at
- `V` copy, `Del` delete, `U` undo
- `K / Shift+K` save the selection as a construct / save the state
- `I` send each change as you make it / on Enter
- `L` read the map's models again
- `WASD, Space, C` fly (Shift fast, Ctrl slow)
- `Tab` browser, `Esc` cancel a move, `H` hide the help

## NPC Manager

- `/npcmanager` opens or closes the manager (I recommend binding it to a key)
- `/npcmanager spawn | manage | routing` opens it on that tab
- `/npcmanager loadnpc <name>` puts one of your saved NPCs in the spawn form
- `/npcmanager loadconstruct <name>` picks up an NPC construct to put it down
- `/npcmanager talk <npc> <message>` the NPC (targetname or number) says it in chat (experimental)
- `/npcmanager select <npc> [name]` what you say in chat is said by that NPC, as that name; `/npcmanager select` alone shows who you talk through (experimental)
- `/npcmanager select off` talks as yourself again (`/npcmanager unselect` too)
- `/npcmanager walk <npc>[,<npc>...] <route> [walk|run|<speed>] [loop] [local]` walks the NPCs along a saved route, together if several; `local` on your screen only (experimental)
- `/npcmanager walk <npc>[,<npc>...] stop` stops them where they are; `/npcmanager routes` lists the saved routes
- `/npcmanager reindex` rescans for NPC types and models, e.g. after installing a new pk3
- `/npcmanager close` closes the manager
- `/npcnames` reads the NPC names and shows them under the crosshair to everyone with this client, you included, as they are now (admins, experimental); `/npcnames clear` takes them back

## Shader Manager

- `/shadermanager` opens or closes the manager (I recommend binding it to a key)
- `/shadermanager crosshair` shows or hides the shader under your crosshair (`cl_shaderCrosshair`)
- `/shadermanager remaps` lists the shaders remapped in this map
- `/shadermanager mine [reset]` lists your player shaders, or puts them all back
- `/shadermanager mine others` lists the player shaders others shared; `/shadermanager mine share` sends yours again
- `/shadermanager reindex` rescans for shaders
- `/shadermanager close` closes the manager

## Effect Manager

- `/effectmanager` opens or closes the manager (I recommend binding it to a key)
- `/effectmanager <search>` opens the browser already filtered
- `/effectmanager active | presets | mine` opens it on that tab
- `/effectreplacer` opens the Mine tab by itself, for anyone: your own force powers', saber's and weapons' effects played as others
- `/effectmanager place` places the selected effect in the world
- `/effectmanager edit` selects and moves the map's effects, and the ones waiting
- `/effectmanager bolt <tag>` bolts the selected effect to your own tag or bone
- `/effectmanager loadconstruct <name>` shows a saved construct, ready to use
- `/effectmanager names` lists the names you've given to effects
- `/effectmanager remove <name>` removes the effects with that name from the map
- `/effectmanager savestate <name>` saves the effects you sent or named, and their names
- `/effectmanager reloadstate <name>` gives the effects of a state their names back, after `rpdump load`
- `/effectmanager reindex` rescans for effects, e.g. after installing a new pk3
- `/effectmanager close` closes the manager

## Effect Manager keys (free camera)

- `Left click` send, or put down to wait; while moving: select (Shift adds), or put down
- `Enter` send what's waiting; `Shift+Enter` name it first
- `Right click / F` lock or follow the crosshair
- `Wheel, Z, X, Q, E` turn: yaw, pitch, roll
- `R` straight up (no angles), `N` face away from surfaces
- `M` once / loop
- `Arrows, Page Up/Down` nudge
- `G` cycle the snapping presets
- `V` switch between placing effects and selecting/moving them
- `Del` remove, `U` undo
- `K` save the selection as a construct
- `I` send on putting down / on Enter
- `L` read the map's effects again
- `WASD, Space, C` fly (Shift fast, Ctrl slow)
- `Tab / Esc` back to the manager, `H` hide the help

## Storyteller

- `/storyteller` opens or closes the storyteller (I recommend binding it to a key)
- `/storyteller play [name]` plays the story, or loads that one and plays it
- `/storyteller continue` goes on past a "wait for me" step (bind it to a key)
- `/storyteller stop` stops playing
- `/storyteller save <name>` saves the story; `/storyteller load <name>` loads one; `/storyteller list` lists them
- `/storyteller open <name>` opens a saved story in a tab of its own
- `/storyteller markers [on|off]` draws where the armed stories wait, on your screen
- `/storyteller arm <name> [again|<times>]` arms a saved story: it plays when its first step is set off, once, every time with `again`, or that many times
- `/storyteller disarm [name|all]` disarms one, or all; `/storyteller armed` lists the armed stories
- `/storyteller close` closes the storyteller
- `/objective [list]` lists the objectives; `/objective add|done|undone|remove <text or number>`, `/objective clear [done]` and `/objective title [text]` change them for everyone with this client (experimental)

## Storyteller keys

- `Up/Down` select a step, `Ctrl+Up/Down` move it
- `Ctrl+click / Shift+click` pick several; `Ctrl+C, Ctrl+X, Ctrl+V` copy, cut, paste; `Ctrl+A` pick all
- `Del` delete them, `Tab` next box
- `Esc` back / close
- In the diagram: the wheel, `Up/Down`, `Left/Right` and `PgUp/PgDn` scroll, `Home` back to the top, `Esc` back

## Cinematic Manager (playing it for others is experimental)

- `/cinematicmanager [name]` opens the manager, on that cinematic or a new one by that name
- `/cinematicmanager send <name>` plays a saved one for everyone; `/cinematicmanager play <name>` on your screen only, to try it
- `/cinematicmanager video <name>` plays a saved one on your screen and records it to `videos/<name>-001.avi` in the game folder, at `cl_aviFrameRate` frames a second (25 unless you set it)
- `/cinematicmanager stop` stops it on your screen, `/cinematicmanager stop all` for everyone
- In the manager: **Ctrl+Z** undoes, **Ctrl+Y** (or Ctrl+Shift+Z) redoes, **Ctrl+C** copies what's picked, **Ctrl+V** pastes it at the moment picked
- `/cincam play <shot>` plays a camera shot made before the manager
- `/cincam here` prints the spot you look from, as x y z pitch yaw roll fov
- `/cincam stop` leaves the cinematic, on your screen
- `/cincam anim <who> <ANIMATION> [from] [to] [play|reverse|hold] [loop] [speed %] [seconds]` tries an animation on your screen only, e.g. `/cincam anim me BOTH_DEATH1 0 -1 play loop 50`; `/cincam anim stop` lets them go
- `/cincam [local] start | path | move | cut | orbit | lookat ...` drive the camera by hand; `/cincam` alone lists them all

## Cinematic Manager keys

- `Hold right mouse` look and fly; `WASD, Space, C` move (Shift fast, Ctrl slow)
- While flying: `wheel` zooms, `Q/E` roll, `R` levels
- Camera tab: `Enter` add a spot, `Del` delete it, `Up/Down` pick one, `F` look through it
- Entities tab: `Enter` add a spot to the move picked at the crosshair, or a key to the position; a click picks a spot or a key, a drag moves it, `Del` takes it off
- Effects and Sounds tabs: a click on one in the view picks it, a drag moves it over walls and floors, `J` move arrows, `Del` deletes it
- While recording: `F10` (or `cl_cinRecordKey`) ends it
- `P` or `Space` preview, from the timeline's moment if one is picked; `T` hide or show the timeline
- `Esc` lets go of the timeline's moment, then goes back

## Map Editor

- `/mapedit` opens or closes the map editor (I recommend binding it to a key)
- `/mapedit save [name]` keeps the edits in the layout loaded, or as a layout of that name
- `/mapedit load <name>` loads a layout in place of the edits shown
- `/mapedit layouts` lists the layouts kept for this map
- `/mapedit clear` takes away the edits shown
- `/mapedit share` sends your edits to everyone on the map with this client; `/mapedit unshare` takes them back (experimental)
- `/mapedit on | off` shows the edits outside the editor too, or not

## Map Editor keys

- `Hold right mouse` look and fly; `WASD, Space, C` move (Shift fast, Ctrl slow); `wheel` zooms while flying
- `Left click` pick, a map prop too
- `G` move it, `Del` delete it, `Ctrl+D` copy it
- `Ctrl+S` save, `Esc` back

## Sound and Music Managers

- `/soundmanager` opens or closes the sound manager; `/musicmanager` the music manager (I recommend binding them to keys)
- `/soundmanager <search>`, `/musicmanager <search>` open them already filtered
- `/soundmanager list` lists the looping sounds playing (`rpsound list`)
- `/soundmanager reindex`, `/musicmanager reindex` rescan for sounds and music
- `/soundmanager close`, `/musicmanager close` close them
- Keys: `Up/Down` pick, `Left/Right` folder, `double-click` or `Ctrl+Enter` preview, `Enter` send / play, `Tab` switch tab

## Languages

- `/languages` opens the window for all of it
- `/language` shows which character you play, what it knows and what it speaks
- `/language select <language>` speaks it; `/language basic` speaks Basic again
- `/language say <language> <text>` says one line in it
- `/language add <language> [understand]` your character speaks it, or only understands it; `/language remove <language>`
- `/language character <name>` plays as that character, made if new; `/language characters` lists them

## Radial Menus

- `+radial [menu]` on a key: hold to open the menu, point and let go to run an item, e.g. `/bind mouse4 "+radial tools"`
- `/radial [menu]` opens a menu that stays open, or closes the one open
- `/radialsettings` makes menus, fills them and binds keys to them
- While a menu is open: `1-9, 0` run that item, `left click` runs the one pointed at, `right click / Esc` close

## Animation Manager

- `/animationmanager` opens or closes it (bind it to a key)
- `/animationmanager anim <ANIMATION>` opens it on that animation; `/animationmanager chains [name]`, `/animationmanager custom [name]` on those tabs
- `/animationmanager stop` stops the chain and the custom emote playing; `/animationmanager list` lists them; `/animationmanager reload` reads the animations again and lists each skeleton, the pk3 its animations come from, and how many it added or changed
- `/em<name>` does a chain or plays a custom emote made in it; again stops it
- In it: `Up/Down` the next animation, step or bone, `Space` pause, `Esc` back / close

## Presets

- `/presets` opens or closes them (the in-game menu's PRESETS)
- `/presets save` saves who you are now as a preset; `/presets list` lists them, numbered (`*` a favorite); `/presets <number>` makes you that one
- `/skinselect` the skins of your model, to wear in a click; `/skinselect <model>` another model's
- `/assetmanager` what's on the map and the pk3 each is from; `/assetmanager scan` asks the server again; `/assetmanager list` prints it; `/assetmanager build [name]` packs the custom ones checked into `assetpacks/<name>.pk3`
- `/quickconnect connect <1-4>` goes to JEDI Temple, Grounds, Region or Galaxy; `/quickconnect refresh` asks them for their map and players; `/quickconnect` lists them
- `/adminflags` noclip, nodraw, nonsolid and notarget as known for you; `/adminflags reset` all taken as off
- `/campreset save|load|delete [name]` the camera settings saved or loaded by name (the camera panel's box when none is given); `/campreset next|prev` loads the next saved one or the one before; `/campreset list`

## Saber Styles

- `/saberstyles <style> <style> ...` sets the styles saberAttackCycle goes through: fast, medium, strong, desann, tavion
- `/saberstyles all` goes back to the server's own cycle; `/saberstyles` alone shows the list
- `/saberstyle <style>` switches straight to that style

## Special Moves

- `/specialmoves` opens or closes the special moves menu; `/specialmoves list` lists them in the console with how they're done
- `/specialmove <name>` plays a move: its keys pressed for you for a moment. Bind keys to it, e.g. `bind k "specialmove kata"`. Names: `kata`, `lunge`, `spinattack`, `dfa`, `flipslash`, `staffleap`, `dualleap`, `stabdown`, `backattack`, `backattackcrouch`, `backflipattack`, `cartwheelleft`, `cartwheelright`, `butterflyleft`, `butterflyright`, `kickfront`, `kickback`, `kickleft`, `kickright`, `rollstab`, `rollforward`, `rollback`, `rollleft`, `rollright`, `backflip`, `flipforward`, `flipleft`, `flipright`, `wallrunleft`, `wallrunright`, `wallflipleft`, `wallflipright`, `wallrunup`, `wallgrab`
- `cl_specialMovesDebug 1` prints each step of a move played and what the game shows

## Other tools

- `/escmenu [modern|kotor|classic] [category]` opens the Esc menu in that style, on that category or tab (`/escmenu kotor options`); `cl_escMenu` picks what Esc opens: 0 the game's own (default), 1 Modern, 2 KOTOR
- `/chatlog` opens or closes the chat log window; `/chatlog start` and `/chatlog stop` record or stop on this server; `/chatlog open` opens today's page in your browser, `/chatlog folder` the folder
- `/datapad` opens your datapad in the bottom right corner, or puts it away; `/datapad open`, `/datapad close` and `/datapad settings` go straight there. Its Settings set the keys that open and close it; Esc puts it away
- `/datapad holonet [n]` opens the HoloNet on it (the n-th article); `/datapad comport [guest|<forum number>]` the Comport (its forums, or that one)
- `/datapad pazaak` opens Pazaak on it; `/pazaak create [name]`, `/pazaak join <table>`, `/pazaak watch <table>`, `/pazaak end`, `/pazaak stand`, `/pazaak play <card 0-3> [1|-1] [1|2]` and `/pazaak leave` do what its buttons do, for binds; `cl_pazaakDeck` is the side deck you play with
- `cl_datapadColor` (0-6) is its colour; `cl_datapadEmote 1` has your character hold a datapad (`/emlift`) while it's open
- `/datapad pos <x> <y>` moves where it opens on the 640x480 screen (its top left; `cl_datapadX`, `cl_datapadY`); `/datapad pos` puts it back above the HUD
- `cl_chatLog 1` records by itself on joining a server; `cl_chatLogTells`, `cl_chatLogTeam`, `cl_chatLogWhispers`, `cl_chatLogOOC`, `cl_chatLogEmotes`, `cl_chatLogAdmin` (1 or 0) keep those kinds of chat or not

- `/toolaccess` shows whether the admin tools open for you on this server, and on Temple whether your rank may turn experimental features on; `/toolaccess check` asks again
- `/feedback` sends a bug report or an idea for the OpenJK-RPMod project, with your RPMod account, and shows the answers; `/feedback mine` opens on your reports, `/feedback close` closes it
- `/c1b3r` shows your link to C1B3R: connected, linked, approved by staff, and the RPMod account you're on
- `/c1b3r link` links this game, logged in to your RPMod account: your browser opens, you log in with Discord there, and staff approve it. Then you send what the tools share. The code it shows also works as `/link <code>` in Discord
- `/c1b3r discord` opens the page to add C1B3R to your Discord account, so `/link` works in a DM without being in the project's Discord
- `/c1b3r unlink` ends the link
- `/clientusers` shows who on this server has this client, who's linked and who sends: the players the tools' hooks go to
- `/version` shows your OpenJK-rpmod version and whether it's the newest, with the link to a newer one; `/version check` asks GitHub again
- `/rpexperimental [on|off]` turns the experimental features on or off, and keeps them so every time the game starts; on Temple only for Department Head, Council and God, and turned off for anyone else
- `/emotepreview` lists the emotes the Help menu can preview; `/emotepreview forget <emote>` learns one again; `/emotepreview watch` prints your animation's changes after an emote

## Console and scripts

- `Drag` select, `double-click` a word, `Ctrl+C / right click` copy, `right click` with nothing selected paste
- `Ctrl+F` (mouse over the console) find; `Enter, Up/Down` next and previous, `Tab` back to the input line, `Esc` close it
- `/colorcodes [text]` prints your name, or the text, with its color codes visible
- `/con_keybind <key>` the key that opens the console; `/con_keybind none` takes it away; `/con_keybind` alone shows it
- `/execlist` lists the scripts still running
- `/execstop [file]` stops all running scripts, or only that one
- `/loadpk3 [name]` loads pk3s added to the game's folders while it runs, without a vid_restart

## Settings

- `rpc_removeExtraColorsInConsole 0` shows RPMod's extra colors in the console
- `rpc_chatRangeConsole_mode <0-3>`, `rpc_chatRangeConsole_min`, `rpc_chatRangeConsole_max`, `rpc_chatRangeConsole_alpha` set the chat range in the console
- `cl_updateCheck 0` never asks GitHub whether there's a newer OpenJK-rpmod
- `cl_feedbackUrl` is where /feedback sends reports and asks for their answers
- `cl_c1b3rToken` is this game's link, made by `/c1b3r link`: keep it to yourself
- `cl_presenceUrl` is C1B3R's address: the link to everyone with this client, through which the tools' hooks go; empty, nothing is shared and hooks show on your screen only
- `renderersettings sync` / `apply`: what Setup's RENDERER page uses to show the renderer settings and to set them at the next `vid_restart`
- `cl_renderer rd-vulkan` then `vid_restart` draws the game with the Vulkan renderer *(experimental)*, `cl_renderer rd-vanilla` with the usual one; the Vulkan one's own settings (`r_fbo`, `r_bloom`, `r_ext_multisample`, `r_vbo`, `r_ext_supersample`, `r_ssao`, `r_softParticles`, `r_fxaa`, `r_sharpen`, `r_sunShafts`, `r_tonemap`, `r_motionBlur` and more) are in [FEATURES.md](FEATURES.md#vulkan-renderer-experimental)
- `cl_whisper 1` shows whispers (chat in ^k) only to those close by, `cl_whisperHear` and `cl_whisperNotice` set how close
- `cl_shaderCrosshair 1` names the shader under your crosshair
- `cl_fighterTakeoffGuard 0` turns the fighter takeoff guard off
- `+vehboost` fires a fighter's boost on its own key
- `cl_crouch 1` keeps you crouched while on foot
- `s_backgroundAudio 1` the sound keeps playing with the game minimized or another window in front (Setup > Sound > Background Audio); during a cinematic it always does
- `cl_ratioFix 1` turns the widescreen text fix on
- `cl_saberStyles` holds the styles set with `/saberstyles`
- `rpc_cinematics 0` keeps cinematics sent through RPMod from taking your camera
- `rpc_mapEdits 0` only shows your map edits inside the editor
- `rpc_mapShare 0` ignores map edits other players share
- `cl_storyMarkers 0` hides where the armed stories wait
- `cl_npcNames 0` hides the NPC names under the crosshair
- `cl_customEmotes 0` hides the custom emotes admins share
- `cl_discord 1` your Discord profile shows what you're playing (off by default); `cl_discordPrivate 1` your Discord status leaves out the server and your character's name
- `cl_adminFlags 1` shows the noclip, nodraw, nonsolid and notarget line (*0* by default)
- `ui_cameraSettings 1` shows the camera settings beside the in-game menu (Setup > Game Options > Camera Settings)
- `cl_modelFollowFront 1` keeps what's in the modelmanager's hand in front of you; `cl_modelHoldDistance` how far (128)
- `cl_wallStick 1` keeps you on a wall you grabbed until you press jump again (`/cl_wallstick`)
- `cl_cinRecordKey F10` the key that ends a recording in the cinematic manager
- `cl_safeModels 1` shows map objects with too many verts as missing models; `cl_safePlayerModels 1` shows such player models as Kyle
- `cl_shaderShareMine 1` shares your player shaders (the Mine tab's Shared)
- `cl_effectShareMine 1` shares the effects you swapped in the Effect Replacer (its Shared, on by default)
- `cl_rpExperimental 1` experimental features on as the game starts (`/rpexperimental`)
- `con_modernInput 1` the console's line goes on to more rows, with word jumps, selection and copy and paste
- `cl_chatSplit 1` a chat too long for the server is sent in parts; `cl_chatSplitLength`, `cl_chatSplitMark`, `cl_chatSplitDelay`, `cl_chatSplitCommands`
- `cl_chatCapital 1` chat and each sentence in it start with a capital; `cl_chatCapitalEmotes 1` each emote's first letter too; `cl_chatCapitalCommands` the commands it works in
- `cl_consoleTildeKey 0` the key under Esc is an ordinary key (`/con_keybind` sets the console's own)
- `cg_saberlight 1` blades light what's around them, pulsing; `2` only your own
