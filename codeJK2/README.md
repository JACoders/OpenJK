# OpenJK "Jedi Outcast" Singleplayer Code #

This folder contains the partial source required to build the JK2 shared library.  Set BuildJK2SPGame to ON in the main CMakeLists.txt file in the root of the repository to the feature on.

Turn BuildJK2SPEngine and BuildJK2SPRdVanilla to ON to build the JK2 version of the engine with renderer.

Output will be openjo_sp.ARCH(.exe on windows) rdjosp-vanilla_ARCH.[dll/so/dylib] for the engine and renderer.
These should go alongside in GameData next to jk2sp.

The game binary jospgameARCH.[dll/so/dylib] should be placed in the "base" folder.  The original jk2gamex86.dll (Windows-x86) within the root GameData folder is no longer supported, same as with Jedi Academy's jagamex86.dll.
