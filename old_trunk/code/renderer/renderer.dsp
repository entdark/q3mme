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
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
RSC=rc.exe

!IF  "$(CFG)" == "renderer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_VM_NATIVE" /D "HAVE_VM_COMPILED" /FR /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
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
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_VM_NATIVE" /D "HAVE_VM_COMPILED" /YX /FD /GZ /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=xilink6.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "renderer - Win32 Release"
# Name "renderer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\tr_animation.c
# End Source File
# Begin Source File

SOURCE=.\tr_backend.c
# End Source File
# Begin Source File

SOURCE=.\tr_bsp.c
# End Source File
# Begin Source File

SOURCE=.\tr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\tr_curve.c
# End Source File
# Begin Source File

SOURCE=.\tr_flares.c
# End Source File
# Begin Source File

SOURCE=.\tr_font.c
# End Source File
# Begin Source File

SOURCE=.\tr_image.c
# End Source File
# Begin Source File

SOURCE=.\tr_init.c
# End Source File
# Begin Source File

SOURCE=.\tr_light.c
# End Source File
# Begin Source File

SOURCE=.\tr_main.c
# End Source File
# Begin Source File

SOURCE=.\tr_marks.c
# End Source File
# Begin Source File

SOURCE=.\tr_mesh.c
# End Source File
# Begin Source File

SOURCE=.\tr_model.c
# End Source File
# Begin Source File

SOURCE=.\tr_noise.c
# End Source File
# Begin Source File

SOURCE=.\tr_scene.c
# End Source File
# Begin Source File

SOURCE=.\tr_shade.c
# End Source File
# Begin Source File

SOURCE=.\tr_shade_calc.c
# End Source File
# Begin Source File

SOURCE=.\tr_shader.c
# End Source File
# Begin Source File

SOURCE=.\tr_shadows.c
# End Source File
# Begin Source File

SOURCE=.\tr_sky.c
# End Source File
# Begin Source File

SOURCE=.\tr_surface.c
# End Source File
# Begin Source File

SOURCE=.\tr_world.c
# End Source File
# Begin Source File

SOURCE=..\win32\win_gamma.c
# End Source File
# Begin Source File

SOURCE=..\win32\win_glimp.c
# End Source File
# Begin Source File

SOURCE=..\win32\win_qgl.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\qcommon\cm_public.h
# End Source File
# Begin Source File

SOURCE=..\win32\glw_win.h
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jchuff.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jconfig.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdct.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdhuff.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jerror.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jinclude.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jmemsys.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jmorecfg.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jpegint.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jpeglib.h"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jversion.h"
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

SOURCE=.\qgl_linked.h
# End Source File
# Begin Source File

SOURCE=..\win32\resource.h
# End Source File
# Begin Source File

SOURCE=..\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\tr_local.h
# End Source File
# Begin Source File

SOURCE=.\tr_public.h
# End Source File
# Begin Source File

SOURCE=..\cgame\tr_types.h
# End Source File
# Begin Source File

SOURCE=..\win32\win_local.h
# End Source File
# End Group
# Begin Group "JPEG"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\jpeg-6\jcapimin.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jccoefct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jccolor.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcdctmgr.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jchuff.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcinit.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcmainct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcmarker.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcmaster.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcomapi.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcparam.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcphuff.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcprepct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jcsample.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jctrans.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdapimin.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdapistd.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdatadst.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdatasrc.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdcoefct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdcolor.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jddctmgr.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdhuff.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdinput.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdmainct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdmarker.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdmaster.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdpostct.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdsample.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jdtrans.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jerror.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jfdctflt.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jidctflt.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jmemmgr.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jmemnobs.c"
# End Source File
# Begin Source File

SOURCE="..\jpeg-6\jutils.c"
# End Source File
# End Group
# End Target
# End Project
