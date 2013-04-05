# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=game - Win32 SHDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 SHDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 FinalBuild" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 SHDebug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Code/game", FSLAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "game - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /I "..\ICARUS" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /Yu"g_headers.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Release/jk2gamex86.dll"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /W4 /Gm /Gi /GX /ZI /Od /I "..\ICARUS" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /FR /Yu"g_headers.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:I386 /out:"..\Debug/jk2gamex86.dll"
# SUBTRACT LINK32 /incremental:no /map

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "FinalBuild"
# PROP BASE Intermediate_Dir "FinalBuild"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\FinalBuild"
# PROP Intermediate_Dir "..\FinalBuild\game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /Zi /O2 /I "..\ICARUS" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GX /O2 /I "..\ICARUS" /D "NDEBUG" /D "FINAL_BUILD" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /Yu"g_headers.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Release/efgamex86.dll"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /machine:I386 /out:"..\FinalBuild/jk2gamex86.dll"
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "game___Win32_SHDebug"
# PROP BASE Intermediate_Dir "game___Win32_SHDebug"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\SHDebug"
# PROP Intermediate_Dir "..\SHDebug\game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /Gm /Gi /GX /ZI /Od /I "..\ICARUS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /Yu"g_headers.h" /FD /c
# ADD CPP /nologo /G6 /W4 /Gm /Gi /GX /ZI /Od /I "..\ICARUS" /D "_DEBUG" /D "MEM_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /FR /Yu"g_headers.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:I386 /out:"..\Debug/jk2gamex86.dll"
# SUBTRACT BASE LINK32 /incremental:no /map
# ADD LINK32 ../shdebug/game/smrtheap.obj kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:I386 /out:"..\SHDebug/jk2gamex86.dll"
# SUBTRACT LINK32 /incremental:no /map

!ENDIF 

# Begin Target

# Name "game - Win32 Release"
# Name "game - Win32 Debug"
# Name "game - Win32 FinalBuild"
# Name "game - Win32 SHDebug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "ICARUS"

# PROP Default_Filter "*.cpp, *.h"
# Begin Source File

SOURCE=..\Icarus\BlockStream.cpp
# ADD CPP /Yu"icarus.h"
# End Source File
# Begin Source File

SOURCE=..\Icarus\BlockStream.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\ICARUS.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Instance.cpp
# ADD CPP /Yu"icarus.h"
# End Source File
# Begin Source File

SOURCE=..\Icarus\Instance.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequence.cpp
# ADD CPP /Yu"icarus.h"
# End Source File
# Begin Source File

SOURCE=..\Icarus\sequence.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequencer.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequencer.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\TaskManager.cpp
# ADD CPP /Yu"icarus.h"
# End Source File
# Begin Source File

SOURCE=..\Icarus\TaskManager.h
# End Source File
# End Group
# Begin Group "Cgame"

# PROP Default_Filter "*.cpp"
# Begin Source File

SOURCE=.\bg_lib.cpp

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bg_misc.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\bg_pangles.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\bg_panimate.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\bg_pmove.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\bg_slidemove.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_camera.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_consolecmds.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_credits.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_draw.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_drawtools.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_effects.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_ents.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_event.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_headers.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_info.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_lights.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_localents.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_main.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_marks.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_players.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_playerstate.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_predict.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_scoreboard.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_servercmds.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_snapshot.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_syscalls.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_text.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_view.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_weapons.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_ATSTMain.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_Blaster.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_Bowcaster.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_BryarPistol.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_DEMP2.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_Disruptor.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_Emplaced.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_Flechette.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_HeavyRepeater.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FX_RocketLauncher.cpp
# ADD CPP /Yu"cg_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxParsing.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxPrimitives.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxScheduler.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxSystem.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxTemplate.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\cgame\FxUtil.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\AI_Atst.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\AI_Default.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Droid.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\AI_GalakMech.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Grenadier.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Howler.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_ImperialProbe.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\AI_Interrogator.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Jedi.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\AI_Mark1.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Mark2.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_MineMonster.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Remote.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Seeker.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Sentry.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Sniper.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Stormtrooper.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\AI_Utils.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_active.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_breakable.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_camera.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_client.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_cmds.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_combat.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_functions.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_fx.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_headers.cpp
# ADD CPP /Yc"../cgame/cg_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_ICARUS.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_inventory.cpp
# End Source File
# Begin Source File

SOURCE=.\g_itemLoad.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_items.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_main.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_mem.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_misc.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_misc_model.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_missile.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_mover.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_nav.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_navigator.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_navnew.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_object.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_objectives.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_ref.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_roff.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_savegame.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_session.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_spawn.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_svcmds.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_target.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\G_Timer.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_trigger.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_turret.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_usable.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_utils.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_weapon.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\g_weaponLoad.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\ui\gameinfo.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\genericparser2.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\NPC.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_behavior.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_combat.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_goal.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_misc.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_move.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_reactions.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_senses.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_sounds.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_spawn.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_stats.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\NPC_utils.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\Q3_Interface.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\Q3_Registers.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# Begin Source File

SOURCE=.\q_math.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\q_shared.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=..\smartheap\SMRTHEAP.C

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\qcommon\tri_coll_test.cpp
# ADD CPP /Yu"common_headers.h"
# End Source File
# Begin Source File

SOURCE=.\wp_saber.cpp
# ADD CPP /Yu"g_headers.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Group "Icarus Headers"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\g_headers.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\interface.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Interpreter.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Tokenizer.h
# End Source File
# End Group
# Begin Group "CGame Headers"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=..\cgame\animtable.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_headers.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_local.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_media.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_public.h
# End Source File
# Begin Source File

SOURCE=..\cgame\common_headers.h
# End Source File
# Begin Source File

SOURCE=..\cgame\FxParsing.h
# End Source File
# Begin Source File

SOURCE=..\cgame\FxPrimitives.h
# End Source File
# Begin Source File

SOURCE=..\cgame\FxScheduler.h
# End Source File
# Begin Source File

SOURCE=..\cgame\FxSystem.h
# End Source File
# Begin Source File

SOURCE=..\cgame\FxUtil.h
# End Source File
# Begin Source File

SOURCE=..\cgame\strip_objectives.h
# End Source File
# Begin Source File

SOURCE=..\renderer\tr_types.h
# End Source File
# Begin Source File

SOURCE=..\client\vmachine.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\AI.h
# End Source File
# Begin Source File

SOURCE=.\anims.h
# End Source File
# Begin Source File

SOURCE=.\b_local.h
# End Source File
# Begin Source File

SOURCE=.\b_public.h
# End Source File
# Begin Source File

SOURCE=.\bg_local.h
# End Source File
# Begin Source File

SOURCE=.\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\bset.h
# End Source File
# Begin Source File

SOURCE=.\bstate.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_camera.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_lights.h
# End Source File
# Begin Source File

SOURCE=.\channels.h
# End Source File
# Begin Source File

SOURCE=.\characters.h
# End Source File
# Begin Source File

SOURCE=.\common_headers.h
# End Source File
# Begin Source File

SOURCE=.\dmstates.h
# End Source File
# Begin Source File

SOURCE=.\events.h
# End Source File
# Begin Source File

SOURCE=..\ff\ff.h
# End Source File
# Begin Source File

SOURCE=..\ff\ff_public.h
# End Source File
# Begin Source File

SOURCE=..\client\fffx.h
# End Source File
# Begin Source File

SOURCE=.\fields.h
# End Source File
# Begin Source File

SOURCE=..\ghoul2\G2.h
# End Source File
# Begin Source File

SOURCE=.\g_functions.h
# End Source File
# Begin Source File

SOURCE=.\g_icarus.h
# End Source File
# Begin Source File

SOURCE=.\g_items.h
# End Source File
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\g_nav.h
# End Source File
# Begin Source File

SOURCE=.\g_navigator.h
# End Source File
# Begin Source File

SOURCE=.\g_public.h
# End Source File
# Begin Source File

SOURCE=.\g_roff.h
# End Source File
# Begin Source File

SOURCE=.\g_shared.h
# End Source File
# Begin Source File

SOURCE=..\ui\gameinfo.h
# End Source File
# Begin Source File

SOURCE=.\genericparser2.h
# End Source File
# Begin Source File

SOURCE=.\ghoul2_shared.h
# End Source File
# Begin Source File

SOURCE=..\renderer\mdx_format.h
# End Source File
# Begin Source File

SOURCE=.\npc_headers.h
# End Source File
# Begin Source File

SOURCE=.\objectives.h
# End Source File
# Begin Source File

SOURCE=.\Q3_Interface.h
# End Source File
# Begin Source File

SOURCE=.\Q3_Registers.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\say.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\sstring.h
# End Source File
# Begin Source File

SOURCE=.\statindex.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\stripPublic.h
# End Source File
# Begin Source File

SOURCE=.\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\tags.h
# End Source File
# Begin Source File

SOURCE=.\teams.h
# End Source File
# Begin Source File

SOURCE=.\weapons.h
# End Source File
# Begin Source File

SOURCE=.\wp_saber.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\game.def
# End Source File
# End Group
# Begin Source File

SOURCE=..\base\ext_data\items.dat

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\base\ext_data\NPCs.cfg

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\base\ext_data\weapons.dat

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\smartheap\HAW32M.LIB

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 SHDebug"

!ENDIF 

# End Source File
# End Target
# End Project
