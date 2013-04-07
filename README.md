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

## Changes ##

* The Jedi Academy singleplayer now searches for jagamex86.dll in the mod's folder, too, meaning SP Code mods are possible.


## Known Bugs ##

* There's some crash bug (probably the OpenGL extension string) that only happens when the binary is not called jamp.exe/jasp.exe - there's apparently a Jedi Academy specific workaround in the driver. So for now, don't rename the exe