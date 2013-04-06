# Microsoft Developer Studio Project File - Name="starwars" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=starwars - Win32 SHDebug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "starwars.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "starwars.mak" CFG="starwars - Win32 SHDebug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "starwars - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "starwars - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "starwars - Win32 FinalBuild" (based on "Win32 (x86) Application")
!MESSAGE "starwars - Win32 SHDebug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Code", EILAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /I "ff/ifc" /D "NDEBUG" /D "_JK2EXE" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /D "_FF" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ALut.lib OpenAL32.lib advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib ff/ifc/ifc22.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386 /out:".\Release/jk2sp.exe"
# SUBTRACT LINK32 /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /Gi /GX /ZI /Od /I "ff/ifc" /D "_DEBUG" /D "_JK2EXE" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /D "_FF" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo"win32\winquake.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug/starwars.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 ALut.lib OpenAL32.lib advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib ff/ifc/ifc22.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386 /out:".\Debug/jk2sp.exe"
# SUBTRACT LINK32 /profile /incremental:no /nodefaultlib

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "starwars___Win32_FinalBuild"
# PROP BASE Intermediate_Dir "starwars___Win32_FinalBuild"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\FinalBuild"
# PROP Intermediate_Dir ".\FinalBuild\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__USEA3D" /D "__A3D_GEOM" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /I "ff/ifc" /D "NDEBUG" /D "FINAL_BUILD" /D "_JK2EXE" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /D "_FF" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386
# SUBTRACT BASE LINK32 /incremental:yes /nodefaultlib
# ADD LINK32 ALut.lib OpenAL32.lib advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib ff/ifc/ifc22.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386 /out:".\FinalBuild/jk2sp.exe"
# SUBTRACT LINK32 /incremental:yes /nodefaultlib

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "starwars___Win32_SHDebug"
# PROP BASE Intermediate_Dir "starwars___Win32_SHDebug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\SHDebug"
# PROP Intermediate_Dir ".\SHDebug\exe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MTd /W4 /Gm /Gi /GX /ZI /Od /D "_NPATCH" /D "_DEBUG" /D "_JK2EXE" /D "WIN32" /D "_WINDOWS" /Fr /YX /FD /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /Gi /GX /ZI /Od /I "ff/ifc" /D "_DEBUG" /D "MEM_DEBUG" /D "_JK2EXE" /D "WIN32" /D "_WINDOWS" /D "_IMMERSION" /D "_FF" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /fo"win32\winquake.res" /d "_DEBUG"
# ADD RSC /l 0x409 /fo"win32\winquake.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo /o"Debug/starwars.bsc"
# ADD BSC32 /nologo /o"SHDebug/starwars.bsc"
LINK32=link.exe
# ADD BASE LINK32 ALut.lib OpenAL32.lib win32/FeelIt/ffc10d.lib advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386 /out:".\Debug/jk2sp.exe"
# SUBTRACT BASE LINK32 /profile /incremental:no /nodefaultlib
# ADD LINK32 ./shdebug/exe/smrtheap.obj ALut.lib OpenAL32.lib advapi32.lib winmm.lib kernel32.lib user32.lib gdi32.lib ole32.lib wsock32.lib ff/ifc/ifc22.lib /nologo /stack:0x800000 /subsystem:windows /map /debug /machine:I386 /out:".\SHDebug/jk2sp.exe"
# SUBTRACT LINK32 /profile /incremental:no /nodefaultlib

!ENDIF 

# Begin Target

# Name "starwars - Win32 Release"
# Name "starwars - Win32 Debug"
# Name "starwars - Win32 FinalBuild"
# Name "starwars - Win32 SHDebug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Group "0_compiled_first"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\0_compiled_first\0_SH_Leak.cpp

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# End Group
# Begin Group "EAX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\client\eax\eax.h
# End Source File
# Begin Source File

SOURCE=.\client\eax\EaxMan.h
# End Source File
# End Group
# Begin Group "OpenAL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\client\OpenAL\al.h
# End Source File
# Begin Source File

SOURCE=.\client\OpenAL\alc.h
# End Source File
# Begin Source File

SOURCE=.\client\OpenAL\alctypes.h
# End Source File
# Begin Source File

SOURCE=.\client\OpenAL\altypes.h
# End Source File
# Begin Source File

SOURCE=.\client\OpenAL\alu.h
# End Source File
# Begin Source File

SOURCE=.\client\OpenAL\alut.h
# End Source File
# End Group
# Begin Group "smartheap"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SHDebug\HA312W32.DLL

!IF  "$(CFG)" == "starwars - Win32 Release"

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smartheap\HEAPAGNT.H

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\SHDebug\SHW32.DLL
# End Source File
# Begin Source File

SOURCE=.\smartheap\SMRTHEAP.C

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# SUBTRACT CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smartheap\SMRTHEAP.H

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smartheap\smrtheap.hpp

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smartheap\HAW32M.LIB

!IF  "$(CFG)" == "starwars - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\client\cl_cgame.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_cin.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_console.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_input.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_keys.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_main.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_mp3.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_parse.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_scrn.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\cl_ui.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_load.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_patch.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_polylib.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_test.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_trace.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cmd.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\common.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\cvar.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\server\exe_headers.cpp
# ADD CPP /Yc"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\qcommon\files.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\game\genericparser2.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\hstring.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\md4.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\msg.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\qcommon\net_chan.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\game\q_math.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\game\q_shared.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\client\snd_ambient.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\snd_dma.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\snd_mem.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\snd_mix.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\client\snd_music.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\qcommon\strip.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\server\sv_ccmds.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_client.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_game.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_init.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_main.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_savegame.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_snapshot.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\qcommon\unzip.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\client\vmachine.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\win32\win_input.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_main.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_shared.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_snd.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_syscon.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_video.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_wndproc.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\game\anims.h
# End Source File
# Begin Source File

SOURCE=.\game\b_public.h
# End Source File
# Begin Source File

SOURCE=.\game\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\BlockStream.h
# End Source File
# Begin Source File

SOURCE=.\game\bset.h
# End Source File
# Begin Source File

SOURCE=.\game\bstate.h
# End Source File
# Begin Source File

SOURCE=.\cgame\cg_public.h
# End Source File
# Begin Source File

SOURCE=.\client\cl_mp3.h
# End Source File
# Begin Source File

SOURCE=.\client\client.h
# End Source File
# Begin Source File

SOURCE=.\client\client_ui.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_local.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_patch.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_polylib.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_public.h
# End Source File
# Begin Source File

SOURCE=.\game\common_headers.h
# End Source File
# Begin Source File

SOURCE=.\server\exe_headers.h
# End Source File
# Begin Source File

SOURCE=.\client\fffx.h
# End Source File
# Begin Source File

SOURCE=.\game\g_functions.h
# End Source File
# Begin Source File

SOURCE=.\game\g_items.h
# End Source File
# Begin Source File

SOURCE=.\game\g_local.h
# End Source File
# Begin Source File

SOURCE=.\game\g_nav.h
# End Source File
# Begin Source File

SOURCE=.\game\g_public.h
# End Source File
# Begin Source File

SOURCE=.\game\g_shared.h
# End Source File
# Begin Source File

SOURCE=.\game\genericparser2.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\hstring.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\ICARUS.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\Instance.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\interface.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\Interpreter.h
# End Source File
# Begin Source File

SOURCE=.\client\keys.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\sequence.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\Sequencer.h
# End Source File
# Begin Source File

SOURCE=.\server\server.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_ambient.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_local.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_music.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_public.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\sstring.h
# End Source File
# Begin Source File

SOURCE=.\game\statindex.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\strip.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\stripPublic.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\TaskManager.h
# End Source File
# Begin Source File

SOURCE=.\game\teams.h
# End Source File
# Begin Source File

SOURCE=.\Icarus\Tokenizer.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\unzip.h
# End Source File
# Begin Source File

SOURCE=.\client\vmachine.h
# End Source File
# Begin Source File

SOURCE=.\game\weapons.h
# End Source File
# Begin Source File

SOURCE=.\win32\win_local.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\win32\background.bmp
# End Source File
# Begin Source File

SOURCE=.\win32\clear.bmp
# End Source File
# Begin Source File

SOURCE=.\EaxMan.dll
# End Source File
# Begin Source File

SOURCE=.\FFC10.dll
# End Source File
# Begin Source File

SOURCE=.\FFC10d.dll
# End Source File
# Begin Source File

SOURCE=.\OpenAL32.dll
# End Source File
# Begin Source File

SOURCE=.\win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\win32\starwars.ico
# End Source File
# Begin Source File

SOURCE=.\win32\winquake.rc
# End Source File
# Begin Source File

SOURCE=.\ALut.lib
# End Source File
# Begin Source File

SOURCE=.\OpenAL32.lib
# End Source File
# End Group
# Begin Group "MP3 Source"

# PROP Default_Filter ""
# Begin Group "MP3 Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mp3code\config.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\copyright.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\htable.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\jdw.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\L3.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\mhead.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\mp3struct.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\port.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\small_header.h
# End Source File
# Begin Source File

SOURCE=.\mp3code\tableawd.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\mp3code\cdct.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\csbt.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\csbtb.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\csbtL3.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cup.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cupini.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cupL1.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cupl3.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cwin.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cwinb.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\cwinm.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\hwin.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\l3dq.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\l3init.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\mdct.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\mhead.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\msis.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\towave.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\uph.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\upsf.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\mp3code\wavep.c
# SUBTRACT CPP /YX
# End Source File
# End Group
# Begin Group "ForceFeedback Source"

# PROP Default_Filter ""
# Begin Group "ForceFeedback Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\FeelIt\FeelBaseTypes.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelBox.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelCompoundEffect.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelCondition.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelConstant.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelDamper.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelDevice.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelDXDevice.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelEffect.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelEllipse.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelEnclosure.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelFriction.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelGrid.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelInertia.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelitAPI.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FEELitIFR.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelMouse.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelPeriodic.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelProjects.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelRamp.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelSpring.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FeelTexture.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FFC.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FFCErrors.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\fffx_feel.h
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "ForceFeedback Binaries"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\FeelIt\FFC10.dll
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FFC10d.dll
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FFC10d.lib
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\FFC10.lib
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\win32\FeelIt\fffx.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\win32\FeelIt\fffx_feel.cpp
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "Ghoul2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ghoul2\G2.h
# End Source File
# Begin Source File

SOURCE=.\ghoul2\G2_API.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ghoul2\G2_bolts.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ghoul2\G2_bones.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ghoul2\G2_misc.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ghoul2\G2_surfaces.cpp
# ADD CPP /Yu"../server/exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\game\ghoul2_shared.h
# End Source File
# Begin Source File

SOURCE=.\renderer\matcomp.h
# End Source File
# Begin Source File

SOURCE=.\renderer\mdx_format.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\MiniHeap.h
# End Source File
# End Group
# Begin Group "Renderer Source"

# PROP Default_Filter ""
# Begin Group "Renderer Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\renderer\glext.h
# End Source File
# Begin Source File

SOURCE=.\win32\glw_win.h
# End Source File
# Begin Source File

SOURCE=.\renderer\qgl.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_font.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_jpeg_interface.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_local.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_public.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_quicksprite.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_stl.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_types.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_WorldEffects.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\renderer\MatComp.c
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_animation.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_backend.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_bsp.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_cmds.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_curve.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_draw.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_font.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_ghoul2.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_image.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_init.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_jpeg_interface.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_light.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_main.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_marks.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_mesh.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_model.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_noise.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_quicksprite.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_scene.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shade.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shade_calc.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shader.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shadows.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_sky.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_stl.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_surface.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_surfacesprites.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_world.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_WorldEffects.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_gamma.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_glimp.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\win32\win_qgl.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# End Group
# Begin Group "UI Source"

# PROP Default_Filter ""
# Begin Group "UI Headers"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\game\channels.h
# End Source File
# Begin Source File

SOURCE=.\ui\gameinfo.h
# End Source File
# Begin Source File

SOURCE=.\client\keycodes.h
# End Source File
# Begin Source File

SOURCE=.\ui\menudef.h
# End Source File
# Begin Source File

SOURCE=.\game\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\qcommon.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\qfiles.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\stv_version.h
# End Source File
# Begin Source File

SOURCE=.\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\tags.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_local.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_playerinfo.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_public.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_shared.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ui\ui_atoms.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ui\ui_connect.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ui\ui_debug.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ui\ui_main.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ui\ui_shared.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=.\ui\ui_syscalls.cpp
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# End Group
# Begin Group "JPEG"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\jpeg-6\jcapimin.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jccoefct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jccolor.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcdctmgr.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jchuff.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jchuff.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcinit.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmainct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmarker.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmaster.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcomapi.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jconfig.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcparam.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcphuff.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcprepct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcsample.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jctrans.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdapimin.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdapistd.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdatadst.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdatasrc.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdcoefct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdcolor.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdct.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jddctmgr.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdhuff.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdhuff.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdinput.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmainct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmarker.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmaster.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdpostct.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdsample.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdtrans.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jerror.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jerror.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jfdctflt.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jidctflt.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jinclude.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmemmgr.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmemnobs.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmemsys.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmorecfg.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jpegint.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jpeglib.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jutils.cpp"
# ADD CPP /Yu"..\server\exe_headers.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jversion.h"
# End Source File
# End Group
# Begin Group "encryption"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\encryption\buffer.cpp

!IF  "$(CFG)" == "starwars - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encryption\buffer.h
# End Source File
# Begin Source File

SOURCE=.\encryption\cpp_interface.cpp

!IF  "$(CFG)" == "starwars - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encryption\cpp_interface.h
# End Source File
# Begin Source File

SOURCE=.\encryption\encryption.h
# End Source File
# Begin Source File

SOURCE=.\encryption\sockets.cpp

!IF  "$(CFG)" == "starwars - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "starwars - Win32 FinalBuild"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "starwars - Win32 SHDebug"

# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\encryption\sockets.h
# End Source File
# End Group
# Begin Group "FF (Immersion)"

# PROP Default_Filter ""
# Begin Group "IFC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ff\IFC\FeelitAPI.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\IFC.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\IFCErrors.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmBaseTypes.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmBox.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmCompoundEffect.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmCondition.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmConstant.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmDamper.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmDevice.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmDevices.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmDXDevice.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmEffect.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmEffectSuite.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmEllipse.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmEnclosure.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmFriction.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmGrid.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmIFR.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmInertia.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmMouse.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmPeriodic.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmProjects.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmRamp.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmSpring.h
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\ImmTexture.h
# End Source File
# End Group
# Begin Group "FF Binaries"

# PROP Default_Filter "*.dll"
# Begin Source File

SOURCE=.\IFC22.dll
# End Source File
# Begin Source File

SOURCE=.\ff\IFC\IFC22.lib
# End Source File
# End Group
# Begin Source File

SOURCE=.\ff\cl_ff.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\cl_ff.h
# End Source File
# Begin Source File

SOURCE=.\ff\common_headers.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ChannelCompound.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ChannelSet.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ChannelSet.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ConfigParser.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ConfigParser.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ffset.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_ffset.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_HandleTable.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_HandleTable.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_local.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiCompound.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiCompound.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiEffect.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiEffect.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiSet.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_MultiSet.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_public.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_snd.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_snd.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_system.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_system.h
# End Source File
# Begin Source File

SOURCE=.\ff\ff_utils.cpp
# SUBTRACT CPP /YX
# End Source File
# Begin Source File

SOURCE=.\ff\ff_utils.h
# End Source File
# End Group
# End Target
# End Project
