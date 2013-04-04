# Microsoft Developer Studio Project File - Name="ModView" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ModView - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ModView.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ModView.mak" CFG="ModView - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ModView - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ModView - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Tools/ModView", NYDCAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ModView - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /Zi /O2 /D "NDEBUG" /D "_MODVIEW" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 opengl32.lib glu32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "_MODVIEW" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib glu32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "ModView - Win32 Release"
# Name "ModView - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\anims.cpp
# End Source File
# Begin Source File

SOURCE=.\clipboard.cpp
# End Source File
# Begin Source File

SOURCE=.\CommArea.cpp
# End Source File
# Begin Source File

SOURCE=.\drag.cpp
# End Source File
# Begin Source File

SOURCE=.\generic_stuff.cpp
# End Source File
# Begin Source File

SOURCE=.\GetString.cpp
# End Source File
# Begin Source File

SOURCE=.\gl_bits.cpp
# End Source File
# Begin Source File

SOURCE=.\glm_code.cpp
# End Source File
# Begin Source File

SOURCE=.\image.cpp
# End Source File
# Begin Source File

SOURCE=.\jpeg_interface.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\matcomp.cpp
# End Source File
# Begin Source File

SOURCE=.\mc_compress2.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\model.cpp
# End Source File
# Begin Source File

SOURCE=.\ModView.cpp
# End Source File
# Begin Source File

SOURCE=.\ModView.rc
# End Source File
# Begin Source File

SOURCE=.\ModViewDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ModViewTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\ModViewView.cpp
# End Source File
# Begin Source File

SOURCE=.\oldskins.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\R_GLM.cpp
# End Source File
# Begin Source File

SOURCE=.\R_Image.cpp
# End Source File
# Begin Source File

SOURCE=.\R_MD3.cpp
# End Source File
# Begin Source File

SOURCE=.\R_MDR.cpp
# End Source File
# Begin Source File

SOURCE=.\R_Model.cpp
# End Source File
# Begin Source File

SOURCE=.\R_Surface.cpp
# End Source File
# Begin Source File

SOURCE=.\script.cpp
# End Source File
# Begin Source File

SOURCE=.\sequence.cpp
# End Source File
# Begin Source File

SOURCE=.\shader.cpp
# End Source File
# Begin Source File

SOURCE=.\skins.cpp
# End Source File
# Begin Source File

SOURCE=.\SOF2NPCViewer.cpp
# End Source File
# Begin Source File

SOURCE=.\Splash.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TEXT.CPP
# End Source File
# Begin Source File

SOURCE=.\Textures.cpp
# End Source File
# Begin Source File

SOURCE=.\wintalk.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\anims.h
# End Source File
# Begin Source File

SOURCE=.\clipboard.h
# End Source File
# Begin Source File

SOURCE=.\CommArea.h
# End Source File
# Begin Source File

SOURCE=.\drag.h
# End Source File
# Begin Source File

SOURCE=.\generic_stuff.h
# End Source File
# Begin Source File

SOURCE=.\GetString.h
# End Source File
# Begin Source File

SOURCE=.\gl_bits.h
# End Source File
# Begin Source File

SOURCE=.\glm_code.h
# End Source File
# Begin Source File

SOURCE=.\image.h
# End Source File
# Begin Source File

SOURCE=.\includes.h
# End Source File
# Begin Source File

SOURCE=.\jpeg_interface.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\matcomp.h
# End Source File
# Begin Source File

SOURCE=.\mc_compress2.h
# End Source File
# Begin Source File

SOURCE=.\md3_format.h
# End Source File
# Begin Source File

SOURCE=.\mdr_format.h
# End Source File
# Begin Source File

SOURCE=.\mdx_format.h
# End Source File
# Begin Source File

SOURCE=.\model.h
# End Source File
# Begin Source File

SOURCE=.\ModView.h
# End Source File
# Begin Source File

SOURCE=.\ModViewDoc.h
# End Source File
# Begin Source File

SOURCE=.\ModViewTreeView.h
# End Source File
# Begin Source File

SOURCE=.\ModViewView.h
# End Source File
# Begin Source File

SOURCE=.\oldskins.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\R_Common.h
# End Source File
# Begin Source File

SOURCE=.\R_GLM.h
# End Source File
# Begin Source File

SOURCE=.\R_Image.h
# End Source File
# Begin Source File

SOURCE=.\R_MD3.h
# End Source File
# Begin Source File

SOURCE=.\R_MDR.h
# End Source File
# Begin Source File

SOURCE=.\R_Model.h
# End Source File
# Begin Source File

SOURCE=.\R_Surface.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\script.h
# End Source File
# Begin Source File

SOURCE=.\sequence.h
# End Source File
# Begin Source File

SOURCE=.\shader.h
# End Source File
# Begin Source File

SOURCE=.\skins.h
# End Source File
# Begin Source File

SOURCE=.\SOF2NPCViewer.h
# End Source File
# Begin Source File

SOURCE=.\special_defines.h
# End Source File
# Begin Source File

SOURCE=.\Splash.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\stl.h
# End Source File
# Begin Source File

SOURCE=.\TEXT.H
# End Source File
# Begin Source File

SOURCE=.\textures.h
# End Source File
# Begin Source File

SOURCE=.\todo.h
# End Source File
# Begin Source File

SOURCE=.\wintalk.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ModView.ico
# End Source File
# Begin Source File

SOURCE=.\res\ModView.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ModViewDoc.ico
# End Source File
# Begin Source File

SOURCE=.\Splsh16.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Group "JPEG Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jpeg6\jcomapi.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdapimin.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdapistd.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdatasrc.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdcoefct.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdcolor.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jddctmgr.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdhuff.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdinput.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdmainct.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdmarker.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdmaster.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdpostct.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdsample.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdtrans.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jerror.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jidctflt.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jmemmgr.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jmemnobs.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpeg6\jutils.c

!IF  "$(CFG)" == "ModView - Win32 Release"

# ADD CPP /FR
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "ModView - Win32 Debug"

# ADD CPP /Fr
# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "JPEG Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jpeg6\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jpeg6\jversion.h
# End Source File
# End Group
# Begin Group "PNG"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\png\png.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\png\png.h
# End Source File
# End Group
# Begin Group "ZLIB"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\zlib\adler32.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\crc32.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infblock.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infcodes.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=.\zlib\inflate.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\infutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.c
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\zlib\zutil.h
# End Source File
# End Group
# Begin Group "Foreign Muck Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\files.cpp
# End Source File
# Begin Source File

SOURCE=.\GenericParser2.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Foreign Muck Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\common_headers.h
# End Source File
# Begin Source File

SOURCE=.\disablewarnings.h
# End Source File
# Begin Source File

SOURCE=.\files.h
# End Source File
# Begin Source File

SOURCE=.\GenericParser2.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
# Section ModView : {72ADFD78-2C39-11D0-9903-00A0C91BC942}
# 	1:10:IDB_SPLASH:102
# 	2:21:SplashScreenInsertKey:4.0
# End Section
