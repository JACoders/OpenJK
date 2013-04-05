# Microsoft Developer Studio Project File - Name="renderer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=renderer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "renderer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "renderer.mak" CFG="renderer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "renderer - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "renderer - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "renderer - Win32 FinalBuild" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/Code/renderer", NCMAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "renderer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\Release"
# PROP Intermediate_Dir "..\Release\renderer"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "__USEA3D" /D "__A3D_GEOM" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\Debug"
# PROP Intermediate_Dir "..\Debug\renderer"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "..\ICARUS" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "__USEA3D" /D "__A3D_GEOM" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "renderer___Win32_FinalBuild"
# PROP BASE Intermediate_Dir "renderer___Win32_FinalBuild"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\FinalBuild"
# PROP Intermediate_Dir "..\FinalBuild\renderer"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /Ob2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "__USEA3D" /D "__A3D_GEOM" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W4 /GX /Zi /O2 /Ob2 /D "_MBCS" /D "_LIB" /D "__USEA3D" /D "__A3D_GEOM" /D "WIN32" /D "NDEBUG" /D "FINAL_BUILD" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "renderer - Win32 Release"
# Name "renderer - Win32 Debug"
# Name "renderer - Win32 FinalBuild"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\MatComp.c
# End Source File
# Begin Source File

SOURCE=.\tr_animation.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_backend.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_bsp.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_cmds.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_curve.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_draw.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_ghoul2.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_image.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_init.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_jpeg_interface.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_light.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_main.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_marks.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_mesh.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_model.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_noise.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_scene.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_shade.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_shade_calc.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_shader.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_shadows.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_sky.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_stl.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_surface.cpp
# End Source File
# Begin Source File

SOURCE=.\tr_world.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\win_gamma.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\win_glimp.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\win_qgl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\amd3d.h
# End Source File
# Begin Source File

SOURCE=..\game\channels.h
# End Source File
# Begin Source File

SOURCE=..\qcommon\cm_public.h
# End Source File
# Begin Source File

SOURCE=..\game\ghoul2_shared.h
# End Source File
# Begin Source File

SOURCE=.\glext.h
# End Source File
# Begin Source File

SOURCE=..\win32\glw_win.h
# End Source File
# Begin Source File

SOURCE=.\MatComp.h
# End Source File
# Begin Source File

SOURCE=.\mdx_format.h
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

SOURCE=.\qgl.h
# End Source File
# Begin Source File

SOURCE=..\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\tr_jpeg_interface.h
# End Source File
# Begin Source File

SOURCE=.\tr_local.h
# End Source File
# Begin Source File

SOURCE=.\tr_public.h
# End Source File
# Begin Source File

SOURCE=.\tr_stl.h
# End Source File
# Begin Source File

SOURCE=.\tr_types.h
# End Source File
# Begin Source File

SOURCE=..\win32\win_local.h
# End Source File
# End Group
# Begin Group "JPEG Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\jpeg6\jcomapi.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdapimin.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdapistd.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdatasrc.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdcoefct.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdcolor.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jddctmgr.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdhuff.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdinput.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdmainct.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdmarker.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdmaster.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdpostct.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdsample.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdtrans.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jerror.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jidctflt.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jmemmgr.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jmemnobs.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\jpeg6\jutils.c

!IF  "$(CFG)" == "renderer - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "renderer - Win32 Debug"

# ADD CPP /W3
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "renderer - Win32 FinalBuild"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# End Group
# Begin Group "JPEG Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\jpeg6\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdct.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jerror.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\jpeg6\jversion.h
# End Source File
# End Group
# End Target
# End Project
