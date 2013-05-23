# JACoders "OpenJK" project #
IRC: irc.arloria.net / #JACoders ([webchat](http://unic0rn.github.io/tiramisu/jacoders/))

The purpose of this project is to maintain and improve the Jedi Academy and Jedi Outcast games, developed by Raven Software.
This project will not attempt to rebalance or otherwise modify core gameplay aspects.

## Installation ##

First, install Jedi Academy. If you don't already own the game you can buy it from either [Steam](http://store.steampowered.com/app/6020/), [Amazon](http://www.amazon.com/Star-Wars-Jedi-Knight-Academy-Pc/dp/B0000A2MCN) or [Play](http://www.play.com/Games/PC/4-/127805/Star-Wars-Jedi-Knight-Jedi-Academy/Product.html?searchstring=jedi+academy&searchsource=0&searchtype=allproducts&urlrefer=search).

Then point the OpenJK installer to the GameData folder in the Jedi Academy install, e.g.  just point it to your "Jedi Academy/GameData" folder. If you've downloaded an archive, just unpack it to GameData.

## Maintainers (in alphabetical order) ##
* eezstreet
* Ensiform
* mrwonko
* Raz0r
* redsaurus

## Significant contributors (in alphabetical order) ##
* exidl
* Scooper
* Sil
* Xycaleth

## Dependencies ##
* OpenGL
* OpenAL (included on Windows)
* libpng (optional)
* libjpeg (optional)
* zlib (included on Windows)

## Dedicated Server ##
In order to run dedicated server, you must use the openjkded binary, running dedicated from the main executable is currently not allowed because it is broken with the addition of modular renderer.

## Developer Notes ##

OpenJK is licensed under GPLv2 as free software. You are free to use, modify and redistribute OpenJK following the terms in LICENSE.txt.

Please be aware of the implications of the GPLv2 licence. In short, be prepared to share your code.

### If you wish to contribute to OpenJK, please do the following ###
* [Fork](https://github.com/Razish/OpenJK/fork) the project on Github
* Create a new branch and make your changes
* Send a [pull request](https://help.github.com/articles/creating-a-pull-request) to upstream (Razish/OpenJK)

### If you wish to base your work off OpenJK (mod or engine) ###
* [Fork](https://github.com/Razish/OpenJK/fork) the project on Github
* Change the GAMEVERSION define in codemp/game/g_local.h from "basejka_modbase" to your project name
* If you make a nice change, please consider backporting to upstream via pull request as described above. This is so everyone benefits without having to reinvent the wheel for every project.

### Reserved renderer names ###
* rd-vanilla (JA's original renderer)
* rd-dedicated (Stripped down renderer for use with dedicated server)
* rd-raspberry (OpenGL ES compliant renderer for use with Raspberry Pi - feel free to maintain!)
* rd-strawberry (Maintainer: Xycaleth)
* rd-vader (Maintainer: mrwonko)

### Sorry for the history changes! ###
For legal reasons we had to make changes to the history. This likely broke every forker's repo. See [here](http://git-scm.com/docs/git-rebase.html#_recovering_from_upstream_rebase) for how to fix if you've changed anything, or just delete your github fork and local folder and start over if you haven't.

### Engine "hax" ###
* If your mod intends to use engine "hax" to figure out the nedaddr types (NA_IP, etc) NA_BAD is now 0 and NA_BOT is now 1 instead of vice versa.
* You will no longer be able to use runtime memory patches. Consider forking OpenJK and adding your engine modifications directly.

### Links of Interest ###
* [Buildbot](http://jk.xd.cm/)
* [Buildbot builds](http://builds.openjk.org/)
