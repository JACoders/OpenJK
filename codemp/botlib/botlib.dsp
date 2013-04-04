# Microsoft Developer Studio Project File - Name="botlib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=botlib - Win32 Release JK2
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "botlib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "botlib.mak" CFG="botlib - Win32 Release JK2"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "botlib - Win32 Release JK2" (based on "Win32 (x86) Static Library")
!MESSAGE "botlib - Win32 Debug JK2" (based on "Win32 (x86) Static Library")
!MESSAGE "botlib - Win32 Final JK2" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/jedi/codemp/botlib", EAAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "botlib - Win32 Release JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "botlib___Win32_Release_TA"
# PROP BASE Intermediate_Dir "botlib___Win32_Release_TA"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Release"
# PROP Intermediate_Dir "../Release/botlib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /O2 /D "WIN32" /D "NDebug" /D "_MBCS" /D "_LIB" /D "BOTLIB" /YX /FD /c
# ADD CPP /nologo /G6 /W4 /GX /Zi /O2 /I "../game" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BOTLIB" /D "_JK2" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "botlib - Win32 Debug JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "botlib___Win32_Debug_TA"
# PROP BASE Intermediate_Dir "botlib___Win32_Debug_TA"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../Debug"
# PROP Intermediate_Dir "../Debug/botlib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_Debug" /D "_MBCS" /D "_LIB" /D "BOTLIB" /D "Debug" /FR /YX /FD /GZ /c
# ADD CPP /nologo /G6 /W3 /Gm /GX /ZI /Od /I "../game" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BOTLIB" /D "_JK2" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_Debug"
# ADD RSC /l 0x409 /d "_Debug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "botlib - Win32 Final JK2"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "botlib___Win32_Final_JK2"
# PROP BASE Intermediate_Dir "botlib___Win32_Final_JK2"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../Final"
# PROP Intermediate_Dir "../Final/botlib"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /W4 /GX /Zi /O2 /I "../jk2/game" /D "NDebug" /D "WIN32" /D "_MBCS" /D "_LIB" /D "BOTLIB" /D "_JK2" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /G6 /W4 /GX /O2 /I "../game" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "BOTLIB" /D "WIN32" /D "_JK2" /D "FINAL_BUILD" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x409 /d "NDebug"
# ADD RSC /l 0x409 /d "NDebug"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "botlib - Win32 Release JK2"
# Name "botlib - Win32 Debug JK2"
# Name "botlib - Win32 Final JK2"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\be_aas_bspq3.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_debug.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_entity.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_file.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_main.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_move.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_optimize.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_reach.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_route.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_routealt.cpp
# End Source File
# Begin Source File

SOURCE=.\be_aas_sample.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_char.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_chat.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_gen.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_goal.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_move.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_weap.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ai_weight.cpp
# End Source File
# Begin Source File

SOURCE=.\be_ea.cpp
# End Source File
# Begin Source File

SOURCE=.\be_interface.cpp
# End Source File
# Begin Source File

SOURCE=.\l_crc.cpp
# End Source File
# Begin Source File

SOURCE=.\l_libvar.cpp
# End Source File
# Begin Source File

SOURCE=.\l_log.cpp
# End Source File
# Begin Source File

SOURCE=.\l_memory.cpp
# End Source File
# Begin Source File

SOURCE=.\l_precomp.cpp
# End Source File
# Begin Source File

SOURCE=.\l_script.cpp
# End Source File
# Begin Source File

SOURCE=.\l_struct.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\aasfile.h
# End Source File
# Begin Source File

SOURCE=..\game\be_aas.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_bsp.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_cluster.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_debug.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_def.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_entity.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_file.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_funcs.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_main.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_move.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_optimize.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_reach.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_route.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_routealt.h
# End Source File
# Begin Source File

SOURCE=.\be_aas_sample.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_char.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_chat.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_gen.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_goal.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_move.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ai_weap.h
# End Source File
# Begin Source File

SOURCE=.\be_ai_weight.h
# End Source File
# Begin Source File

SOURCE=..\game\be_ea.h
# End Source File
# Begin Source File

SOURCE=.\be_interface.h
# End Source File
# Begin Source File

SOURCE=..\game\botlib.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\cm_public.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\disablewarnings.h
# End Source File
# Begin Source File

SOURCE=.\l_crc.h
# End Source File
# Begin Source File

SOURCE=.\l_libvar.h
# End Source File
# Begin Source File

SOURCE=.\l_log.h
# End Source File
# Begin Source File

SOURCE=.\l_memory.h
# End Source File
# Begin Source File

SOURCE=.\l_precomp.h
# End Source File
# Begin Source File

SOURCE=.\l_script.h
# End Source File
# Begin Source File

SOURCE=.\l_struct.h
# End Source File
# Begin Source File

SOURCE=.\l_utils.h
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\qcommon.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\qfiles.h
# End Source File
# Begin Source File

SOURCE=..\server\server.h
# End Source File
# Begin Source File

SOURCE=..\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\tags.h
# End Source File
# End Group
# End Target
# End Project
