# Microsoft Developer Studio Project File - Name="JK2cgame" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=JK2cgame - Win32 Release JK2
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "JK2_cgame.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "JK2_cgame.mak" CFG="JK2cgame - Win32 Release JK2"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "JK2cgame - Win32 Release JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "JK2cgame - Win32 Debug JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "JK2cgame - Win32 Final JK2" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/jedi/codemp/cgame", ICAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "JK2cgame - Win32 Release JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "JK2cgame___Win32_Release_TA"
# PROP BASE Intermediate_Dir "JK2cgame___Win32_Release_TA"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Release"
# PROP Intermediate_Dir "../Release/JK2cgame"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /O2 /D "WIN32" /D "NDebug" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /I ".." /I "./../game" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "_JK2" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDebug" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDebug" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /machine:I386 /def:".\JK2_cgame.def" /out:"../Release/cgamex86.dll"
# SUBTRACT BASE LINK32 /debug
# ADD LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /debug /machine:I386 /def:".\JK2_cgame.def" /out:"../Release/cgamex86.dll"

!ELSEIF  "$(CFG)" == "JK2cgame - Win32 Debug JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "JK2cgame___Win32_Debug_TA"
# PROP BASE Intermediate_Dir "JK2cgame___Win32_Debug_TA"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "../Debug/JK2cgame"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_Debug" /D "_WINDOWS" /FR /YX /FD /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I ".." /I "./../game" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "_JK2" /D "JK2AWARDS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_Debug" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_Debug" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_Debug"
# ADD RSC /l 0x409 /d "_Debug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /debug /machine:I386 /out:"..\Debug/cgamex86.dll"
# SUBTRACT BASE LINK32 /profile /nodefaultlib
# ADD LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /debug /machine:I386 /def:".\JK2_cgame.def" /out:"..\Debug\cgamex86.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "JK2cgame - Win32 Final JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "../Final"
# PROP BASE Intermediate_Dir "../Final/JK2cgame"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Final"
# PROP Intermediate_Dir "../Final/JK2cgame"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /Zi /O2 /I ".." /I "../../jk2/game" /D "NDebug" /D "WIN32" /D "_WINDOWS" /D "MISSIONPACK" /D "_JK2" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /W4 /GX /O2 /I ".." /I "./../game" /D "NDEBUG" /D "_WINDOWS" /D "MISSIONPACK" /D "WIN32" /D "_JK2" /D "FINAL_BUILD" /YX /FD /c
# ADD BASE MTL /nologo /D "NDebug" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDebug" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /machine:I386 /def:".\JK2_cgame.def" /out:"../Final/cgamex86.dll"
# ADD LINK32 /nologo /base:"0x30000000" /subsystem:windows /dll /map /machine:I386 /def:".\JK2_cgame.def" /out:"../Final/cgamex86.dll"

!ENDIF 

# Begin Target

# Name "JK2cgame - Win32 Release JK2"
# Name "JK2cgame - Win32 Debug JK2"
# Name "JK2cgame - Win32 Final JK2"
# Begin Group "Source Files"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=..\game\bg_lib.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\game\bg_misc.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_panimate.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_pmove.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_saber.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_slidemove.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_weapons.c
# End Source File
# Begin Source File

SOURCE=.\cg_consolecmds.c
# End Source File
# Begin Source File

SOURCE=.\cg_draw.c
# End Source File
# Begin Source File

SOURCE=.\cg_drawtools.c
# End Source File
# Begin Source File

SOURCE=.\cg_effects.c
# End Source File
# Begin Source File

SOURCE=.\cg_ents.c
# End Source File
# Begin Source File

SOURCE=.\cg_event.c
# End Source File
# Begin Source File

SOURCE=.\cg_info.c
# End Source File
# Begin Source File

SOURCE=.\cg_light.c
# End Source File
# Begin Source File

SOURCE=.\cg_localents.c
# End Source File
# Begin Source File

SOURCE=.\cg_main.c
# End Source File
# Begin Source File

SOURCE=.\cg_marks.c
# End Source File
# Begin Source File

SOURCE=.\cg_newDraw.c
# End Source File
# Begin Source File

SOURCE=.\cg_players.c
# End Source File
# Begin Source File

SOURCE=.\cg_playerstate.c
# End Source File
# Begin Source File

SOURCE=.\cg_predict.c
# End Source File
# Begin Source File

SOURCE=.\cg_saga.c
# End Source File
# Begin Source File

SOURCE=.\cg_scoreboard.c
# End Source File
# Begin Source File

SOURCE=.\cg_servercmds.c
# End Source File
# Begin Source File

SOURCE=.\cg_snapshot.c
# End Source File
# Begin Source File

SOURCE=.\cg_strap.c
# End Source File
# Begin Source File

SOURCE=.\cg_syscalls.c
# End Source File
# Begin Source File

SOURCE=.\cg_turret.c
# End Source File
# Begin Source File

SOURCE=.\cg_view.c
# End Source File
# Begin Source File

SOURCE=.\cg_weaponinit.c
# End Source File
# Begin Source File

SOURCE=.\cg_weapons.c
# End Source File
# Begin Source File

SOURCE=.\fx_blaster.c
# End Source File
# Begin Source File

SOURCE=.\fx_bowcaster.c
# End Source File
# Begin Source File

SOURCE=.\fx_bryarpistol.c
# End Source File
# Begin Source File

SOURCE=.\fx_demp2.c
# End Source File
# Begin Source File

SOURCE=.\fx_disruptor.c
# End Source File
# Begin Source File

SOURCE=.\fx_flechette.c
# End Source File
# Begin Source File

SOURCE=.\fx_force.c
# End Source File
# Begin Source File

SOURCE=.\fx_heavyrepeater.c
# End Source File
# Begin Source File

SOURCE=.\fx_rocketlauncher.c
# End Source File
# Begin Source File

SOURCE=..\game\q_math.c
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.c
# End Source File
# Begin Source File

SOURCE=..\ui\ui_shared.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\game\anims.h
# End Source File
# Begin Source File

SOURCE=.\animtable.h
# End Source File
# Begin Source File

SOURCE=..\game\bg_local.h
# End Source File
# Begin Source File

SOURCE=..\game\bg_public.h
# End Source File
# Begin Source File

SOURCE=..\game\bg_saga.h
# End Source File
# Begin Source File

SOURCE=..\game\bg_strap.h
# End Source File
# Begin Source File

SOURCE=..\game\bg_weapons.h
# End Source File
# Begin Source File

SOURCE=.\cg_lights.h
# End Source File
# Begin Source File

SOURCE=.\cg_local.h
# End Source File
# Begin Source File

SOURCE=.\cg_public.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\disablewarnings.h
# End Source File
# Begin Source File

SOURCE=.\fx_local.h
# End Source File
# Begin Source File

SOURCE=..\ghoul2\G2.h
# End Source File
# Begin Source File

SOURCE=.\JK2_cgame.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\ui\keycodes.h
# End Source File
# Begin Source File

SOURCE=..\..\ui\menudef.h
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\qfiles.h
# End Source File
# Begin Source File

SOURCE=..\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\tags.h
# End Source File
# Begin Source File

SOURCE=.\tr_types.h
# End Source File
# Begin Source File

SOURCE=..\ui\ui_shared.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\cgame.bat
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\cgame.q3asm
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
