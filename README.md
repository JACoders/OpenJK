# OpenJK-rpmod

A Jedi Academy multiplayer client, built on [OpenJK](https://github.com/JACoders/OpenJK), with tools that work alongside **RPMod**, the JEDI clan's roleplay mod.

[![build](https://github.com/Ber-til/OpenJK-rpmod/actions/workflows/build.yml/badge.svg)](https://github.com/Ber-til/OpenJK-rpmod/actions/workflows/build.yml)
[![release](https://img.shields.io/github/v/release/Ber-til/OpenJK-rpmod)](https://github.com/Ber-til/OpenJK-rpmod/releases/latest)

## What it is, and what it isn't

OpenJK-rpmod is not a replacement for RPMod, and doesn't try to recreate what RPMod does. It adds tools on top of what RPMod and OpenJK already give you, to make building scenes, events and stories easier, plus quality-of-life features for everyone.

Most of it lives in the engine, so it works with the game module RPMod's server loads. The tools send RPMod's own commands (`rpmodel`, `rpeffect`, `npc`, `rpsound`...), so your usual RPMod permissions apply.

## What's in it

- **Managers** for models, NPCs, shaders, effects, sounds and music: browse every asset your game has, preview it, place it from a free camera, and send it with one click
- **Storyteller**: missions made of steps, played on cue or armed to play when someone walks in, says or uses something
- **Cinematic Manager**: whole cut scenes on one timeline, camera shots, actors, effects and sounds, played for everyone at once
- **Map Editor**: move or hide parts of the map, add models, lights, fog, sounds and triggers, and share them
- **Chat and console**: languages, long chat in parts, capitals, colours while typing, console settings, selection and find, a console key of your own
- **Small things**: radial menus, saber styles, saber light, emote preview, wall stick, toggle crouch, widescreen text and more

Everything is described in [FEATURES.md](FEATURES.md), the commands and settings are in [COMMANDS.md](COMMANDS.md), and what changed in each version is in [CHANGELOG.md](CHANGELOG.md).

### Experimental features

A few features reach other players hidden inside RPMod's `rpnotify`, until RPMod supports them itself. Only players with this client, up to date, see them, and they stay off until you turn on **TOGGLE EXPERIMENTAL** (or `/rpexperimental on`). They're listed in [FEATURES.md](FEATURES.md#experimental-features).

## Installing

You need Jedi Academy installed, from [Steam](https://store.steampowered.com/app/6020/) or [GOG](https://www.gog.com/game/star_wars_jedi_knight_jedi_academy) for example.

1. [Download the latest release](https://github.com/Ber-til/OpenJK-rpmod/releases/latest) for your system: `OpenJK-windows-x86.zip` on Windows.
2. Extract everything in it into Jedi Academy's `GameData` folder, the renderer (`rd-vanilla_x86.dll`) included: several features need it. For Steam that's `<Steam folder>/steamapps/common/Jedi Academy/GameData/`. On Linux and macOS, extract the archive there the same way.
3. If you already used OpenJK, you're ready. If not, tick **Use OpenJK if available** in the RPMod launcher.

## Building

The client is built 32-bit (x86), as RPMod's game modules are. On Windows, from an *x86 Native Tools Command Prompt* for Visual Studio:

```
cmake -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-x86 --target openjk.x86
```

then copy `build-x86/openjk.x86.exe` (and the renderer, if you changed it) into `GameData`. OpenJK's [compilation guide](https://github.com/JACoders/OpenJK/wiki/Compilation-guide) covers the rest, Linux and macOS included.

Bugs and ideas go in the [issues](https://github.com/Ber-til/OpenJK-rpmod/issues).

## License and credits

OpenJK-rpmod is free software under the GPLv2, as OpenJK is: see [LICENSE.txt](LICENSE.txt).

It's built on the work of the [OpenJK team](https://github.com/JACoders/OpenJK#maintainers-full-list-jacoders) and [its contributors](https://github.com/JACoders/OpenJK/graphs/contributors), who keep Jedi Academy running on today's systems. Changes to OpenJK itself belong [upstream](https://github.com/JACoders/OpenJK).
