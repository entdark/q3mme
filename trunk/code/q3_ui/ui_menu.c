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

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_DEMOS				10
#define ID_SETUP				11
#define ID_CREDITS				12
#define ID_EXIT					13

#define MAIN_MENU_VERTICAL_SPACING		34



typedef struct {
	menuframework_s	menu;

	menutext_s		setup;
	menutext_s		demos;
	menutext_s		credits;
	menutext_s		exit;
} mainmenu_t;

static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SETUP:
		UI_GraphicsOptionsMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CREDITS:
//		trap_Cmd_ExecuteText( EXEC_APPEND, "screenshotjpeg;" );
		UI_CreditMenu( qfalse );
		break;

	case ID_EXIT:
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
//		UI_PopMenu();
//		UI_CreditMenu( qtrue );
		break;
	}
}


/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {

	static vec4_t color  = { 0.7f, 0.0f, 0.0f, 1.0f };
	UI_BackGroundRender( 0, 0, 640, 480, 0 );
	
	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	} else {
		// standard menu drawing
		Menu_Draw( &s_main.menu );
	}

	UI_DrawString( 320, 450, "Quake III Movie Makers Edition " Q3_VERSION_NUMBER, UI_CENTER|UI_SMALLFONT, color );
}

/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	UI_BackGroundCache();
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );
	
	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;

	y = 330;
#define PLACE_X 530

	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= PLACE_X;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_red;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= PLACE_X;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_red;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.credits.generic.type				= MTYPE_PTEXT;
	s_main.credits.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.credits.generic.x				= PLACE_X;
	s_main.credits.generic.y				= y;
	s_main.credits.generic.id				= ID_CREDITS;
	s_main.credits.generic.callback			= Main_MenuEvent; 
	s_main.credits.string					= "CREDITS";
	s_main.credits.color					= color_red;
	s_main.credits.style					= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= PLACE_X;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_red;
	s_main.exit.style						= style;

	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.credits );
	Menu_AddItem( &s_main.menu,	&s_main.exit );             

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}
