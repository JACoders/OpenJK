# Microsoft Developer Studio Project File - Name="JK2game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=JK2game - Win32 Release JK2
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "JK2_game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "JK2_game.mak" CFG="JK2game - Win32 Release JK2"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "JK2game - Win32 Release JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "JK2game - Win32 Debug JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "JK2game - Win32 Final JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/jedi/codemp/game", EGAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "JK2game - Win32 Release JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "JK2game___Win32_Release_TA"
# PROP BASE Intermediate_Dir "JK2game___Win32_Release_TA"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Release"
# PROP Intermediate_Dir "../Release/JK2game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /O2 /D "WIN32" /D "NDebug" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "QAGAME" /D "_JK2" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDebug" /mktyplib203 /win32
# ADD MTL /nologo /D "NDebug" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /machine:I386 /out:"..\Release/qaJK2gamex86.dll"
# SUBTRACT BASE LINK32 /incremental:yes /debug
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /def:".\JK2_game.def" /out:"../Release/jk2mpgamex86.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "JK2game - Win32 Debug JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "JK2game___Win32_Debug_TA"
# PROP BASE Intermediate_Dir "JK2game___Win32_Debug_TA"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "../Debug/JK2game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_Debug" /D "_WINDOWS" /D "BUILDING_REF_GL" /D "Debug" /FR /YX /FD /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "BUILDING_REF_GL" /D "Debug" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "QAGAME" /D "_JK2" /D "JK2AWARDS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_Debug" /mktyplib203 /win32
# ADD MTL /nologo /D "_Debug" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_Debug"
# ADD RSC /l 0x409 /d "_Debug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Debug/qaJK2gamex86.dll"
# SUBTRACT BASE LINK32 /incremental:no
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /def:".\JK2_game.def" /out:"..\Debug\jk2mpgamex86.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "JK2game - Win32 Final JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../Final"
# PROP BASE Intermediate_Dir "../Final/JK2game"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Final"
# PROP Intermediate_Dir "../Final/JK2game"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /Zi /O2 /I ".." /I "../../jk2/game" /D "NDebug2" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "QAGAME" /D "_JK2" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /W4 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "MISSIONPACK" /D "QAGAME" /D "WIN32" /D "_JK2" /D "FINAL_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDebug" /mktyplib203 /win32
# ADD MTL /nologo /D "NDebug" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /debug /machine:I386 /def:".\JK2_game.def" /out:"../../Release/jk2mpgamex86.dll"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /map /machine:I386 /def:".\JK2_game.def" /out:"../Final/jk2mpgamex86.dll"
# SUBTRACT LINK32 /pdb:none /debug

!ENDIF 

# Begin Target

# Name "JK2game - Win32 Release JK2"
# Name "JK2game - Win32 Debug JK2"
# Name "JK2game - Win32 Final JK2"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\ai_main.c
# End Source File
# Begin Source File

SOURCE=.\ai_util.c
# End Source File
# Begin Source File

SOURCE=.\ai_wpnav.c
# End Source File
# Begin Source File

SOURCE=.\bg_lib.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\bg_misc.c
# End Source File
# Begin Source File

SOURCE=.\bg_panimate.c
# End Source File
# Begin Source File

SOURCE=.\bg_pmove.c
# End Source File
# Begin Source File

SOURCE=.\bg_saber.c
# End Source File
# Begin Source File

SOURCE=.\bg_slidemove.c
# End Source File
# Begin Source File

SOURCE=.\bg_weapons.c
# End Source File
# Begin Source File

SOURCE=.\g_active.c
# End Source File
# Begin Source File

SOURCE=.\g_arenas.c
# End Source File
# Begin Source File

SOURCE=.\g_bot.c
# End Source File
# Begin Source File

SOURCE=.\g_client.c
# End Source File
# Begin Source File

SOURCE=.\g_cmds.c
# End Source File
# Begin Source File

SOURCE=.\g_combat.c
# End Source File
# Begin Source File

SOURCE=.\g_exphysics.c
# End Source File
# Begin Source File

SOURCE=.\g_ICARUScb.c
# End Source File
# Begin Source File

SOURCE=.\g_items.c
# End Source File
# Begin Source File

SOURCE=.\g_log.c
# End Source File
# Begin Source File

SOURCE=.\g_main.c
# End Source File
# Begin Source File

SOURCE=.\g_mem.c
# End Source File
# Begin Source File

SOURCE=.\g_misc.c
# End Source File
# Begin Source File

SOURCE=.\g_missile.c
# End Source File
# Begin Source File

SOURCE=.\g_mover.c
# End Source File
# Begin Source File

SOURCE=.\g_nav.c
# End Source File
# Begin Source File

SOURCE=.\g_navnew.c
# End Source File
# Begin Source File

SOURCE=.\g_object.c
# End Source File
# Begin Source File

SOURCE=.\g_saga.c
# End Source File
# Begin Source File

SOURCE=.\g_session.c
# End Source File
# Begin Source File

SOURCE=.\g_spawn.c
# End Source File
# Begin Source File

SOURCE=.\g_strap.c
# End Source File
# Begin Source File

SOURCE=.\g_svcmds.c
# End Source File
# Begin Source File

SOURCE=.\g_syscalls.c
# End Source File
# Begin Source File

SOURCE=.\g_target.c
# End Source File
# Begin Source File

SOURCE=.\g_team.c
# End Source File
# Begin Source File

SOURCE=.\g_timer.c
# End Source File
# Begin Source File

SOURCE=.\g_trigger.c
# End Source File
# Begin Source File

SOURCE=.\g_utils.c
# End Source File
# Begin Source File

SOURCE=.\g_weapon.c
# End Source File
# Begin Source File

SOURCE=.\NPC.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Atst.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Default.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Droid.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_GalakMech.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Grenadier.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Howler.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_ImperialProbe.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Interrogator.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Jedi.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Mark1.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Mark2.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_MineMonster.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Remote.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Seeker.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Sentry.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Sniper.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Stormtrooper.c
# End Source File
# Begin Source File

SOURCE=.\NPC_AI_Utils.c
# End Source File
# Begin Source File

SOURCE=.\NPC_behavior.c
# End Source File
# Begin Source File

SOURCE=.\NPC_combat.c
# End Source File
# Begin Source File

SOURCE=.\NPC_goal.c
# End Source File
# Begin Source File

SOURCE=.\NPC_misc.c
# End Source File
# Begin Source File

SOURCE=.\NPC_move.c
# End Source File
# Begin Source File

SOURCE=.\NPC_reactions.c
# End Source File
# Begin Source File

SOURCE=.\NPC_senses.c
# End Source File
# Begin Source File

SOURCE=.\NPC_sounds.c
# End Source File
# Begin Source File

SOURCE=.\NPC_spawn.c
# End Source File
# Begin Source File

SOURCE=.\NPC_stats.c
# End Source File
# Begin Source File

SOURCE=.\NPC_utils.c
# End Source File
# Begin Source File

SOURCE=.\q_math.c
# End Source File
# Begin Source File

SOURCE=.\q_shared.c
# End Source File
# Begin Source File

SOURCE=.\w_force.c
# End Source File
# Begin Source File

SOURCE=.\w_saber.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Group "ICARUS Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\icarus\interpreter.h
# End Source File
# Begin Source File

SOURCE=..\icarus\Q3_Interface.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ai.h
# End Source File
# Begin Source File

SOURCE=.\ai_main.h
# End Source File
# Begin Source File

SOURCE=.\anims.h
# End Source File
# Begin Source File

SOURCE=..\cgame\animtable.h
# End Source File
# Begin Source File

SOURCE=.\b_local.h
# End Source File
# Begin Source File

SOURCE=.\b_public.h
# End Source File
# Begin Source File

SOURCE=.\be_aas.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_char.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_chat.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_gen.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_goal.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_move.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_weap.h
# End Source File
# Begin Source File

SOURCE=.\be_ea.h
# End Source File
# Begin Source File

SOURCE=.\bg_lib.h
# End Source File
# Begin Source File

SOURCE=.\bg_local.h
# End Source File
# Begin Source File

SOURCE=.\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\bg_saga.h
# End Source File
# Begin Source File

SOURCE=.\bg_strap.h
# End Source File
# Begin Source File

SOURCE=.\bg_weapons.h
# End Source File
# Begin Source File

SOURCE=.\botlib.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_local.h
# End Source File
# Begin Source File

SOURCE=..\cgame\cg_public.h
# End Source File
# Begin Source File

SOURCE=.\chars.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\disablewarnings.h
# End Source File
# Begin Source File

SOURCE=..\ghoul2\G2.h
# End Source File
# Begin Source File

SOURCE=.\g_ICARUScb.h
# End Source File
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\g_nav.h
# End Source File
# Begin Source File

SOURCE=.\g_public.h
# End Source File
# Begin Source File

SOURCE=.\g_team.h
# End Source File
# Begin Source File

SOURCE=.\inv.h
# End Source File
# Begin Source File

SOURCE=.\JK2_game.def

!IF  "$(CFG)" == "JK2game - Win32 Release JK2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "JK2game - Win32 Debug JK2"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "JK2game - Win32 Final JK2"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\match.h
# End Source File
# Begin Source File

SOURCE=..\..\ui\menudef.h
# End Source File
# Begin Source File

SOURCE=.\npc_headers.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\say.h
# End Source File
# Begin Source File

SOURCE=.\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\syn.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\tags.h
# End Source File
# Begin Source File

SOURCE=.\teams.h
# End Source File
# Begin Source File

SOURCE=..\cgame\tr_types.h
# End Source File
# Begin Source File

SOURCE=.\w_saber.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\game.bat
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\game.q3asm
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
