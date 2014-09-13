# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=game - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "game - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/qagamex86.dll"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/qagamex86.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "game - Win32 Release"
# Name "game - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ai_chat.c
# End Source File
# Begin Source File

SOURCE=.\ai_cmd.c
# End Source File
# Begin Source File

SOURCE=.\ai_dmnet.c
# End Source File
# Begin Source File

SOURCE=.\ai_dmq3.c
# End Source File
# Begin Source File

SOURCE=.\ai_main.c
# End Source File
# Begin Source File

SOURCE=.\ai_team.c
# End Source File
# Begin Source File

SOURCE=.\ai_vcmd.c
# End Source File
# Begin Source File

SOURCE=.\bg_misc.c
# End Source File
# Begin Source File

SOURCE=.\bg_pmove.c
# End Source File
# Begin Source File

SOURCE=.\bg_slidemove.c
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

SOURCE=.\g_items.c
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

SOURCE=.\g_session.c
# End Source File
# Begin Source File

SOURCE=.\g_spawn.c
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

SOURCE=.\g_trigger.c
# End Source File
# Begin Source File

SOURCE=.\g_utils.c
# End Source File
# Begin Source File

SOURCE=.\g_weapon.c
# End Source File
# Begin Source File

SOURCE=.\q_math.c
# End Source File
# Begin Source File

SOURCE=.\q_shared.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ai_chat.h
# End Source File
# Begin Source File

SOURCE=.\ai_cmd.h
# End Source File
# Begin Source File

SOURCE=.\ai_dmnet.h
# End Source File
# Begin Source File

SOURCE=.\ai_dmq3.h
# End Source File
# Begin Source File

SOURCE=.\ai_main.h
# End Source File
# Begin Source File

SOURCE=.\ai_team.h
# End Source File
# Begin Source File

SOURCE=.\ai_vcmd.h
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

SOURCE=.\bg_local.h
# End Source File
# Begin Source File

SOURCE=.\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\botlib.h
# End Source File
# Begin Source File

SOURCE=.\chars.h
# End Source File
# Begin Source File

SOURCE=.\g_local.h
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

SOURCE=.\match.h
# End Source File
# Begin Source File

SOURCE=..\..\ui\menudef.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=.\syn.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\game.def
# End Source File
# End Target
# End Project
