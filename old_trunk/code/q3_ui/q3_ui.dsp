# Microsoft Developer Studio Project File - Name="q3_ui" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=q3_ui - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "q3_ui.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "q3_ui.mak" CFG="q3_ui - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "q3_ui - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "q3_ui - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "q3_ui - Win32 Release"

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
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "Q3_UI_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/uix86.dll"

!ELSEIF  "$(CFG)" == "q3_ui - Win32 Debug"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "Q3_UI_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /out:"Debug/uix86.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "q3_ui - Win32 Release"
# Name "q3_ui - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\game\bg_misc.c
# End Source File
# Begin Source File

SOURCE=..\game\q_math.c
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.c
# End Source File
# Begin Source File

SOURCE=.\ui_addbots.c
# End Source File
# Begin Source File

SOURCE=.\ui_atoms.c
# End Source File
# Begin Source File

SOURCE=.\ui_cdkey.c
# End Source File
# Begin Source File

SOURCE=.\ui_cinematics.c
# End Source File
# Begin Source File

SOURCE=.\ui_confirm.c
# End Source File
# Begin Source File

SOURCE=.\ui_connect.c
# End Source File
# Begin Source File

SOURCE=.\ui_controls2.c
# End Source File
# Begin Source File

SOURCE=.\ui_credits.c
# End Source File
# Begin Source File

SOURCE=.\ui_demo2.c
# End Source File
# Begin Source File

SOURCE=.\ui_display.c
# End Source File
# Begin Source File

SOURCE=.\ui_gameinfo.c
# End Source File
# Begin Source File

SOURCE=.\ui_ingame.c
# End Source File
# Begin Source File

SOURCE=.\ui_main.c
# End Source File
# Begin Source File

SOURCE=.\ui_menu.c
# End Source File
# Begin Source File

SOURCE=.\ui_mfield.c
# End Source File
# Begin Source File

SOURCE=.\ui_mods.c
# End Source File
# Begin Source File

SOURCE=.\ui_network.c
# End Source File
# Begin Source File

SOURCE=.\ui_options.c
# End Source File
# Begin Source File

SOURCE=.\ui_playermodel.c
# End Source File
# Begin Source File

SOURCE=.\ui_players.c
# End Source File
# Begin Source File

SOURCE=.\ui_playersettings.c
# End Source File
# Begin Source File

SOURCE=.\ui_preferences.c
# End Source File
# Begin Source File

SOURCE=.\ui_qmenu.c
# End Source File
# Begin Source File

SOURCE=.\ui_removebots.c
# End Source File
# Begin Source File

SOURCE=.\ui_serverinfo.c
# End Source File
# Begin Source File

SOURCE=.\ui_servers2.c
# End Source File
# Begin Source File

SOURCE=.\ui_setup.c
# End Source File
# Begin Source File

SOURCE=.\ui_sound.c
# End Source File
# Begin Source File

SOURCE=.\ui_sparena.c
# End Source File
# Begin Source File

SOURCE=.\ui_specifyserver.c
# End Source File
# Begin Source File

SOURCE=.\ui_splevel.c
# End Source File
# Begin Source File

SOURCE=.\ui_sppostgame.c
# End Source File
# Begin Source File

SOURCE=.\ui_spreset.c
# End Source File
# Begin Source File

SOURCE=.\ui_spskill.c
# End Source File
# Begin Source File

SOURCE=.\ui_startserver.c
# End Source File
# Begin Source File

SOURCE=..\ui\ui_syscalls.c
# End Source File
# Begin Source File

SOURCE=.\ui_team.c
# End Source File
# Begin Source File

SOURCE=.\ui_teamorders.c
# End Source File
# Begin Source File

SOURCE=.\ui_video.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\game\bg_public.h
# End Source File
# Begin Source File

SOURCE=.\keycodes.h
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.h
# End Source File
# Begin Source File

SOURCE=..\game\surfaceflags.h
# End Source File
# Begin Source File

SOURCE=..\cgame\tr_types.h
# End Source File
# Begin Source File

SOURCE=.\ui_local.h
# End Source File
# Begin Source File

SOURCE=..\ui\ui_public.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ui.def
# End Source File
# End Target
# End Project
