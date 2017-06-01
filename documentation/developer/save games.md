# Save Games

Save games are handled in `code/server/sv_savegame.cpp`. A save game consists of blocks which start with their type (a 4 byte string) and length; in Jedi Academy they are as follows:

1.  Save Game Version ("_VER" read in `SG_Open()`; must be iSAVEGAME_VERSION)
2.  Comment ("COMM" read in `SG_ReadSavegame()`)
3.  ??? ignored ("CMTM" read in `SG_ReadSavegame()`)
4.  Map ("MPCM" read in `SG_ReadSavegame()`)
5.  CVars: Count ("CVCN" in `SG_ReadCvars()`) and each name and value ("CVAR" and "VALU")
6.  Whether it's an autosave ("GAME")
 
    And if so, it reads various Cvars:
    1.  "playersave" which contains the players' status ("CVSV")
    2.  "playerammo" ("AMMO")
    3.  "playerinv" ("IVTY")
    4.  "playerfplvl" ("FPLV")
    
    Else:
    
    1.  server time ("TIME")
    2.  server residual time ("TIMR")
    3.  Portals ("PRTS")
    4.  Server Config Strings: Count ("CSCN") and each index ("CSIN") and value ("CSDA")
7.  *At this point we enter game code; we are now in `ReadLevel()` in `code/game/g_savegame.cpp`.*

    There's some special case handling for autosaves and transitions (which I believe have to do with hub levels), but typically it then loads the client (`level.clients[0]`, "GCLI"). This is done using `EvaluateFields()`, which first reads the binary data, then possibly adjusts for differences between retail and patch (changed `saberInfo_t`) and finally fixes the pointers in the read data, e.g. strings. Half of the `level_locals_t` data is loaded in the same way ("LVLC").
8.  Objectives ("OBJT")
9.  Effects ("FXLE", 32 \* "FXFN")
10. Entities: `ReadGEntities()`
    1.  All the entities (count = "NMED", count * "EDNM", "GENT", "GNPC", "GCLI", "PARM", "VHIC", "GHL2")
        * "GHL2" is handled by renderer, which has the G2 code.
    2.  Timers
    3.  Icarus `CIcarus::Load()`
        1.  Version ("ICAR", must match `CIcarus::ICARUS_VERSION`)
        2.  Buffer ("ISEQ") - flat buffer that is subsequently parsed to reconstruct the Icarus state
    4.  Icarus Endmarker ("ICOK")
    5.  `g_entityInUseBits` ("INUS")
11. Icarus Variables ("[FS]VAR", "[FS]IDL", "[FS]IDS", "[FS]VAL", "SVSZ")
12. `player_locked` ("LCKD")
13. `CG_ReadTheEvilCGHackStuff()` ("FPSL", "IVSL")
14. End marker ("DONE")
