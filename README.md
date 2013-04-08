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
* Compiles/runs with VS2012.
* Fixed clients being able to set IP via cvar.
* Fixed MiniHeapSize issue in SP
* Fixed widescreen resolution changes causing black screen when UI restarted.
* Improved command line parsing based off of ioquake3 patches.
* Improved echo command by preserving colors based off of ioquake3 patches.
* Improved GL_Extensions using ioquake3 to prevent crashes on newer cards.
* Removed CD Check Code
* Removed demo restriction code.
* Removed Anti-Tamper Code.
* Removed shift key requirement to open console


## Known Bugs ##

* Missing Known Bugs list

## Mod Developer Notes ##
* If your mod intends to use engine "hax" to figure out the nedaddr types (NA_IP, etc) NA_BAD is now 0 and NA_BOT is now 1 instead of vice versa.
