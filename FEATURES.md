# OpenJK-rpmod features

Everything this client adds on top of OpenJK. The commands and settings are in [COMMANDS.md](COMMANDS.md), the changes from version to version in [CHANGELOG.md](CHANGELOG.md).

Some features reach other players through workarounds only this client understands; they are marked *(experimental)* and listed under [Experimental features](#experimental-features).

- [Model Manager](#model-manager)
- [NPC Manager](#npc-manager)
- [Shader Manager](#shader-manager)
- [Effect Manager](#effect-manager)
- [Storyteller](#storyteller)
- [Cinematic Manager](#cinematic-manager-playing-it-for-others-is-experimental)
- [Map Editor](#map-editor)
- [Sound and Music Managers](#sound-and-music-managers)
- [Languages](#languages)
- [HD Video Cinematics](#hd-video-cinematics)
- [Radial Menus](#radial-menus)
- [Saber Styles](#saber-styles)
- [Emote Preview](#emote-preview)
- [Animation Manager](#animation-manager-custom-emotes-are-experimental)
- [Presets](#presets)
- [Discord Status](#discord-status)
- [In-game Menu and Camera Settings](#in-game-menu-and-camera-settings)
- [Widescreen Text](#widescreen-text)
- [Console Selection and Find](#console-selection-and-find)
- [Lists and Text Boxes](#lists-and-text-boxes)
- [Long Chat in Parts](#long-chat-in-parts)
- [Chat Capitals](#chat-capitals)
- [Console Key](#console-key)
- [Saber Light](#saber-light)
- [NPC Names](#npc-names)
- [RPMod Colors in the Console](#rpmod-colors-in-the-console)
- [Chat Range in the Console](#chat-range-in-the-console)
- [Whispers](#whispers)
- [Background Exec](#background-exec)
- [Load a pk3 while playing](#load-a-pk3-while-playing)
- [Fighter Takeoff Guard](#fighter-takeoff-guard)
- [Toggle Crouch](#toggle-crouch)
- [Wall Stick](#wall-stick)
- [Console Settings](#console-settings)
- [Model Safeguards](#model-safeguards)
- [Update Check](#update-check)
- [Feedback](#feedback)
- [Vulkan Renderer](#vulkan-renderer-experimental)
- [Experimental features](#experimental-features)

## Model Manager

Place new models, and select, move, copy and delete the ones already in the map, all from a free camera.

### Model browser

![](https://i.ibb.co/bRL26Kxz/openjk-x86-2026-10-07-18-36-22-277.png)

- Lists every model your game can find, sorted into *Base* (the game's own files) and *Custom* > pk3 > folders
- Filter buttons to show everything, only Base or only Custom
- Type to search across all folders at once
- Preview the selected model before you place it
- Your saved constructs and states are listed here too
- Click the star by a model to make it a **favorite**; the Favorites row lists them all

### Placing and editing

![](https://i.ibb.co/YF9wQZ38/openjk-x86-2026-10-07-18-36-44-962.png)

- Fly a free camera around the map while a ghost of the model follows your crosshair and sits on whatever surface you aim at
- Or press P to keep it in front of you instead, so it doesn't jump far away when you look into the distance; Ctrl+Wheel brings it nearer or farther, and it still stops on a closer wall
- The map's own models are read from the server: click one to select it, Shift+click to select several
- Pick up the selection and carry it, turn it (pitch, yaw, roll), scale it from 1% to 1000%, nudge it with the arrow keys and Page Up/Down, and switch it between solid and non-solid
- Copy (V), delete (Del) and undo (U)
- Press J for move arrows: red X, green Y and blue Z over the selection, and the mouse becomes a cursor. Drag an arrow and the selection moves along that axis only, on the snap grid; hold the right mouse button to look around, J again to hide them
- Five snapping presets, from 1°/1 unit up to 90°/32 units, for neat alignment
- Send each change as you make it, or queue them all and send them with Enter (I switches between the two)
- Everything is sent as `rpmodel add` and `rpmodel remove` for you, so your usual RPMod permissions apply. RPMod can't move a model, so a moved model is removed and added again

### Outlines

- Selected models are outlined in color
- B turns the outlines off and on, for a clean look at the scene
- With outlines off, hold O or the middle mouse button to outline only the model you aim at

### Names, constructs and states

- Shift+Enter gives a name to what you place or send, e.g. "bridge", and `/modelmanager remove bridge` removes all of it later
- K saves the selection as a **construct**: a group of models you can place again anywhere, in any map
- Shift+K saves a **state**: the models you placed and their names. RPMod doesn't keep the names, so after an `rpdump load`, reload the state to get them back

## NPC Manager

Spawn NPCs with a live preview, and run every `/npc` command on the NPCs in the map without typing a single targetname.

### Spawn tab

![](https://i.ibb.co/YTTW41BS/Screenshot-2026-10-09-134607.png)

![](https://i.ibb.co/Zzf0wDrs/Screenshot-2026-10-09-134659.png)

- Lists every NPC type your game can find (characters and vehicles), sorted into *Base* and *Custom* > pk3 > .npc file, with filter buttons and a search box
- Set a targetname, a team (default, player or enemy), a custom model or model/skin and a scale
- Browse every model and skin with a turning 3D preview
- Spawn it in front of you, or pick the spot with a free camera, anywhere on the map, and turn it with the mouse wheel
- Put down several NPCs, then spawn them all at once. They're sent one at a time so the server's flood protection doesn't drop them
- Preview the NPC before spawning it: its model, skin, scale, tint and the weapon or sabers it holds
- Save the form's settings as an NPC of your own, to spawn again later with one click
- Save NPCs you've put down as an **NPC construct**, to put the whole group down again anywhere
- Click and it's done: the manager sends the matching `npc spawn` command for you, so your usual RPMod permissions apply

### Manage tab

![](https://i.ibb.co/rG0mPTmt/Screenshot-2026-10-09-134731.png)

- Lists every NPC in the map with its number, type and targetname, and how far it is from you
- Select an NPC in the list, or click its label on the map
- Apply each command to that NPC, to every NPC sharing its targetname, or to all NPCs
- Kill (with a confirm click), kill spawners, freeze/unfreeze, set or stop an emote, follow you or another player, set a dialog, hologram on/off, and show the kill score
- Teleport to a spot you click, in front of you, or to exact coordinates (X, Y, Z and yaw)
- All of RPMod's emotes in a searchable list
- **Talk** *(experimental)*: make the selected NPC say a line in chat, as a player would, with the name and name color you choose and the emtalk emote if you want it
- The server's reply to each command shows right in the window

### Routes *(experimental)*

- Draw a route on the map (Walk row, *Route on map...*) and the NPC walks it, at a walk or a run, once or round and round, its legs moving and turning at each spot
- Routes are saved by name, to walk again with `Again`, or from the Storyteller's Walk NPC step
- Mark several NPCs with Ctrl+click, in the list or on the map, and apply to `Marked`: they walk the route together, each keeping its place around the others, short of walls
- Players with this client see them walk; the others see them hop from spot to spot, as the real NPC is teleported along behind
- A route is unhooked from an NPC that dies or is gone from the map, and only NPCs can walk one

### Routes tab *(experimental)*

- Every NPC in the map with the route it walks, or none; show all of them, only the ones walking, or the ones that aren't
- Give a saved route to one NPC, to those named alike, or to the marked ones (Ctrl+click) together; walk or run, once or round and round; stop one or all
- Make new routes, change a saved one's spots, or delete it
- The routes being walked now, and who walks them; a click selects the NPC
- **Routes on map...**: a free camera showing every route being walked, each in its own color, its spots numbered and its NPC named where it is. Hide the outlines (O), or show only the one you hover (F). Click an NPC or its route to select it

### Talking through an NPC *(experimental)*

- `/npcmanager select <npc> [name]` and everything you say in chat is said by that NPC instead, as the name you give it (RPMod colors work, e.g. `^qDarth ^7Revan`)
- It lasts until `/npcmanager select off`, or until the map changes
- `/npcmanager talk <npc> <message>` says a single line through the NPC
- The NPC is its targetname or its number from the Manage tab; without a name it talks as its targetname
- Team chat and private messages stay your own

## Shader Manager

Click any surface, see what it's made of, and swap its texture for everyone on the server.

### Finding shaders

![](https://i.ibb.co/vxRPtwTq/Screenshot-2026-10-09-134757.png)

- Fly a free camera around the map and click any surface to list its shaders
- Works on everything you can see: map surfaces, curved patches, models baked into the map, doors and movers, md3 and Ghoul2 models (players and NPCs), and effects
- Optional crosshair readout that names the shader you're looking at, even while playing

### Remapping

![](https://i.ibb.co/sv4Zxn9q/Screenshot-2026-10-09-134823.png)

![](https://i.ibb.co/dsXwyZh5/Screenshot-2026-10-09-134831.png)

- Pick a replacement from every shader your game can find, with a search box and filters (textures, models, gfx)
- Click the star by a shader to make it a **favorite**; *Faves* shows only those, among the replacements (skies too) and among the shaders on what you clicked
- Or press `Path...` and type any shader or image path yourself, e.g. *effects/gfx/force2*; the manager tells you whether your game has it
- See it on the surface on your own screen first, before anyone else does
- Click and it's done: the manager sends the matching `rpshader remap` command for you, with an optional time offset
- A sky can only be swapped for another sky, so a remap can't break the map's sky
- A list of everything remapped in the map, and an undo for your own remaps

### Overlays

- Search a shader and draw it over everyone's screen with `rpshader overlay add`: where it goes and how big on a 640x480 screen (empty for the whole screen), and its fade
- Preview it where it would go, on your screen only, before you add it
- List the server's overlays, and remove one or all of them

### Sky

- Tint the map's sky with red, green and blue sliders
- Spin it slowly at the speed you choose, for a moving sky, in step on everyone's screen
- Turn it round, tilt and roll it, to put the planet or the sun's glow where you want them
- Zoom it, from 0.25x to 4x, to make the sky look nearer or farther; a 3D skybox or sky portal zooms its view
- On a map with a 3D skybox, the whole skybox turns, and can be moved too
- Shown on your screen as you change it; **Share with all** *(experimental)* sends it to everyone with this client until the map changes, **Reset for all** puts the map's sky back
- Click a sky in the free camera and *Sky look...* takes you there. *Needs the release's renderer, OpenGL (rd-vanilla_x86.dll) or Vulkan (rd-vulkan_x86.dll).*
- **Sky portal**: the map seen from a spot in place of the sky, as a 3D skybox does, even on a map without one. Pick the map's own 3D skybox, or add the free camera's spot; the spots are kept per map. Turn and move it with the sliders above, and Share with all sends it too

### Player shaders (Mine tab)

- Change the shaders of your own force powers, sabers and shields, e.g. a white force lightning, from a list sorted into Force, Sabers and Shells
- Only yours change: someone else using the same power keeps the standard look on your screen. No command is sent
- Live shows the new one on your own as you pick it; they're kept for every map and server
- Click a player effect in the free camera and *Only mine...* takes you there
- **Shared** *(experimental)*: everyone with this client sees yours on you, sent as you change them, as a map starts and when someone joins. It uses `rpnotify`, so it needs an admin's rights

## Effect Manager

Browse every effect, watch it play live, put it exactly where you want (or on someone's hand), move it later, and clean up afterwards.

### Effect browser

![](https://i.ibb.co/vxR7xLYh/Screenshot-2026-10-09-134933.png)

- Lists every effect your game can find, sorted into *Base* and *Custom* > pk3 > folders, with filter buttons
- Type to search across all folders at once
- Click the star by an effect to make it a **favorite**; the Favorites row lists them all
- The selected effect plays live in a window onto the world, on your screen only
- Choose once or loop, and where it plays: where you stand, at a position, or bolted to someone

### Placing

![](https://i.ibb.co/jkd1VjJg/Screenshot-2026-10-09-134953.png)

- Fly a free camera around the map while the effect plays where your crosshair points
- Lock it in place, then fine-tune it with the arrow keys and Page Up/Down, or press J for move arrows and drag the X, Y or Z one with the mouse
- **Size** *(experimental)*: [ and ] (or - and +) make the effect bigger or smaller before you send it, from 0.1x to 8x, also next to Once/Loop in the manager. Everyone with this client sees it that size (sent hidden in an rpnotify, again a little after someone joins); others see it as made
- Rotate it (pitch, yaw, roll), or let it face away from the wall or floor you aim at
- Send each effect as you place it, or put down several and send them all together
- Click and it's done: the manager sends the matching `rpeffect once/add` command for you, so your usual RPMod permissions apply

### Bolting

![](https://i.ibb.co/F4XKG0C1/Screenshot-2026-10-09-135023.png)

![](https://i.ibb.co/wNXSMQHn/Screenshot-2026-10-09-135035.png)

![](https://i.ibb.co/S7XwcypF/Screenshot-2026-10-09-135048.png)

- Pick yourself, a player or a named NPC
- See their current model and skin, turning, with the bolt point marked on it
- Choose from that model's own tags (\*r_hand, \*back...) and bones
- Watch the effect on the real player or NPC before sending it
- Bolt several effects, then send them all together

### Moving effects

- Press V in the free camera to select effects, one or several, both the ones waiting to be sent and the ones already playing
- Carry, turn and nudge them, remove them, and undo
- J shows the move arrows over the selection here too; [ and ] change the selected ones' size
- Effects already playing are moved with `rpeffect remove` and `rpeffect add` for you

### Names, constructs and states

- Shift+Enter names a looping effect, or a selection, and `/effectmanager remove <name>` removes all of it later
- K saves the selection, or what's waiting to be sent, as an **effect construct** to place again anywhere
- Constructs can hold bolted effects too: the player or NPC isn't saved, so you pick who wears them each time you use it
- States work like the Model Manager's: save one before `rpdump`, reload it after `rpdump load` to get the names back

### Active effects and presets

![](https://i.ibb.co/mrq6LvXZ/Screenshot-2026-10-09-135113.png)

- Lists every effect playing in the map, with its number, position and distance from you
- Check effects in the list or by clicking their labels on the map, then remove the checked ones in one go
- Make the checked ones (or the one picked) bigger or smaller with Size *(experimental)*, or back to x1
- Remove every effect, or everything bolted to you, a player or an NPC
- One-click presets: alarm, fade out / fade back in, and camera shake with intensity and duration
- Several commands in a row are spaced out automatically, so the server's flood protection doesn't drop them

## Storyteller

Assemble a mission out of steps, then play it: NPCs, models, effects, sounds, music, chat and cinematics, one after the other.

### Steps

![](https://i.ibb.co/cc1QJSmd/openjk-x86-2026-10-14-20-17-25-677.png)

- **Flow**: wait a number of seconds, "wait for me" until you press Continue (or `/storyteller continue`), a "when" for each kind of trigger (someone enters a map trigger, comes near a spot or an effect, a door moves, someone says something or uses a use trigger), If / Else / End if under a "when", play another saved story alongside (**Play story**), and run any console command
- **Speech**: say a line in chat, as you or as one of the players on the server *(experimental)*, a center message for everyone or one player, a notification with a countdown, NPC talk and objectives *(experimental)*
- **NPCs**: spawn an NPC, teleport NPCs, walk them along a route *(experimental)*, and any `npc` command on them
- **World**: place a model, play an effect, a screen effect (fade, shake...), remap a shader, a screen overlay, the sky's look *(experimental)*, lock or unlock a door, play a sound, change the music
- **Cinematic** *(experimental)*: play a cinematic made in the Cinematic Manager, and wait for it to end
- Fill most steps in by picking straight from the NPC, Model, Effect, Shader, Sound and Music Managers
- Turn steps on and off, reorder them with Ctrl+Up/Down
- Commands are spaced out so the server's flood protection doesn't drop them, and a story keeps playing with the window closed
- Save stories by name and load them again later

### Conditions and timers

- **If** goes under a "when" step and looks once, when the story gets to it: how many players, NPCs or people are in a map trigger or near a spot or an effect (at least, at most or exactly so many), whether someone has said one of the words you give since the story began, or **who set off** the "when" above it, by name (the whole of it, or a part of three letters or more). True, the steps after it play; false, the ones after its **Else**. **End if** closes it, and Ifs can sit inside Ifs. If, Else and End if are greyed out until there's a "when" above where they'd go; one left above every "when" shows in red, and the story stops there
- A "when" on an area can wait for them to **stay** in it a number of seconds; "When someone says" waits for one of the words you give (`|` between them)
- "When someone uses a trigger" waits for one of the Map Editor's use triggers, the one you look at or pick in a free camera (a held one once it's been held long enough); If can ask whether someone used it too
- Its **Too early** keeps the story in order: while the story is at an earlier step (or waits armed for its first), the use trigger is hidden from the players, or shows them its message instead of being used, until the story gets there. Dinner after breakfast: one story, "When someone uses breakfast", then "When someone uses dinner" with Too early set. For players with this client, while the storyteller is on the map
- **Play story** plays a saved story alongside the one playing, which goes on: from its start, or once its own first "when" is set off. "When someone uses lever", then "Play story Story2", and the lever starts Story2
- **Diagram** shows the story as a flowchart: the "whens", each If splitting into a yes and a no side that join again at its End if, and the stories it plays; click a step to pick it, or a Play story to open that story's diagram. The wheel or the arrows scroll, Esc goes back
- Map triggers and doors are picked where you stand or aim, in a free camera, or from a list
- The **Notification** step shows a line in the top left corner with `rpnotify`, for everyone or one player; give it a countdown and it shows the seconds left, live (`<timer>`)
- The **Lock** step locks a door or a trigger to a rank with `rplock`, or unlocks it, picked in the Map Editor's Locks tab

### Tabs and multiselect

- Open several stories at once, each in a numbered tab; the story playing keeps playing from its own tab whichever is shown
- Ctrl+click and Shift+click pick several steps; copy, cut, paste and delete them with the buttons or Ctrl+C, Ctrl+X, Ctrl+V and Del, Ctrl+A picks them all
- Copied steps can be pasted into another tab's story

### Arming

![](https://i.ibb.co/SD6KTCD8/openjk-x86-2026-10-14-20-17-31-527.png)

- A story that starts with a "when" can be **armed**: it waits in the background and plays by itself when a player or NPC walks into a map trigger, comes near a spot or an effect, a door moves, or someone says or uses what it waits for
- Have it fire once, every time, or a set number of times
- Several stories can be armed at once; they're disarmed when the map changes
- People too far away to be seen set it off too *(experimental)*: those with this client say when they walk into a watched area, and an admin's client asks RPMod where the others are
- Where the armed stories wait is drawn in the world, on your screen only: a ring round a spot or an effect, a box round a map trigger or a door, with the story's name over it (orange armed, green set off)

### NPC talk and objectives *(experimental)*

- NPC talk: an NPC says a line in chat as a player would, with its own name, name color and an optional emote. During a cinematic with the HUD hidden it shows as a subtitle
- Objectives: a list of mission objectives on the left while the Esc menu is up. Add them, strike them through, take them off or clear them, from a step or with `/objective`

## Cinematic Manager *(playing it for others is experimental)*

Make a whole cut scene on one timeline, the camera's shots and what the players and NPCs in it do, and play it for everyone: the camera taken over, black bars, the HUD hidden.

![](https://i.ibb.co/nMjWj9bW/openjk-x86-2026-10-14-20-17-56-214.png)

- `/cinematicmanager` (or `/cincam edit`) opens it: fly a free camera over the map, the panel on the right, the timeline under the view
- **Camera tab**: the cinematic's shots, one after the other. A shot is a path through spots (smooth or steady, optionally always looking at a point), one spot held still, or an orbit round a point with zoom. Add spots where the camera is with Enter, look through each one, watch the trail between them
- **Entities tab**: every player on the server, you included, and every NPC in the NPC list; pick one there or click them in the view, or type the targetname of an NPC that isn't here yet and press **+ To spawn**; then **+ Add action**:
  - **Move**: aim at the floor and press Enter for each spot they walk or run through; click a spot in the view to pick it, drag it to move it, Del takes it off. A move starts from wherever they are then, a group of NPCs with the same name keeping their shape
  - **Animation**: JKA's own, a range of frames, one held, or played backwards, once or looping, at any speed, picked from the model's own list and shown on it as you make it
  - **Say**: a line, as the Storyteller's Say and NPC talk do: a player's shows in chat as theirs for everyone with this client, yours you say in chat, an NPC's with the name you give it
  - **Emote**: one of RPMod's emotes at that moment, once or looping, for as long as you drag it; an NPC's, or yours, is done for real too when it's played for everyone
  - **Position**: put them anywhere, turned any way, X Y Z, yaw, pitch and roll, in keys along the timeline, smooth or straight from one to the next; pitch and roll tip the whole body, so with an emote you can have someone thrown across the room by an explosion and land on their back. Drag the keys in the view
  - **Teleport**: they're somewhere else at once, a spot and the way they face: where you aim, or dragged in the view. An NPC is teleported for real (`npc tele`) when it's played for everyone; a player is only seen there, on screens with this client, until it ends
  - **NPC command**: freeze or unfreeze, follow you or stop, a dialog or none, a hologram, a projector or none, or kill, at that moment, sent when it's played for everyone
  - **Spawn**: the NPC appears at that moment, of the type and team you pick, where you aim, with the row's targetname, so its moves and lines can follow; sent when it's played for everyone (the NPC manager's spawn)
  - **Remove**: the NPC is taken out of the game at that moment (`npc kill`), and nothing after it moves it
  - **Record**: act it out yourself after a 3-2-1, moving, jumping, swinging your saber, using the force, and press *F10* (or `cl_cinRecordKey`) to stop: they do exactly that from where they are at that moment, up to 30 s. While you record someone else you see through their eyes, where they are and the way they face, wherever you stand and face yourself, moving as you move, and the rest of the cinematic plays along in time, other recordings included, so you can fight or talk with them; you still bump into the walls where you really are, so act it out in the open
- Blades that meet anywhere along them while a cinematic plays clash, sparks and a ring, fast swings too; a blade through someone else cuts them, sparks and a hiss
- A recording keeps what you are heard doing: saber swings, igniting it, jumps, rolls
- **Effects tab**: pick an effect in the effect manager, which opens on the world: where you put it down, turned and sized as you had it, it goes in the cinematic at the moment picked on the timeline instead of on the server. Then drag it over walls and floors in the view, or press J and drag its move arrows; set when it starts, whether it plays once or over and over for as long as you like, its size and which way it faces. Effects have their own row on the timeline, to drag them or their end
- **Sounds tab**: pick a sound in the Sound Manager, which says **Add to cinematic** instead of Send: it goes in the cinematic at the moment picked on the timeline instead of to the server. Set when it starts, whether it plays once or over and over for as long as you like, and whether it's heard **everywhere** alike or **from a spot**, louder nearer it: drag the spot over walls and floors in the view, press J for its move arrows, or put it at the crosshair. **Listen** hears it on your side. Sounds have their own row on the timeline too. A sound playing once still plays to its end if the cinematic is stopped early
- **Timeline** under the view: a row for the camera, two for each one in it, one for the effects and one for the sounds. A shot made longer or shorter takes what's in it along, stretched with it, and what comes after it moves by as much; adding or taking out a shot moves what follows too. Click or drag to any moment and see it then, through the cinematic's camera or your free camera, everyone where they'd be and on their animation's frame. Drag the bars to change when things start, an animation's end to change how long it lasts, a spot or a shot's end to change when the camera gets there
- Preview it on your own screen, from the start or the moment picked
- **Play for everyone** sends it whole (a part a second), then it plays on every screen with this client at the same time; **End for everyone** stops it. NPCs are brought on the server to where their moves end, so players without this client see them there too
- Cinematics are saved by name and played with the Storyteller's Play cinematic step, or `/cinematicmanager send <name>`. Camera shots made before open as a cinematic of one shot
- **Depth of field**: sharp at the distance you set, on whatever the camera looks at, or on a player or NPC as they move, or on a moving spot, blurred in front and behind; each shot keeps its own, and each spot can change it. A moving spot is a circle in the view as wide as what stays sharp: drag it onto what is to be in focus, then at another moment of the timeline drag it again for another key, and the focus goes from key to key; the wheel on it makes it deeper or shallower, and its keys drag along the timeline. *Needs the rd-vanilla_x86.dll from the release, or the Vulkan renderer with `r_fbo 1`.*
- **Filters** for each shot: black and white, noir, sepia, old film, night vision, thermal, security camera, VHS tape, hologram, warm, cold, faded or negative. *Needs the rd-vanilla_x86.dll from the release, or the Vulkan renderer with `r_fbo 1`.*
- **Transitions** in and out of each shot: a fade, or a wipe from the left, the right, the top or the bottom, in black or any color, as long as you like
- Players moved or animated, you too, are only seen that way by those with this client, and go back where they were when it ends
- With the HUD hidden, the chat box is hidden too, so chat shows as subtitles at the bottom of the screen: NPC talk, and the players' chat your `rpc_chatRange_*` settings would show, green like the chat box unless the line has its own colors
- Players can opt out with `rpc_cinematics 0`, and leave one with `/cincam stop`
- The server only sends what's near each player, so something far from them may be missing from a shot of somewhere else

## Map Editor

Edit the map on your own screen: move or hide any part of it, place models, lights and fog, change its lighting, and share it all with the people on the map.

### Editing

![](https://i.ibb.co/SDpWyyRS/openjk-x86-2026-10-14-20-18-21-917.png)

- **Map**: every entity of the map, with all its keys; triggers boxed in orange and brush entities in blue
- **Brushes**: any brush of the map, not only its props, moved with the X, Y and Z arrows, or hidden on your screen
- **Models**: place models, carry them to the crosshair, turn and scale them. The map's own props can be lifted out too, to move or remove them
  - Other maps too, whole: type *map* in the list, pick one and Add puts it at the crosshair, its middle there and its lowest point on what you're looking at; move, turn and scale it like a model to fuse the two maps; it turns and scales around that spot, and its sky and fog are left out. It's drawn only: no one can walk on it, as the server only knows its own map. Others see it shared if they have that map too
- **Lights**: dynamic lights with a color and reach, that can follow a light style
- **Triggers**: boxes that run a console command when you walk in
- **Use triggers**: boxes players use (+use), with a name shown when they look at one; their character reaches out to it, and the Storyteller can wait for it. Shared with the edits
- **Hold use**: a use trigger can be held, in milliseconds as JKA's `usetime`: it's only used once the use key has been held on it that long, a bar showing how far along, while the character works the console (`emconsole`) and stops when it's done or let go
- **Sounds**: speakers that play any music or sound, heard near them or everywhere, looping
- **Styles**: give the map's switchable lights a new pattern (flicker, pulse, strobe...) and color, change its existing light styles, or the lighting of the whole map
- **Fog**: the color and thickness of each of the map's fogs, `No fog` to take one away, or fog volumes of your own. *Needs the release's renderer, OpenGL (rd-vanilla_x86.dll) or Vulkan (rd-vulkan_x86.dll); with Vulkan's own fog (`r_drawfog 2`, its default) up to 16 fogs in all.*
- **Locks**: every door and trigger_multiple in the map with its lock. Lock one to a rank (*-1* for no one) with a name shown to whoever it refuses, change it or unlock it, or remove every lock, through RPMod's `rplock`; locked ones are drawn in red. These are the server's, so everyone is held by them
- A map always starts without edits: they show when you load one of its layouts, or when someone shares theirs, so no one's edits load against anyone else's (`rpc_mapEdits 0` only shows them in the editor)
- They're saved as entities a server could spawn, ready for when RPMod supports it

### Layouts and sharing

![](https://i.ibb.co/ynBqhHTB/openjk-x86-2026-10-14-20-18-38-976.png)

- Keep several sets of edits per map, by name, and load the one you want; Save keeps them in the one loaded. Edits saved before this show as the *old-saved-edits* layout
- `Clear` takes away the edits shown
- **Share** *(experimental)* sends your edits to everyone on the map who uses this client; **Unshare** takes them back
- Shared edits show in place of the receiver's own without touching their files, until they're unshared or the map changes; `rpc_mapShare 0` opts out
- Only the look of the map changes: hidden walls still block and moved props have no collision, since the server knows nothing of it. Triggers that run commands are never shared
- Players who join later need it shared again

## Sound and Music Managers

Find any sound or music track, hear it first, and play it for everyone.

### Sound Manager

![](https://i.ibb.co/ds9K6XN8/openjk-x86-2026-10-14-20-18-53-065.png)

- Lists every sound your game can find, sorted into *Base* and *Custom* > pk3 > folders, with filter buttons and a search box
- Hear it on your side first
- Send it with `rpsound`: once to everyone, to you or to another player, looping everywhere, on a player or at a spot, or local
- The Playing tab lists the looping sounds in the map, to remove one or all of them
- Opened from the Cinematic Manager's Sounds tab, **Add to cinematic** puts the sound in the cinematic instead of sending it

### Music Manager

![](https://i.ibb.co/NndPqCjX/openjk-x86-2026-10-14-20-19-18-215.png)

- Lists every music track the same way
- Preview it on your side; the map's music comes back when you stop
- Play it for everyone with `rpmusic`, or go back to the map's own music, or none

## Languages

Your characters speak the galaxy's languages, and only understand the ones they know.

- Make your characters and give each the languages it speaks, or only understands, in `/languages`
- Anything said as `^3::Huttese:: << text >>` in a language your character doesn't know shows as gibberish, in the chat box, the console and cinematic subtitles; the same words are always the same gibberish, each language its own, and the language's name still shows
- Pick a language to speak and what you say goes out in it on its own, or `/language say <language> <text>` for a single line
- RP emotes stay outside the `<< >>` and are never gibberish: *Hey ^7:: he bows :: ^2All good.* goes out as `::Huttese:: << Hey >> ^7:: he bows :: << All good. >>`
- Team chat, private messages and OOC `(( ))` are never changed
- Nothing changes until you make a character; they're kept in `languages.txt`

## HD Video Cinematics

`/rpcinematic` is no longer limited to RoQ files: it can play real videos, sound included.

- Plays `video/<name>.mp4`, *.webm*, *.mkv* or *.mov* when one exists, and falls back to `<name>.roq` otherwise; the command doesn't change
- Any resolution up to 2048 pixels wide, 1920x1080 included
- H.264, H.265 and VP9 video, with AAC, Opus, Vorbis or MP3 sound kept in sync with the picture
- Loop, hold and stop work just like with RoQ
- Store the video uncompressed in your pk3 ("Store" in 7-Zip) so seeking and looping stay smooth
- Players on a stock client still get the RoQ, so keep a *.roq* with the same name if you want everyone to see something

## Radial Menus

Hold a key and a ring of commands opens around your crosshair: point and let go, or press a number.

![](https://i.ibb.co/R49pyz9m/openjk-x86-2026-10-14-20-19-33-636.png)

- Hold the key, point the mouse at an item and let go to run it, or press its number (1-9, 0)
- Tap the key and it stays open until you click, press a number, Esc or the key again
- You keep moving while it's open: only the mouse, the number keys and the mouse buttons go to it
- Make as many menus as you like in `/radialsettings`: up to ten items each, any console command, with a live preview and a button to bind a key
- An item can open another menu with `radial <menu>`
- Each menu has its own size and look, boxes or round icons, and its text a size and color; each item can have its own color, text size and color, and an image: one of your own from the `radialicons` folder, or any image the game has
- Comes with three: **tools** (every manager), **styles** (the saber styles) and **gestures** (taunt, bow, meditate, flourish, gloat)

## Saber Styles

Choose which styles your style key cycles through, or jump straight to one.

- `/saberstyles medium tavion strong` makes your saberAttackCycle key go through only those, in that order; `/saberstyles all` goes back to normal
- `/saberstyle tavion` switches straight to that style, and the **styles** radial menu does the same
- The server only knows "next style", so the client cycles for you until you're there; the styles in between show for a moment
- Dual sabers and the staff are left as they are

## Animation Manager *(custom emotes are experimental)*

Every animation in the game, emotes made of emotes, and emotes of your own. `/animationmanager` opens it (bind it to a key).

- **Animations**: every animation of every skeleton loaded, the base game's and custom pk3s', with a search; each plays on your own model (other skeletons on theirs), drag it to turn it. The skeleton's button goes to the next one, a right click back one, its arrow lists them all to pick from. Pause it, step it a frame at a time, drag the bar through it, change its speed. **Play on me** plays it on your character, on your screen only, until you move
  - Each says where it comes from: the base game, or the pk3 that **added** it (green) or **changed** it (blue, with the base game's frames and speed beside), found by comparing the animation list in use with the base game's own copy. **All / Base / Custom** shows only those
- **Emote chains**: one command for several emotes: `/emgreet` does `/emsalute`, then `/emyes` once its animation is over. Pick the emotes from the list (or type any command), set a step's seconds or leave it to its animation's length, reorder them. They're the server's own emotes, so everyone sees them. Moving stops the chain
- **Custom emotes** *(experimental)*: an animation, played whole or from one frame to another, at a speed, once or looping, ending with it or held until you move; then bones of it following another animation (the arm waving while the rest sits), turned (pitch, yaw, roll) or moved (X, Y, Z), picked from the skeleton's bones; tags move with their bones. The picked bone has rings and arrows on the model: drag a ring to turn it, an arrow to move it, or type the numbers. **Reset this bone** takes it back. An admin's, with **TOGGLE EXPERIMENTAL** on, show for everyone with this client; anyone else's on their own screen only. **Under it** plays an RPMod emote with it, so players without this client see something too
- Chains and custom emotes are commands of their own, named `em...` like the server's, saved in `emotes/`
- `cl_customEmotes 0` hides the custom emotes admins share

## Presets

Characters saved, and swapped to in one click: for missions, or anyone you play.

- **PRESETS** in the in-game menu (between Profile and Controls), or `/presets`
- **Save who I am now** keeps your name, model and skin, its colours, and your sabers and their colours, with the date
- Each preset is a card: the name in its colours (RPMod's too), the character standing with its hilts in hand, the skin, the sabers and when it was saved
- Click a card to become that character at once; **Update** saves who you are now over it, **Delete** (twice) takes it out
- Kept in `presets.txt`

## Discord Status

Your Discord profile shows what you're playing: "Playing ::JEDI::", the server ("On JEDI Temple"), "Playing as" your character, the players there, the time since you joined, the map's picture (Temple, Grounds, Region and Galaxy's maps; the logo on any other), and a **Join JEDI** button.

- Spoken straight to the Discord app on your computer: nothing to install, and nothing happens when Discord isn't running
- Off until you turn it on: **Setup > Game Options > Discord Status**, or `cl_discord 1`; `cl_discordPrivate 1` leaves out the server and your character's name

## In-game Menu and Camera Settings

- The in-game menu (Esc) has no Vote, Call Vote or Add Bot, and a **PRESETS** button between Profile and Controls
- **FEEDBACK** between Quick Connect and Exit opens [Feedback](#feedback)
- **QUICK CONNECT** after Setup: JEDI Temple, Grounds, Region and Galaxy, each with its address and its map and players as the server says now (asked when it opens, or **Refresh**); click one, or its **Connect**, to go there
- **Admin flags**: when noclip, nodraw, nonsolid or notarget is turned on for you, a small line at the bottom says which are on (green) and off (red), until they're all off. They're read from the commands' replies (noclip from the game too); it hides with the HUD (`cg_draw2D 0`, `cg_drawStatus 0`); `cl_adminFlags 0` hides it, `/adminflags` lists them, `/adminflags reset` takes them all as off
- **Setup > Game Options > Camera Settings** on shows the third person camera's settings beside the in-game menu: third person on or off, distance, angle around you, height, side, tilt, how closely it follows your aim, see-through, field of view and the HUD, and **Defaults**. While it's on, the camera keeps right behind you (`cg_thirdPersonCameraDamp 1`), so it never swings round as you turn. They're the game's own settings (`cg_thirdPerson...`, `cg_fov`)
- **Camera presets**: type a name under the settings and **Save** them as it; **<** and **>** go through the saved ones, each loaded as it comes up; **Delete** takes the one named out. Kept in `camerapresets.txt`, also `/campreset`
- Nothing is shipped for it: the menus, RPMod's or the base game's, are changed as the game reads them

## Emote Preview

See an emote before you do it, right in the Help menu.

![](https://i.ibb.co/S4SWHNVH/openjk-x86-2026-10-14-20-20-00-737.png)

- Click an emote in Help > Emote Commands and your own character plays it next to the list, like the singleplayer datapad
- Every RPMod emote is built in; one added later is learned the first time you do it
- `/emotepreview` lists them

## Widescreen Text

An option to stop text looking stretched on widescreen monitors.

- HUD, scoreboard, chat, menus and the chat input line keep their proper shape, like TaystJK's `cl_ratioFix`
- Off by default; `/cl_ratioFix 1` turns it on

## Console Selection and Find

Select, copy and search the console with the mouse.

- Drag the mouse over the console to select its text; dragging past the top or bottom scrolls it. Double-click a word, Shift+click to stretch the selection
- Ctrl+C or right click copies it, without the color codes; right click with nothing selected pastes into the input line
- Click the input line to put the cursor there; with `con_modernInput 1` drag over it to select, double-click a word
- Ctrl+F with the mouse over the console opens a find box at the bottom right: everything matching is lit up and it jumps to the newest; Enter, Up and Down (or its arrows) go from one to the next, Esc closes it
- Full screen the console draws its own cursor; in a window your own is used

## Lists and Text Boxes

The same lists and text boxes in every window.

- Every list has a bar on its right side when it's longer than its box: drag it, or click beside it to jump there
- In a text box, click to put the cursor anywhere, Home/End and the arrows move it, Ctrl+arrows jump a word, Ctrl+Backspace deletes one
- Select with Shift or by dragging the mouse, double-click a word; Ctrl+A selects all, Ctrl+C, Ctrl+X and Ctrl+V copy, cut and paste
- A text too long for its box goes on to the lines below while you type it, so long lines (Say, NPC talk, center messages...) can be read and fixed whole

## Long Chat in Parts

Say more than the server keeps in one message.

- `cl_chatSplit 1` (off at first, or Long chat in `/consolesettings`): a chat too long is sent in parts, "--" at the end of one and the start of the next, its colour carried on
- The parts go a second apart, as the server ignores commands closer together (`cl_chatSplitDelay`, 1100 ms)
- `cl_chatSplitLength` is how much the server keeps of one message (149 in the base game); a spoken language's tag is counted in
- `cl_chatSplitCommands` lists the chat commands split (`say say_team tell:1 me`); *:1* keeps one word, as tell's player, in front of each part. `cl_chatSplitMark` is the mark (*--*)
- A long `/me` is split too, each part its own emote line, and so is what you type in the chat box

## Chat Capitals

Your chat starts its sentences with a capital, even when you don't.

- `cl_chatCapital 1` (off at first, or Capitals in `/consolesettings`): a message, and each sentence in it, starts with a capital: *hi. how are you? fine* goes out as *Hi. How are you? Fine*
- A sentence ends at a . ! or ? and a space; *...* is a pause and leaves the next word as it is, and *3.5* or a web address are left alone
- `cl_chatCapitalEmotes 1`: each emote's first letter too, `::waves::` as `::Waves::`; a message starting with an emote gets its capital after it
- `cl_chatCapitalCommands` lists the commands it works in (`say say_team me`); private messages are left as typed

## Console Key

Open the console with a key of your own, and free the key under Esc.

- `/con_keybind <key>` makes that key open and close the console, `/con_keybind none` takes it away; also **Its key** in `/consolesettings`, Typing and keys
- It works over menus and the managers too, unless it's a key that types something
- `cl_consoleTildeKey 0` (**Key under Esc**) makes the key under Esc an ordinary key, to type \` and ~ or bind it to something else; the settings window only turns it off once the console has a key of its own
- Shift+Esc always opens the console

## Saber Light

Lightsabers light up what's around them in their own color, pulsing as they hum.

- `cg_saberlight 1` every blade lights the walls, the floor and the people near it; *2* only your own; *0* off (the default)
- In the blade's own color, RGB blades too, pulsing gently and following the blade along its length, so a long blade lights along all of it
- It takes the place of the game's own steady saber light
- On your screen only, nothing is sent: you see everyone's blades light up, whatever client they use
- Needs dynamic lights on (`r_dynamiclight 1`)

## NPC Names

Put the crosshair on an NPC and its name shows, as a player's does, in the same place and lettering.

- `cl_npcNames 1` shows the targetname of the NPC under the crosshair (the default); NPCs without one show nothing; *0* off
- Only RPMod's `npc list` says who's who, and it's for admins, so nothing shows until an admin with this client and **TOGGLE EXPERIMENTAL** on runs `/npcnames`: it reads the list quietly and sends the names as they are then to everyone with this client, the admin included. Names not made for roleplay stay out of the way until an admin wants them
- NPCs spawned and players who joined after that need another `/npcnames`. `/npcnames clear` takes them back from everyone

## RPMod Colors in the Console

All of RPMod's extra colors now show properly in the console, instead of being stripped out.

![](https://i.ibb.co/MHmNM5n/Screenshot-2026-10-09-135207.png)

- Draws all 62 RPMod color codes (^0-^9, ^a-^z, ^A-^Z) in the console and the chat lines at the top of the screen, with RPMod's exact colors
- Only on RPMod: other servers look the same as before
- Set `rpc_removeExtraColorsInConsole 0` to use it
- `/colorcodes` prints your name with every code visible and drawn in its own color, handy while editing your name
- `/colorcodes <text>` does the same for any text
- What you type in the console and the chat box shows in its colors as you type it, RPMod's too, the codes still showing

## Chat Range in the Console

RPMod's chat ranges, now for the console too: chat from far away can fade out or stay out of your console.

- Works like RPMod's `rpc_chatRange_*` settings for the chat box, with its own settings for the console, so you can set them differently
- `rpc_chatRangeConsole_mode`: 0 shows all chat as normal (default), 1 fades chat by distance, 2 also leaves out chat from beyond the max range, 3 only shows chat from closer than the min range
- `rpc_chatRangeConsole_min` (128): chat from closer than this is fully visible
- `rpc_chatRangeConsole_max` (1024): chat from this far or farther is at the minimum alpha, or left out in mode 2
- `rpc_chatRangeConsole_alpha` (25): how visible the farthest chat stays, in percent
- /me lines too, by how far the player is from you; someone out of sight counts as beyond the max range
- Your chat box and chat sounds don't change

## Whispers

Characters whisper in ^k. With `/cl_whisper 1`, only those close by read it.

- Up to `cl_whisperHear` (128) units away, the whisper shows as it is
- Up to `cl_whisperNotice` (384), it's left out of the chat and RPMod's notification says *:: You hear <name> whispering... ::*, the name in its colors
- Farther, it's only left out
- Only the ^k parts go: the rest of the line, in other colors, shows as ever, and a line that was all whisper doesn't show at all
- Works for say and /me, in the chat box, the console and cinematic subtitles; your own whispers always show

## Background Exec

Run long *.cfg* scripts without being stuck until they finish.

- `/exec` keeps running a script's *wait*s in the background, so you can move, type and run other commands while it plays out
- Run several scripts at the same time
- `/execlist` lists the scripts still running
- `/execstop` stops all of them, `/execstop <file>` only that one

## Load a pk3 while playing

Put a new pk3 in the game's folder and use it at once, without a vid_restart.

- `/loadpk3` loads every pk3 added to the game's folders since the game started; `/loadpk3 <name>` only that one
- Models, skins, shaders and sounds the game couldn't find before are looked for again, and the new pk3's .shader files are read
- Players using a model from the new pk3, who showed as Kyle, change to it straight away; the map's models and sounds in it load again too
- A pk3 that changes files the game already loaded, or a map, still needs a vid_restart or the next map
- Not on a pure server: it only lets you use the pk3s it lists
## Fighter Takeoff Guard

Hold jump to take off in a fighter without being launched into a boost the moment you leave the ground.

Your game now tracks when the takeoff is about to end and stops sending jump until you let go of the key, so the ship lifts off and stays put

### Separate boost key

- `+vehboost` fires a fighter's boost on its own key, e.g. `/bind shift +vehboost`

### Commands

- `/cl_fighterTakeoffGuard 0` turns the guard off and brings back the old behaviour (*1* by default)

## Toggle Crouch

Stay crouched without holding the key.

- `/cl_crouch 1` keeps you crouched while on foot, e.g. `/bind c "toggle cl_crouch"`
- Jumping still works, and it has no effect in vehicles, in water, while flying or spectating

## Wall Stick

Stay on a wall you grabbed while you chat.

- `/cl_wallstick` turns it on or off; grab a wall holding jump as usual and you stay on it
- Chatting, typing in the console or opening a menu no longer drops you; press jump again to push off

## Console Settings

Make the console look the way you like.

- `/consolesettings` opens a window over the console, which stays down so every change shows as you make it
- How see-through it is, how big its letters are and how far down the screen it comes
- The picture behind it: the standard one, an image of yours (put .jpg, .png or .tga files in a `consolebg` folder in your RPMod folder), or any image or shader typed in
- Easier to read: a font of your choice (the console's own, Arial, Ergo, OCR or A New Hope), a shadow or an outline behind the letters, more room between the lines, margins at either side, the picture behind it darker
- Colours: plain text in white, soft white, light grey, light yellow, amber, light green or light blue; dark colours lit up, or every line in the one colour; red-green or blue-yellow colour-blind colours; nothing faded, far chat neither
- Timestamps shown or hidden
- The lines over the game get their own letter size, and stay as long as you like; long ones go on to the next row instead of off the screen
- **Typing** *(both off at first)*: **Console line** (`con_modernInput`) lets a long line in the console go on to more rows, with Ctrl+arrows to jump a word, Shift or the mouse to select (double-click a word, right-click copies) and Ctrl+A, C, X and V, in the chat box too. **Long chat** (`cl_chatSplit`) sends a message too long for the server in parts a second apart, "--" between them
- **Typing and keys** tab: the console line and long chat, **Capitals** (disabled, each sentence, each emote's, or sentences and emotes), and **Opening the console**: its own key (click, then press the key), **None**, and whether the key under Esc opens it
- "Everything as it was" puts it all back. The settings are `con_opacity`, `con_scale`, `con_height`, `con_background`, `con_font`, `con_textShadow`, `con_lineSpacing`, `con_margin`, `con_bgDarken`, `con_textColor`, `con_contrast`, `con_colorblind`, `con_noFade`, `con_timestamps`, `con_notifyScale`, `con_notifytime`, `con_modernInput`, `cl_chatSplit`, `cl_chatCapital`, `cl_chatCapitalEmotes` and `cl_consoleTildeKey`

## Model Safeguards

Don't get disconnected by a model the game can't draw.

- A model with more than 999 verts on a surface disconnects you as it loads
- `/cl_safeModels 1` shows such a map object as a missing model instead, with a warning in the console
- `/cl_safePlayerModels 1` shows such a player or NPC model as Kyle instead, with a warning in the console
- They protect the models loaded after you turn them on

## Update Check

Know when there's a newer OpenJK-rpmod.

- Each time the game starts, it asks GitHub for the newest release, without holding the game up
- When yours is older, the console says so and the main menu shows it at the bottom
- `/version` shows your version and the newest, with the link to download a newer one; `/version check` asks again now
- `/cl_updateCheck 0` never asks
- A build made between releases is named after the next one (`v0.7.4-dev-g5cc1ce8`), and is out of date once that one is out
- On Linux and macOS it needs libcurl, which macOS always has and most Linux systems do; without it the game runs as ever, only without the check

## Feedback

Send a bug or an idea for the OpenJK-RPMod project, and read the answers, without leaving the game. Not every idea will be in RPMod itself.

- **FEEDBACK** in the in-game menu (Esc), between Quick Connect and Exit, or `/feedback`, opens the window; `/feedback` works from the main menu too
- **New report**: a bug or an idea, a title, and what happened (what you did, what happened and what you expected) or what the idea is. The game's version, renderer, map and server go with it, shown under the text before you send
- A report goes with your RPMod account, read from `rpinfo` without showing it in the console: you send it from an RPMod server, logged in. Logged out, or from the main menu, the window says so and nothing is sent
- It's answered on the project's Discord, and marked planned or fixed, or closed
- **My reports** lists what you've sent, with its status: open, planned, fixed or closed. Pick one to read what you wrote and its answers, and answer them; a closed one can't be answered any more. My reports works from the main menu too
- The game asks for news when it starts and every few minutes after (every 30 seconds while My reports is open). A report with news has a yellow mark, the tab says how many, and the console says what's new: an answer, a new status, or both. `/feedback` then opens on it
- Reading the answers needs no login: your game makes a private key the first time you send a report, kept in `feedback.key` in your game's home folder, and only that key sees your reports. Another computer, or the file deleted, doesn't see them
- One bug or idea a report, please: each is answered on its own. A few reports an hour at most
- On Linux and macOS it needs libcurl, as the update check does

## Vulkan Renderer *(experimental)*

[TaystJK](https://github.com/taysta/TaystJK)'s Vulkan renderer comes with the game, next to the usual OpenGL one, which stays the default.

- Setup's RENDERER page (under DEFAULTS, in the main menu and in game) picks it and sets what's below; Apply restarts the video with them. The rows for Vulkan's own settings show once Vulkan is picked, and *More Settings >* at the bottom goes to a second page of them
- Or by hand: `/cl_renderer rd-vulkan` then `/vid_restart` switches to it, without leaving the server; `/cl_renderer rd-vanilla` and `/vid_restart` switch back
- Anti-aliasing (`r_ext_multisample 2`, 4 or 8), bloom (`r_bloom 1`), HDR (`r_hdr`), dithering (`r_dither 1`) and a render resolution of its own (`r_renderScale`, `r_renderWidth`, `r_renderHeight`); most of them need `r_fbo 1`
- `r_vbo 1` and `r_vbo_models 1` keep the map's and the models' surfaces on the graphics card, for more frames per second
- `r_device` picks the graphics card when there's more than one
- **Supersampling** (`r_ext_supersample 1`, needs `r_fbo 1`): the picture drawn at twice the width and height and scaled down to the window, smoothing every edge, for about four times the drawing. Screenshots come out at the larger size
- Effects of its own, each off until turned on, all needing `r_fbo 1`; all but colour grading are on the RENDERER page too:
  - **Ambient occlusion** (`r_ssao 1`): corners, creases and the ground under things darkened. `r_ssaoRadius` is how far it reaches (48 units), `r_ssaoIntensity` how dark (1)
  - **Soft particles** (`r_softParticles 1`): smoke, fire, sprites and other effects fade where they meet walls and floors instead of cutting into them with a hard line; `r_softParticlesDepth` is how far in front of a wall they start to fade (24). In fog too; not on the map's own surfaces
  - **FXAA** (`r_fxaa 1`): smooths edges over the finished picture, the edges of grass and fences too, which anti-aliasing misses
  - **Sharpening** (`r_sharpen`, 0 to 1): crisper detail, against the softness of FXAA or a lower `r_renderScale`
  - **Sun shafts** (`r_sunShafts 1`): beams of light from the sun through gaps, on maps whose sky has a sun; `r_sunShaftsIntensity` how bright (1)
  - **Tone mapping** (`r_tonemap 1`): bright light rolls off along a film-like curve instead of clipping to white
  - **Colour grading**: `r_exposure` (1), `r_saturation` (1), `r_contrast` (1), `r_warmth` (-1 cold to 1 warm). A map can bring its own look in `maps/<map>.grade`, lines like `saturation 0.9` or `warmth 0.3` (and `exposure`, `contrast`, `tonemap 1`), used in place of yours while it's loaded unless `r_mapGrades 0`
  - **Motion blur** (`r_motionBlur`, 0 to 1): the picture smears when the camera moves or turns, as long a smear whatever the frame rate; cuts and teleports don't blur
- Every tool works with it; the cinematic camera's depth of field and filters need `r_fbo 1`. Its depth of field blurs bright points into round discs (bokeh, `r_dofBokeh`, 0 to 1; 0 is OpenGL's look)
- On macOS it needs MoltenVK, which macOS doesn't come with

## Experimental features

Some features reach other players through workarounds that are only available to those that use this client. These 'workarounds' will eventually be removed in favor of server support.

- Client required (Experimental): reaches other players hidden in an `rpnotify`, so only those with this client see it, and it needs **TOGGLE EXPERIMENTAL** on
- Not required (everyone): works with the toggle off

Features not listed here are not experimental.

- **Cinematic Manager**
  - Client required (Experimental): **Play for everyone**, **End for everyone**, `/cinematicmanager send`, `/cinematicmanager stop all`, the Storyteller's Play cinematic step
  - Not required (everyone): making, saving and opening cinematics, recording, the preview, `/cinematicmanager play` on your own screen
- **Cinematic camera (`/cincam`)**
  - Client required (Experimental): sending a shot, the bars or the HUD to others
  - Not required (everyone): playing it `local`, on your own screen
- **NPC Manager**
  - Client required (Experimental): NPC talk (**Talk**, `/npcmanager talk`), talking through an NPC (`/npcmanager select`), routes walked for everyone (the Routes tab, `/npcmanager walk`)
  - Not required (everyone): spawning, managing and the other NPC commands, a route walked with `local` on your own screen
- **Storyteller**
  - Client required (Experimental): the steps Say as a player, NPC talk, Walk a route, Objective, Sky look and Play cinematic; seeing players out of sight walk into a watched area
  - Not required (everyone): every other step, condition and timer
- **NPC Names**
  - Client required (Experimental): an admin sharing the names; seeing them needs this client, not the toggle
- **Animation Manager**
  - Client required (Experimental): a custom emote shared by an admin; seeing it needs this client, not the toggle. The Custom emotes tab itself is experimental
  - Not required (everyone): previewing animations, emote chains, a custom emote on your own screen
- **Objectives**
  - Client required (Experimental): changing them for everyone with `/objective`
  - Not required (everyone): `/objective list`
- **Shader Manager**
  - Client required (Experimental): **Share with all** and **Reset for all** in the Sky tab, **Shared** in the Mine tab
  - Not required (everyone): finding and remapping shaders, overlays, the sky and your player shaders on your own screen
- **Effect Manager**
  - Client required (Experimental): sharing an effect's **Size** (with the toggle off it's drawn that size on your screen only)
  - Not required (everyone): placing, bolting, moving and removing effects, sent with RPMod's own `rpeffect`
- **Map Editor**
  - Client required (Experimental): **Share** and **Unshare** (`/mapedit share`, `/mapedit unshare`)
  - Not required (everyone): editing and layouts on your own screen
