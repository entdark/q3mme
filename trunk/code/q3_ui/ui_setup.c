/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
/*
=======================================================================

SETUP MENU

=======================================================================
*/


#include "ui_local.h"


#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"	
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_VIDEO				11
#define ID_SOUND				12
#define ID_DEFAULTS				14
#define ID_BACK					15

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		setupvideo;
	menutext_s		setupsound;
	menutext_s		defaults;
	menubitmap_s	back;
} setupMenuInfo_t;

static setupMenuInfo_t	setupMenuInfo;


/*
=================
Setup_ResetDefaults_Action
=================
*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
Setup_ResetDefaults_Draw
=================
*/
static void Setup_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset *ALL*", UI_CENTER|UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "options to their default values.", UI_CENTER|UI_SMALLFONT, color_yellow );
}


/*
===============
UI_SetupMenu_Event
===============
*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_VIDEO:
		UI_GraphicsOptionsMenu();
		break;

	case ID_SOUND:
		UI_SoundOptionsMenu();
		break;

	case ID_DEFAULTS:
		UI_ConfirmMenu( "SET TO DEFAULTS?", Setup_ResetDefaults_Draw, Setup_ResetDefaults_Action );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_SetupMenu_Init
===============
*/
static void UI_SetupMenu_Init( void ) {
	int				y;

	UI_SetupMenu_Cache();

	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;

	setupMenuInfo.banner.generic.type				= MTYPE_BTEXT;
	setupMenuInfo.banner.generic.x					= 320;
	setupMenuInfo.banner.generic.y					= 16;
	setupMenuInfo.banner.string						= "SETUP";
	setupMenuInfo.banner.color						= color_white;
	setupMenuInfo.banner.style						= UI_CENTER;

	setupMenuInfo.framel.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framel.generic.name				= ART_FRAMEL;
	setupMenuInfo.framel.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framel.generic.x					= 0;  
	setupMenuInfo.framel.generic.y					= 78;
	setupMenuInfo.framel.width  					= 256;
	setupMenuInfo.framel.height  					= 329;

	setupMenuInfo.framer.generic.type				= MTYPE_BITMAP;
	setupMenuInfo.framer.generic.name				= ART_FRAMER;
	setupMenuInfo.framer.generic.flags				= QMF_INACTIVE;
	setupMenuInfo.framer.generic.x					= 376;
	setupMenuInfo.framer.generic.y					= 76;
	setupMenuInfo.framer.width  					= 256;
	setupMenuInfo.framer.height  					= 334;

	y = 134;
	setupMenuInfo.setupvideo.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupvideo.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupvideo.generic.x			= 320;
	setupMenuInfo.setupvideo.generic.y			= y;
	setupMenuInfo.setupvideo.generic.id			= ID_VIDEO;
	setupMenuInfo.setupvideo.generic.callback	= UI_SetupMenu_Event; 
	setupMenuInfo.setupvideo.string				= "VIDEO";
	setupMenuInfo.setupvideo.color				= color_red;
	setupMenuInfo.setupvideo.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.setupsound.generic.type		= MTYPE_PTEXT;
	setupMenuInfo.setupsound.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsound.generic.x			= 320;
	setupMenuInfo.setupsound.generic.y			= y;
	setupMenuInfo.setupsound.generic.id			= ID_SOUND;
	setupMenuInfo.setupsound.generic.callback	= UI_SetupMenu_Event; 
	setupMenuInfo.setupsound.string				= "SOUND";
	setupMenuInfo.setupsound.color				= color_red;
	setupMenuInfo.setupsound.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.defaults.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.defaults.generic.x				= 320;
	setupMenuInfo.defaults.generic.y				= y;
	setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
	setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
	setupMenuInfo.defaults.string					= "DEFAULTS";
	setupMenuInfo.defaults.color					= color_red;
	setupMenuInfo.defaults.style					= UI_CENTER;

	setupMenuInfo.back.generic.type					= MTYPE_BITMAP;
	setupMenuInfo.back.generic.name					= ART_BACK0;
	setupMenuInfo.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.id					= ID_BACK;
	setupMenuInfo.back.generic.callback				= UI_SetupMenu_Event;
	setupMenuInfo.back.generic.x					= 0;
	setupMenuInfo.back.generic.y					= 480-64;
	setupMenuInfo.back.width						= 128;
	setupMenuInfo.back.height						= 64;
	setupMenuInfo.back.focuspic						= ART_BACK1;

	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.banner );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framel );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.framer );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupvideo );
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsound );
	if( !trap_Cvar_VariableValue( "cl_paused" ) ) {
		Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
	}
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );
}


/*
=================
UI_SetupMenu_Cache
=================
*/
void UI_SetupMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
===============
UI_SetupMenu
===============
*/
void UI_SetupMenu( void ) {
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}
