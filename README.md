# JACoders "OpenJK" project #
[![Coverity Scan Build Status](https://scan.coverity.com/projects/1153/badge.svg)](https://scan.coverity.com/projects/1153)

IRC: irc.arloria.net / #JACoders ([webchat](http://unic0rn.github.io/tiramisu/jacoders/))

Forum: http://jkhub.org/forum/51-discussion/

The purpose of this project is to maintain and improve the Jedi Academy and Jedi Outcast games, developed by Raven Software. This project does not attempt to rebalance or otherwise modify core gameplay.

Major enhancement changes are very low priority at this time unless patches are made available and which do not alter the core functionality of gameplay or the stock renderer itself.

Please use discretion when making issue requests on github. The [JKHub sub-forum](http://jkhub.org/forum/51-discussion/) is a better place for larger discussions on changes that aren't actually bugs.

## Jedi Outcast Support ##

_Do not make issues regarding Jedi Outcast support problems at this time.  It is considered mostly unfinished, broken and use at your own risk!_

There is no Multiplayer code for JK2MP at all on our repository at this time.  The Singleplayer support must be explicitly turned on in your own compile.  The prebuilt versions will not include this.

## 64-bit Support

64-bit is currently supported on non-Windows platforms.

## Installation ##

First, install Jedi Academy. If you don't already own the game you can buy it from online stores such as [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [Play](http://www.play.com/Games/PC/4-/127805/Star-Wars-Jedi-Knight-Jedi-Academy/Product.html?searchstring=jedi+academy&searchsource=0&searchtype=allproducts&urlrefer=search). Then unpack the OpenJK zip file to your Jedi Academy GameData folder.

## Maintainers (in alphabetical order) ##

* eezstreet
* Ensiform
* Razish
* redsaurus
* Xycaleth

## Significant contributors (in alphabetical order) ##

* exidl
* ImperatorPrime
* mrwonko
* Scooper
* Sil

## Dependencies ##

* OpenGL
* OpenAL (included on Windows)
* libpng (included on Windows)
* libjpeg (included on Windows)
* zlib (included on Windows)

## Dedicated Server ##

In order to run dedicated server, you must use the openjkded binary, running dedicated from the main executable is currently not allowed because it is broken with the addition of modular renderer.

## Developer Notes ##

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.

Please be aware of the implications of the GPLv2 licence. In short, be prepared to share your code.

### If you wish to contribute to OpenJK, please do the following ###
* [Fork](https://github.com/JACoders/OpenJK/fork) the project on Github
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (JACoders/OpenJK)

### If you wish to base your work off OpenJK (mod or engine) ###
* [Fork](https://github.com/JACoders/OpenJK/fork) the project on Github
* Change the GAMEVERSION define in codemp/game/g_local.h from "OpenJK" to your project name
* If you make a nice change, please consider backporting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.

### Reserved renderer names ###
* rd-vanilla (JA's original renderer)
* rd-dedicated (Stripped down renderer for use with dedicated server)
* rd-rend2 (Maintainer: Xycaleth)
* rd-es (OpenGL ES compliant renderer for use with Raspberry Pi, Ouya, etc - please contribute!)
* rd-strawberry (Maintainer: Xycaleth)
* rd-vader (Maintainer: mrwonko)
* rd-palpatine (Maintainer: mrwonko)
* rd-sidious (Maintainer: mrwonko)

### Engine "hax" ###
* You will no longer be able to use runtime memory patches. Consider forking OpenJK and adding your engine modifications directly.

### Links of Interest ###
* [Buildbot](http://jk.xd.cm/)
* [Buildbot builds](http://builds.openjk.org/)
* [Compilation Guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide)
* [JKHub sub-forum](http://jkhub.org/forum/51-discussion/)
