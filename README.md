# SORRY FOR THE HISTORY CHANGES! #

For legal reasons we had to make changes to the history. This likely broke every forker's repo. See [here](http://git-scm.com/docs/git-rebase.html#_recovering_from_upstream_rebase) for how to fix if you've changed anything, or just delete your github fork and local folder and start over if you haven't.

# JACoders "OpenJK" project #

**Caution! Do not create JK2 SP mods, they likely won't be supported. Wait for the official JK2 SP mod for Jedi Academy, then base your work off that.** (Or help create it, first.)

## Description ##

The purpose of this project is to maintain and improve the Jedi Academy and Jedi Outcast games, developed by Raven Software.

This project will not attempt to rebalance or otherwise modify core gameplay aspects.

Major contributors, in alphabetical order:
* eezstreet
* Ensiform
* mrwonko
* Raz0r
* redsaurus

## Dependencies ##
* OpenGL
* OpenAL (included on Windows)
* ALUT (included on Windows)
* libpng (optional)
* libjpeg (optional)
* zlib (included on Windows)

## Changelog ##

### Major Changes ###
* The Jedi Academy singleplayer now searches for jagamex86.dll in the mod's folder, too, meaning SP Code mods are possible.
* Increased Command Buffer from 16384 to 128*1024
* Increased max cvars from 1224 to 2048
* Removed unnecessary xbox code.

### Minor Changes ###
* Added mouse-wheel to console
* Added misc security fixes to allow servers to deal with connectionless packets.
* Added security fixes to prevent servers from spoofing clients with connect packets.
* Added security fixes to prevent anyone from spoofing clients with print packets.
* Compiles/runs with VS2012.
* Fixed clients being able to set IP via cvar.
* Fixed MiniHeapSize issue in SP
* Fixed widescreen resolution changes causing black screen when UI restarted.
* Fixed crash when trying to run custom resolutions with a local server.
* Fixed crash related to ragnos NPC in MP
* Fixed Gamma Clamp on WinXP+
* Fixed buffer overflow in client side rcon command.
* (Un)Pausing the MP game in solo play now results in smoother transition.
* Tweaks to the cvar code to make it more strict in terms of read only/cheats/init.  Fixes a lot of broken rules with cvars.
* Cheats are now defaulted to 1 in menu.  Do not be alarmed, starting normally will disable them or connecting to a non-cheat server. This allows cheats to work properly while playing back demos.
* Fixed cvar commands that allow you to "cg_thirdPerson !" prevent you from typing out longer strings starting with a ! as the value.
* Optimized MP shader loader.  This also fixes some cases where duplicate shaders that are wrong were being used.
* Add fx_flashRadius (Defaults to 11) To set the size of FX flash radius. ("Rockets")  Set to 0 to disable completely.
* Add sv_lanForceRate (Defaults to 1) Feature was already enabled, but not toggleable.
* Timescale frametime fixes.  Frametime < 1 is no longer possible (bad things happen)
* Tweaked globalservers master server command a bit for better verbosity.
* Fixed a lot of formatting security holes.
* Several Out-of-bounds memory access and memory leaks fixed.
* Added fontlist command.  Useful for when making mods with custom fonts.
* New serverside kick commands kickall kickallbots and kicknum (alias).
* Improved command line parsing based off of ioquake3 patches.
* Improved echo command by preserving colors based off of ioquake3 patches.
* Improved GL_Extensions using ioquake3 to prevent crashes on newer cards.
* Removed CD Check Code
* Removed CD Key Code
* Removed demo restriction code.
* Removed Anti-Tamper Code.
* Shift-Escape will now also open the console as an alternate (ie: keyboard doesn't support the normal console key)
* Removed shift key requirement to open console


## Known Bugs ##

* Missing Known Bugs list

## Mod Developer Notes ##
* If your mod intends to use engine "hax" to figure out the nedaddr types (NA_IP, etc) NA_BAD is now 0 and NA_BOT is now 1 instead of vice versa.
