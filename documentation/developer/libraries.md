# Library loading

The executable containing the client is separated from the game code in Jedi Academy; this allows for the creation of game mods without changing the client itself. Initially this was the only way of creating mods since the client's source code was not released.

For multiplayer there are the following files:

*   **MP Client** - openjk.x86.exe or similar
    
    The main engine code, which loads the game libraries.
*   **MP Client Game Library** - cgamex86.dll or similar
    
    The client-side game code.
*   **MP Game Library** - jampgamex86.dll or similar
    
    The server-side game code.
*   **MP UI Library** - uix86.dll or similar
    
    The user interface code, including the menu code.

On startup the client searches for the libraries in the current mod's directory (as given in the `fs_game` cvar), falling back to those from base if none are found.

Historically communication with these libraries was done using two functions exported by the libraries:

*   `vmMain( int command, ... )`
    
    This is how the client calls library functions. Based on the command it would call various functions.
*   `dllEntry( intptr_t (*syscallptr)( intptr_t arg,... ) )`
    
    Called once upon loading the dll. The supplied syscallptr is saved by the library; it is the client function to be called whenever the library needs something from the client. Similar to `vmMain` the first argument is the requested call, according to which the arguments are interpreted.

This has its roots in the Quake 3 days, where the code was run in the Quake Virtual Machine (QVM). Since prior to the release of the full source code every mod worked this way, OpenJK still supports this to remain backwards compatible.

Additionally OpenJK introduced `<library>Export_t* GetModuleAPI( int apiVersion, <library>Import_t *import )`, which is a type-safe way of achieving the same thing without the additional layer of indirection by directly passing the function pointers in question.
