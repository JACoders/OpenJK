This file will generally summarise the Github repo commit history
Key: [-] removed, [+] added, [\*] modified

# Features

## Single- and Multiplayer

* [\*] OpenJK now writes configs/screenshots/etc to `fs_homepath` directory. No longer have to run as administrator on Windows. (Multi-User support)
* [+] Added mouse-wheel to console
* [+] Add `r_noborder` option (windowed mode feature).
* [+] Add `r_centerWindow` option (windowed mode feature).
* [+] Add `r_mode -2` option to use desktop resolution.
* [\*] Removed cheat protection from `r_we` cmd, `r_dynamicGlow*` cvars
* [-] Removed CD Check Code
* [-] Removed Anti-Tamper Code. (It serves no purpose with source code available.)
* [\*] Shift-Escape will now also open the console as an alternate (e.g.: keyboard doesn't support the normal console key)
* [\*] Escape key will now close the console before anything else if the console is open
* [\*] Removed shift key requirement to open console
* [+] Raw Mouse Input is now available on all platforms with the in_mouse cvar at the current default value. (SDL2 uses raw mouse input by default)
* [\*] Drastically improved command/cvar tab-completion including auto-completion of arguments (e.g. `map mp/ffa1`)
* [+] Clipboard paste support added to all supported platforms now with SDL2.
* [+] Add `stopmusic` command
* [\*] Fixed segfault crashing in ragdoll code.
* [\*] Fixed `r_overbrightbits 1` resulting in blackness.
* [\*] Made the md4 (filesys) checksum code compatible with 64-bit operating systems.
* [\*] Fixed radar and rocket locking drawing not rendering properly.  Can now support all types of shaders with multiple images if wanted too.
* [\*] MAX_PATCH_PLANES does not occur on some maps anymore with OpenJK (this isn't related because OpenJK specifically changed something to cause the error, but because of newer compilers affecting optimizations on decimal numbers)
* [\*] `OpenJK` folder is now searched as a fallback location if `base` and `fs_game` folders do not contain the appropriate gamecode to load.
* [\*] Error log on crashing exits are now reported to a crashlog text file in your OpenJK homepath.
* [-] Removed `r_allowSoftwareGL` cvar as it is no longer useful or needed.
* [+] Added filename completion to `cinematic` command.
* [\*] No more viewlog console for client or server.
* [+] Added improved dedicated server console with color (0-7) support and arrow key support and tab completion that functions the same as in-game console.
* [-] Removed blackbars around screen when rendering in widescreen.
* [\*] Fixed white screen during load with aformentioned black bar removal.

## Singleplayer only

* [+] Now searches for jagamex86.dll (or equivalent) in the mod's folder, too, meaning SP Code mods are possible.
* [+] NPCs support alternate saber colours
* [\*] Fallback location for jagame mod bin is now `OpenJK` instead of `base`
* [+] Add `modelscale` and `modelscale_vec` support to `misc_model_ghoul`
* [\*] Fix external lightmap support
* [\*] `r_flares` defaults to 1 now like Multiplayer.

### Gamecode (only available in `fs_game openjk` and derived mods)

* [+] Added `cg_smoothCamera` (default 1)
* [+] Added `cg_dynamicCrosshair` (default 1)
* [+] Added simple hud from multiplayer with tweaks to match real HUD options. `cg_hudFiles 1`

## Multiplayer only

* [\*] Gamecode DLL files on Windows are no longer extracted to homepath or basepath, but a temporary file path.
* [\*] Drastically improved status (server) command
* [\*] Tweaked `forcetoggle` rcon command.
* [+] Added `weapontoggle` rcon server command similar to `forcetoggle` command.
* [\*] Fixed memory leak related to NPC navigation on map changes.
* [+] Cheats are now defaulted to 1 in menu. Do not be alarmed, starting normally will disable them or connecting to a non-cheat server. This allows cheats to work properly while playing back demos.
* [+] Add `sv_lanForceRate` (Defaults to 1) Feature was already enabled, but not toggleable.
* [\*] `globalservers` master server command now supports multiple master servers with the `sv_master1..5` cvars
* [+] Added `fontlist` command.  Useful for when making mods with custom fonts.
* [+] New serverside kick commands `kickall`, `kickbots` and `kicknum` (alias to `clientkick`).
* [-] Removed demo restriction code.
* [\*] `svsay` command prints to dedicated console
* [\*] Cvars will be sorted alphabetically when saved to disk
* [+] Added `ja_guid` userinfo field to uniquely track players for statistics
* [+] Added `cvar_modified` to show which cvars have been changed from default values
* [+] Added `s_doppler` sound effect for moving sound sources (rockets)
* [+] Added support for `surfaceSprites flattened` in MP.  (Fixes surface sprites on t2_trip)
* [+] Added in-engine ban code from ioquake3. Cmds: `sv_rehashbans`, `sv_listbans`, `sv_banaddr`, `sv_exceptaddr`, `sv_bandel`, `sv_exceptdel`, `sv_flushbans`. CVar: `sv_banFile`
* [+] `addFavorite` command added to add current or specified server to favorites list.
* [+] Add ability to paste in text files in the UI
* [+] Add `cl_motdServer1..5` cvars. `cl_motd` points to which one is used or 0 to turn off
* [+] Add QuakeLive style mouse accel option (`cl_mouseAccelStyle`, `cl_mouseAccelOffset`)
* [+] Server side demo recording per client (from their pov) support
* [+] Add `sv_blockJumpSelect` cvar to help prevent use of modded clients using an exploit with `FP_LEVITATION` with old mods
* [+] Add external lightmap support from SP
* [+] Added ability to substitute BSP entities with ones from an external .ent file when loading a map (for easier entity modding)

### Gamecode (only available in `fs_game openjk` and derived mods)

* [\*] Tweaked simple hud with adjustments to match real HUD options. `cg_hudFiles 1`
* [+] Added `pmove_float` cvar (default off) for no velocity snapping resulting in framerate-dependent jump heights.
* [+] `clientlist` displays clients by id/name and if they are a bot. (Shows real client id unlike `serverstatus`)
* [+] Added `cg_fovAspectAdjust` to correct field of view on non-4:3 aspect ratios
* [+] Added `cg_fovViewmodel` to adjust the field of view for first-person weapons
* [+] Added `cg_chatBeep` and `cg_teamChatBeep` to toggle the chat message sound
* [+] Added JK2 gametypes
* [\*] Rewrote `callvote` code to allow disabling specific votes, added more options (e.g. display map list)
* [+] Add server command `toggleallowvote` to easily adjust the bit values of `g_allowVote`
* [+] Add userinfo validation options (`g_userinfoValidate`)
* [+] Add server command `toggleuserinfovalidation` to easily adjust the bit values of `g_userinfoValidate`
* [+] `fx_wind` entity
* [\*] `fx_rain` entity supports most options from SP now. LIGHTNING/shaking not supported, acidrain doesn't actually hurt
* [\*] more `customRGBA` options from SP on NPCs
* [+] Force Sight surfaces (cgame modification required)
* [+] Human Merc NPC spawner
* [\*] Make `target_location` entities logical (no gentity space used in most cases)

# Increased/lifted limits

## Single- and Multiplayer

* [\*] Increased Command Buffer from 16384 to 128*1024
* [\*] Increased max cvars from 1224 to 8192

## Singleplayer only

* [\*] `MAX_SHADER_FILES` bumped to 4096 from 1024 to match MP


# Bugfixes

## Single- and Multiplayer

* [\*] Fixed incorrect `alphagen` enum usage.
* [\*] Fixed widescreen resolution changes causing black screen when UI restarted.
* [\*] Fixed crash when trying to run custom resolutions with a listen server.
* [\*] Fixed Gamma Clamp on WinXP+
* [\*] Fixed weather system incorrectly throwing up a warning with shader data if weather system was unable to parse a vector correctly.
* [\*] Windows now uses correct memory status code for > 2gb when checking if low on physical memory.
* [\*] Tweaks to the cvar code to make it more strict in terms of read only/cheats/init. Fixes a lot of broken rules with cvars.
* [\*] Fixed cvar commands that allow you to `cg_thirdPerson !` (i.e. use a value of `!`) prevent you from typing out longer strings starting with a `!` as the value.
* [\*] FX Flashes now properly scale to fov and window aspect.
* [\*] Fixed a lot of formatting security holes.
* [\*] Several Out-of-bounds memory access and memory leaks fixed.
* [\*] Improved command line parsing based off of ioquake3 patches.
* [\*] Improved `echo` command by preserving colors based off of ioquake3 patches.
* [\*] Improved `GL_Extensions` printing using ioquake3 fix to prevent crashes on newer cards.
* [\*] Alt-tab works properly
* [\*] ~~Fix Windows issue where pressing alt key in windowed mode caused a temporary freeze~~
* [\*] Fixed never fading shadow in text
* [\*] Fix up font renderer glyph positioning
* [\*] Fixed a hang with some weird music sample rates
* [\*] Fixed a crash at startup when `r_dynamicGlow` was set to 2
* [\*] Fix Out-Of-Bounds access in `CM_EdgePlaneNum`
* [\*] Clamp `scr_conspeed` to be in the range 1-100
* [\*] Fix stencil shadows not working if a model has more than 500 vertexes.

### Gamecode (only available in `fs_game openjk` and derived mods)

* [\*] Fixed overstrike mode when using any UI edit box
* [\*] Fix crash when standing on an NPC who has been knocked down

## Singleplayer only

* [\*] Fixed a nasty memory issue with clipboard pasting
* [\*] Fixed `MiniHeapSize` issue
* [\*] Fixed intro cinematic only displaying as white screen when on non 4:3 aspect ratio.
* [\*] Fixed potential out of bounds in sound code.
* [\*] Fixed buffer overflow in filesystem code which prevented use of some maps (atlantica). Raven made a `Com_Error` to prevent this from happening, we removed that as well.
* [\*] Rosh no longer randomly dies due to falling damage on the first level.
* [\*] Fix force absorb capping to 100 instead of maximum force points
* [\*] Fix some invalid chunk length errors on x86_64 platforms

## Multiplayer only

* [\*] Fixed several parsers that broke with characters which wrapped to being negative.
* [+] Added misc security fixes to allow servers to deal with connectionless packets.
* [\*] Added security fixes to prevent servers from spoofing clients with connect packets.
* [\*] Added security fixes to prevent anyone from spoofing clients with print packets.
* [\*] Fixed clients being able to set IP via cvar.
* [\*] Dedicated server binaries now correctly print sectioned prints without adding newlines erroneously.
* [\*] Fixed client crash related to ragnos NPC
* [\*] Fixed some item prediction errors in team games. (Item prediction still sucks in general though)
* [\*] Fixed buffer overflow in client side rcon command.
* [\*] Fixes names that contain `*` or `**` at start incorrectly showing up in notify top box and chat box when sending messages.
* [\*] Server side / `viewlog` console now strips the `[skipnotify]` and `*` properly too.
* [+] (Un)pausing the game in solo play now results in smoother transition.
* [\*] `timescale` frametime fixes. Frametime < 1 is no longer possible (bad things happen)
* [\*] Fixed invalid `r_textureBitsLM` resulting in fullbright mode without cheats
* [\*] CVar code in engine is more robust.  Protects against clients setting systeminfo cvars they should not etc.  More accurate warning messages when servers/vms/clients set cvars they should not.
* [\*] Fix exploit with `usercmd` bytes set to -128

### Gamecode (only available in fs_game openjk and derived mods)

* [\*] Fixed very bad out of bounds access in `G_LogWeaponFire` with NPCs.
* [\*] Fixed some voting issues relating to clients disconnecting and/or switching teams.
* [\*] Fixed some voting issues relating to bad percentage calculation for passing/failing.
* [\*] Fixed color code stacking in names where colors were supposed to be stripped. (This relates to engine and gamecode for full effect)
* [\*] Fixed saber lock crash
* [\*] Force updates of clients at a fixed interval to prevent certain exploits.
* [\*] Prevent fast teamswitch exploit
* [\*] Disable "scoreboard" team
* [\*] Fix some looping sound issues
* [\*] Fix spectators being stuck when they stop following dead/disintegrated players
* [\*] Precache weapons on `map_restart` if `g_weaponDisable` changes
* [\*] Properly detect server settings (weapon/force disable)
* [\*] Filter multiple "@" characters in a name resulting in a localised string lookup (translation)
* [\*] Default/missing model+saber is now "kyle"
* [\*] Fixed tournament queueing (for duel/power duel)
* [\*] Fixed issues where spectators had trouble flying through doors close together
* [\*] Fix `cg_smoothClients` affecting local player negatively
* [\*] Fix 2d screen tints with some effects being too dark (protect/abosrb/rage/ysalamiri/inlava/inwater/inslime)
* [+] Filter out servers with invalid chars in their information from the browser (`ui_browserFilterInvalidInfo`)
* [+] UI now supports multiple master servers
* [\*] Fix teamoverlay not making you look dead in siege when in limbo
* [\*] Fix glowing lights bugs on players and dead spectators in siege (in client mod)
* [\*] Properly load siege sounds for all models that have them, fallback to old broken location if necessary.


# Cleanup

## Single- and Multiplayer

* [-] MPlayer code removed.
* [-] Removed force feedback code (requires unavailable commercial library)
* [\*] Updated the JPG library
* [\*] Updated the PNG library
* [\*] Updated the zLib and minizip libraries
* [\*] Refactor binds code in the UI
* [-] Removed (unused) RMG code
* [\*] General buffer safety cleanup

## Multiplayer only

* [\*] Tweaked serverbrowser client engine code.
* [\*] Pure server code cleanups in client and server.

### Gamecode (only available in fs_game openjk and derived mods)

* [\*] Optimised .sab parsing and fixed some potential memory corruption
* [\*] Optimised .veh and .vwp parsing
