# Microsoft Developer Studio Project File - Name="quake3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=quake3 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "quake3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "quake3.mak" CFG="quake3 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "quake3 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "quake3 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "quake3 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "BOTLIB" /D "HAVE_VM_COMPILED" /D "HAVE_VM_NATIVE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib ws2_32.lib winmm.lib /nologo /subsystem:windows /pdb:none /machine:I386 /out:"Release/quake3mme.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "quake3 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "BOTLIB" /D "HAVE_VM_COMPILED" /D "HAVE_VM_NATIVE" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib ws2_32.lib winmm.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/quake3mme.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "quake3 - Win32 Release"
# Name "quake3 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\client\cl_cgame.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_cin.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_console.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_input.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_keys.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_main.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_mme.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_net_chan.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_parse.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_scrn.c
# End Source File
# Begin Source File

SOURCE=.\client\cl_ui.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_load.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_patch.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_polylib.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_test.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cm_trace.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cmd.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\common.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\cvar.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\files.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\huffman.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\md4.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\msg.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\net_chan.c
# End Source File
# Begin Source File

SOURCE=.\game\q_math.c
# End Source File
# Begin Source File

SOURCE=.\game\q_shared.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_adpcm.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_dma.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mem.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mix.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_mme.c
# End Source File
# Begin Source File

SOURCE=.\client\snd_wavelet.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_bot.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_ccmds.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_client.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_game.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_init.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_main.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_net_chan.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_snapshot.c
# End Source File
# Begin Source File

SOURCE=.\server\sv_world.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\unzip.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\vm.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\vm_interpreted.c
# End Source File
# Begin Source File

SOURCE=.\qcommon\vm_x86.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_input.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_main.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_net.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_shared.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_snd.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_syscon.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_wndproc.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\botlib\aasfile.h
# End Source File
# Begin Source File

SOURCE=.\game\be_aas.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_bsp.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_cluster.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_debug.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_def.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_entity.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_file.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_funcs.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_main.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_move.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_optimize.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_reach.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_route.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_routealt.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_sample.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_char.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_chat.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_gen.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_goal.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_move.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ai_weap.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_weight.h
# End Source File
# Begin Source File

SOURCE=.\game\be_ea.h
# End Source File
# Begin Source File

SOURCE=.\botlib\be_interface.h
# End Source File
# Begin Source File

SOURCE=.\game\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\game\botlib.h
# End Source File
# Begin Source File

SOURCE=.\cgame\cg_public.h
# End Source File
# Begin Source File

SOURCE=.\client\client.h
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

SOURCE=.\game\g_public.h
# End Source File
# Begin Source File

SOURCE=.\win32\glw_win.h
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jchuff.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jconfig.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdct.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdhuff.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jerror.h"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jinclude.h"
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

SOURCE=".\jpeg-6\jversion.h"
# End Source File
# Begin Source File

SOURCE=.\ui\keycodes.h
# End Source File
# Begin Source File

SOURCE=.\client\keys.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_crc.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_libvar.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_log.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_memory.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_precomp.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_script.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_struct.h
# End Source File
# Begin Source File

SOURCE=.\botlib\l_utils.h
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

SOURCE=.\renderer\qgl.h
# End Source File
# Begin Source File

SOURCE=.\win32\resource.h
# End Source File
# Begin Source File

SOURCE=.\server\server.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_local.h
# End Source File
# Begin Source File

SOURCE=.\client\snd_public.h
# End Source File
# Begin Source File

SOURCE=.\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_local.h
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_public.h
# End Source File
# Begin Source File

SOURCE=.\cgame\tr_types.h
# End Source File
# Begin Source File

SOURCE=.\ui\ui_public.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\unzip.h
# End Source File
# Begin Source File

SOURCE=.\qcommon\vm_local.h
# End Source File
# Begin Source File

SOURCE=.\win32\win_local.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;rc"
# Begin Source File

SOURCE=.\win32\qe3.ico
# End Source File
# Begin Source File

SOURCE=.\win32\winquake.rc
# End Source File
# End Group
# Begin Group "renderer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\renderer\tr_animation.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_backend.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_bsp.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_cmds.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_curve.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_flares.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_font.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_image.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_init.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_light.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_main.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_marks.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_mesh.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_model.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_noise.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_scene.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shade.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shade_calc.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shader.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_shadows.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_sky.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_surface.c
# End Source File
# Begin Source File

SOURCE=.\renderer\tr_world.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_gamma.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_glimp.c
# End Source File
# Begin Source File

SOURCE=.\win32\win_qgl.c
# End Source File
# End Group
# Begin Group "JPEG"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\jpeg-6\jcapimin.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jccoefct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jccolor.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcdctmgr.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jchuff.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcinit.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmainct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmarker.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcmaster.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcomapi.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcparam.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcphuff.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcprepct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jcsample.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jctrans.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdapimin.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdapistd.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdatadst.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdatasrc.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdcoefct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdcolor.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jddctmgr.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdhuff.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdinput.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmainct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmarker.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdmaster.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdpostct.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdsample.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jdtrans.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jerror.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jfdctflt.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jidctflt.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmemmgr.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jmemnobs.c"
# End Source File
# Begin Source File

SOURCE=".\jpeg-6\jutils.c"
# End Source File
# End Group
# Begin Group "botlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\botlib\be_aas_bspq3.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_cluster.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_debug.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_entity.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_file.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_main.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_move.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_optimize.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_reach.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_route.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_routealt.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_aas_sample.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_char.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_chat.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_gen.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_goal.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_move.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_weap.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ai_weight.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_ea.c
# End Source File
# Begin Source File

SOURCE=.\botlib\be_interface.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_crc.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_libvar.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_log.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_memory.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_precomp.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_script.c
# End Source File
# Begin Source File

SOURCE=.\botlib\l_struct.c
# End Source File
# End Group
# End Target
# End Project
