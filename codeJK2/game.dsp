# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=game - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 FinalBuild" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Code/game", FSLAAAAA"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
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
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /I "..\ICARUS" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Release/efgamex86.dll"
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
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /W3 /Gm /Gi /GX /ZI /Od /I "..\ICARUS" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug/game.bsc"
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:I386 /out:"..\Debug/efgamex86.dll"
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
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /I "..\ICARUS" /D "_WINDOWS" /D "WIN32" /D "NDEBUG" /D "FINAL_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Release/efgamex86.dll"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\FinalBuild/efgamex86.dll"
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "game - Win32 Release"
# Name "game - Win32 Debug"
# Name "game - Win32 FinalBuild"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "ICARUS"

# PROP Default_Filter "*.cpp, *.h"
# Begin Source File

SOURCE=..\Icarus\BlockStream.cpp
# End Source File
# Begin Source File

SOURCE=..\Icarus\BlockStream.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\ICARUS.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Instance.cpp
# End Source File
# Begin Source File

SOURCE=..\Icarus\Instance.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequence.cpp
# End Source File
# Begin Source File

SOURCE=..\Icarus\sequence.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequencer.cpp
# End Source File
# Begin Source File

SOURCE=..\Icarus\Sequencer.h
# End Source File
# Begin Source File

SOURCE=..\Icarus\TaskManager.cpp
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

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bg_misc.cpp
# End Source File
# Begin Source File

SOURCE=.\bg_pangles.cpp
# End Source File
# Begin Source File

SOURCE=.\bg_panimate.cpp
# End Source File
# Begin Source File

SOURCE=.\bg_pmove.cpp
# End Source File
# Begin Source File

SOURCE=.\bg_slidemove.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_camera.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_consolecmds.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_draw.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_drawtools.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_effects.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_ents.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_event.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_info.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_localents.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_main.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_marks.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_players.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_playerstate.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_predict.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_scoreboard.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_servercmds.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_snapshot.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_syscalls.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_text.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_view.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_weapons.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\fx_scavenger.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxParsing.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxPrimitives.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxScheduler.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxSystem.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxTemplate.cpp
# End Source File
# Begin Source File

SOURCE=..\cgame\FxUtil.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\AI_Jedi.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Stormtrooper.cpp
# End Source File
# Begin Source File

SOURCE=.\AI_Utils.cpp
# End Source File
# Begin Source File

SOURCE=.\g_active.cpp
# End Source File
# Begin Source File

SOURCE=.\g_ambients.cpp
# End Source File
# Begin Source File

SOURCE=.\g_boltons.cpp
# End Source File
# Begin Source File

SOURCE=.\g_breakable.cpp
# End Source File
# Begin Source File

SOURCE=.\g_camera.cpp
# End Source File
# Begin Source File

SOURCE=.\g_client.cpp
# End Source File
# Begin Source File

SOURCE=.\g_cmds.cpp
# End Source File
# Begin Source File

SOURCE=.\g_combat.cpp
# End Source File
# Begin Source File

SOURCE=.\g_functions.cpp
# End Source File
# Begin Source File

SOURCE=.\g_fx.cpp
# End Source File
# Begin Source File

SOURCE=.\g_ICARUS.cpp
# End Source File
# Begin Source File

SOURCE=.\g_infostringLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\g_itemLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\g_items.cpp
# End Source File
# Begin Source File

SOURCE=.\g_main.cpp
# End Source File
# Begin Source File

SOURCE=.\g_mem.cpp
# End Source File
# Begin Source File

SOURCE=.\g_misc.cpp
# End Source File
# Begin Source File

SOURCE=.\g_misc_model.cpp
# End Source File
# Begin Source File

SOURCE=.\g_missile.cpp
# End Source File
# Begin Source File

SOURCE=.\g_mover.cpp
# End Source File
# Begin Source File

SOURCE=.\g_nav.cpp
# End Source File
# Begin Source File

SOURCE=.\g_navigator.cpp
# End Source File
# Begin Source File

SOURCE=.\g_navnew.cpp
# End Source File
# Begin Source File

SOURCE=.\g_object.cpp
# End Source File
# Begin Source File

SOURCE=.\g_objectives.cpp
# End Source File
# Begin Source File

SOURCE=.\g_ref.cpp
# End Source File
# Begin Source File

SOURCE=.\g_roff.cpp
# End Source File
# Begin Source File

SOURCE=.\g_savegame.cpp
# End Source File
# Begin Source File

SOURCE=.\g_session.cpp
# End Source File
# Begin Source File

SOURCE=.\g_spawn.cpp
# End Source File
# Begin Source File

SOURCE=.\g_squad.cpp
# End Source File
# Begin Source File

SOURCE=.\g_svcmds.cpp
# End Source File
# Begin Source File

SOURCE=.\g_target.cpp
# End Source File
# Begin Source File

SOURCE=.\G_Timer.cpp
# End Source File
# Begin Source File

SOURCE=.\g_trigger.cpp
# End Source File
# Begin Source File

SOURCE=.\g_turret.cpp
# End Source File
# Begin Source File

SOURCE=.\g_usable.cpp
# End Source File
# Begin Source File

SOURCE=.\g_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\g_weapon.cpp
# End Source File
# Begin Source File

SOURCE=.\g_weaponLoad.cpp
# End Source File
# Begin Source File

SOURCE=..\ui\gameinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_behavior.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_combat.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_formation.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_goal.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_misc.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_move.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_reactions.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_senses.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_sounds.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_spawn.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_stats.cpp
# End Source File
# Begin Source File

SOURCE=.\NPC_utils.cpp
# End Source File
# Begin Source File

SOURCE=.\Q3_Interface.cpp
# End Source File
# Begin Source File

SOURCE=.\Q3_Registers.cpp
# End Source File
# Begin Source File

SOURCE=.\q_math.cpp
# End Source File
# Begin Source File

SOURCE=.\q_shared.cpp
# End Source File
# Begin Source File

SOURCE=..\qcommon\tri_coll_test.cpp
# End Source File
# Begin Source File

SOURCE=.\wp_saber.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Group "Icarus Headers"

# PROP Default_Filter "*.h"
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

SOURCE=..\cgame\cg_local.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_media.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_public.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_text.h
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

SOURCE=.\boltons.h
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

SOURCE=.\channels.h
# End Source File
# Begin Source File

SOURCE=.\characters.h
# End Source File
# Begin Source File

SOURCE=.\events.h
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

SOURCE=.\g_infostrings.h
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

SOURCE=.\g_squad.h
# End Source File
# Begin Source File

SOURCE=..\ui\gameinfo.h
# End Source File
# Begin Source File

SOURCE=.\ghoul2_shared.h
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

SOURCE=.\speakers.h
# End Source File
# Begin Source File

SOURCE=.\statindex.h
# End Source File
# Begin Source File

SOURCE=.\surfaceflags.h
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
# End Group
# Begin Source File

SOURCE=..\base\ext_data\boltOns.cfg
# End Source File
# Begin Source File

SOURCE=.\game.def
# End Source File
# Begin Source File

SOURCE=..\base\ext_data\infostrings.dat
# End Source File
# Begin Source File

SOURCE=..\base\ext_data\items.dat
# End Source File
# Begin Source File

SOURCE=..\base\ext_data\NPCs.cfg
# End Source File
# Begin Source File

SOURCE=..\base\ext_data\weapons.dat
# End Source File
# End Target
# End Project
