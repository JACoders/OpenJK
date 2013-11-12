
General Info
============

This project uses [cmake][]. Platform specific instructions are below. More detailed instructions can be found in the [project wiki][]. For windows, a [separate tutorial exists][].

[cmake]: http://www.cmake.org/
[project wiki]: https://github.com/JACoders/OpenJK/wiki/Compilation-guide
[separate tutorial exists]: http://jkhub.org/tutorials/article/145-compiling-openjk-win32-must-read-for-new-coders/

In general the process is:

- Install dependencies (sdl2, zlib)
- Configure the project using cmake into a `build` folder
- Compile

Windows
=======

Configure
---------

Run cmake, either using the gui or command line, specifying a build folder as the target. For more information, see the [separate tutorial][].

[separate tutorial]: http://jkhub.org/tutorials/article/145-compiling-openjk-win32-must-read-for-new-coders/

Compile (Visual Studio)
-----------------------

[Visual Studio Express for Windows Desktop][] is available for free from Microsoft.

[Visual Studio Express for Windows Desktop]: http://www.visualstudio.com/downloads/download-visual-studio-vs#d-express-windows-desktop

- Open the `OpenJK.sln` file in the `build` folder.
- Select the build configuration to use (Debug/Release/RelWithDebInfo/MinSizeRel).
- Build the solution.
- Built files can be found in `build/<project name>/<build configuration>/`.

Linux
=====

OpenJK currently only supports 32-bit on both Mac and Linux. Due to the difficulties of setting up 32-bit dependencies on 64-bit linux, it may be easiest to setup a virtual machine running 32-bit linux and building there.

Install Dependencies
--------------------

	sudo apt-get install build-essential cmake git libopenal-dev zlib1g-dev libpng12-dev

If installing on 64-bit linux, the following dependencies are also needed:

	sudo apt-get install libc6:i386 libgcc1:i386 gcc-4.8-base:i386 libstdc++5:i386 libstdc++6:i386 gcc-multilib g++-multilib libopenal-dev:i386 libsdl2-dev libsdl2-2.0-0:i386 ia32-libs

Install SDL2 Development Files
------------------------------

SDL2 packages are available in Debian testing (jessie) and Ubuntu 13.10 (saucy), and for earlier Ubuntu releases from the Mir staging PPA.

Adding the PPA on Ubuntu 12.04 LTS, 12.10, and 13.04:

	sudo add-apt-repository ppa:mir-team/staging
	sudo apt-get update

Installing:

	sudo apt-get install libsdl2-dev

More instructions can be found on the [project wiki][], such as how to compile from sdl2 from source.

Configure Build (make)
----------------------

	mkdir build
	cd build
	cmake -G 'Unix Makefiles' ..

For an interactive build that prompts you for values (such as install prefix), run

	cmake -G 'Unix Makefiles' -i ..

Compile (make)
--------------

	make

To compile using multiple cores (jobs), specify the number using the command:

	make -j2

Running
-------

Copy the generated `.so` files and the `.i386` or `.x86_64` binaries to the folder with the JKA `base` folder. Then move all `.so`s except for `rd-vanilla.so` into base, and launch the game using one of the binaries. For example, on a 32-bit system:

	# from within the JKA folder
	./openjk.i386

If you configured an install prefix (e.g. the path to you JKA assets) during the cmake step, you can automatically copy the files using

	make install

OSX
===

Install Homebrew
----------------

Follow the directions at https://github.com/mxcl/homebrew.

Install Xcode
-------------

You will also need Xcode (via AppStore), and agree to the xcode license via

	sudo xcodebuild -license

The command line tools are useful too:

	xcode-select --install

Install Library Dependencies
--------------------

	brew install cmake
	brew install sdl2 --universal

Configure Build (make)
----------------------

	mkdir build
	cd build
	cmake -G 'Unix Makefiles' -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_BUILD_TYPE=Release -DUseInternalPNG=1 ..

You can also use `cmake -G 'Unix Makefiles' -i`, which will interactively prompt you for various options.

Configure Build (Xcode)
-----------------------

If you'd rather use Xcode for editing, then change the above cmake command to be:

	cmake -G Xcode -DCMAKE_OSX_ARCHITECTURES=i386 -DCMAKE_BUILD_TYPE=Release -DUseInternalPNG=1 ..

Please note that while compilation via Xcode works, running via is not configured properly. But that likely does not stop you from attaching to a running process in Xcode.

Compile (make)
--------------

	make

If you want to use all your cores, include a `-j` option:

	make -j2

The above would use two cores to run jobs.

Compile (Xcode)
---------------

Compiling on the command line using the Xcode toolchain:

	xcodebuild -parallelizeTargets

The above would run jobs in parallel.

Compiling within in Xcode is as simple as clicking the big Play button.

Copy Assets
-----------

This step depends on where your legal install of JKA is. But here is an example via the Steam install of JKA:

	mkdir -p ~/Library/Application\ Support/OpenJK
	cp -r ~/Library/Application\ Support/Steam/SteamApps/common/Jedi\ Academy/SWJKJA.app/Contents/base/ ~/Library/Application\ Support/OpenJK/base/

This example copies the base folder from the steam install (containing the game assets) into one of the places that the game automatically uses. This only needs to happen once.

Handle CMake Bugs
-----------------

There is currently a bug in the cmake script where the `.dylib`s are not copied to the appropriate destinations.

	# copy the vanilla renderer binaries to the build root
	find . -name 'rd*.dylib' | xargs -I{} cp -f {} .

	# copy the game binaries to the local base folder
	mkdir -p base && find code codemp -name '*.dylib' | xargs -I{} cp -f {} ./base/

Running
-------

	# Single player
	./openjk_sp.x86.app/Contents/MacOS/openjk_sp.x86

	# Multiplayer
	./openjk.x86.app/Contents/MacOS/openjk.x86

