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

DEMOS MENU

=======================================================================
*/


#include "ui_local.h"


#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_GO0				"menu/art/play_0"
#define ART_GO1				"menu/art/play_1"
#define ART_ARROWS			"menu/art/arrows_vert_0"
#define ART_ARROWTOP		"menu/art/arrows_vert_top"
#define ART_ARROWBOT		"menu/art/arrows_vert_bot"

#define MAX_ENTRIES			4096

#define ID_BACK				10
#define ID_GO				11
#define ID_LIST				12
#define ID_UP				13
#define ID_DOWN				14

#define ARROWS_X			(560)
#define ARROWS_Y			180
#define ARROWS_SIZE			64


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menutext_s		fstart, fname;
	menulist_s		list;

	menubitmap_s	arrows;
	menubitmap_s	up;
	menubitmap_s	down;
	menubitmap_s	back;
	menubitmap_s	go;

	int				numFiles;
	int				numDirs;
	char			fileNames[MAX_ENTRIES * 16];
	char			dirNames[2048];

	const char		*listEntries[MAX_ENTRIES];
	char			path[MAX_QPATH];
} demos_t;

static demos_t	s_demos;

static void UI_DemosMakeList( const char *path ) {
	int		i, filterNumber, entryCount;
	char	*nameRead, *nameWrite;
	int		sizeLeft;
	char	searchPath[MAX_QPATH];

	Com_sprintf( searchPath, sizeof(searchPath), "demos/%s", path );

	s_demos.numDirs = trap_FS_GetFileList( searchPath, "/", s_demos.fileNames, sizeof( s_demos.fileNames ));
	nameRead = s_demos.fileNames;
	nameWrite = s_demos.dirNames;
	sizeLeft = sizeof( s_demos.dirNames );
	filterNumber = 0;
	for (i = 0;i < s_demos.numDirs;i++) {
		int len = strlen( nameRead ) + 1;
		if (nameRead[0] && Q_stricmp( nameRead, ".") && Q_stricmp( nameRead, "..")) {
			if ( len + 1> sizeLeft )
				break;
			*nameWrite = '/';
			memcpy( nameWrite + 1, nameRead, len );
			nameWrite += len + 1;
			filterNumber++;
		}
		nameRead += len;
	}
	s_demos.numDirs = filterNumber;
	
	/* Get the fileName */
	s_demos.numFiles = trap_FS_GetFileList( searchPath, ".dm_6?", s_demos.fileNames, sizeof( s_demos.fileNames ));
	nameRead = s_demos.fileNames;
	nameWrite = s_demos.fileNames;
	filterNumber = 0;
	for (i = 0;i < s_demos.numFiles;i++) {
		char *ext;int len;
		len = strlen( nameRead );
		ext = strrchr( nameRead, '.' );
		if (ext && (!Q_stricmp( ext, ".dm_66" ) || !Q_stricmp( ext, ".dm_67" ) || !Q_stricmp( ext, ".dm_68" ))) {
			*ext = 0;
			strcpy( nameWrite, nameRead );
			nameWrite += strlen( nameWrite ) + 1;
			filterNumber++;
		}
		nameRead += len + 1;
	}
	s_demos.numFiles = filterNumber;


	/* Fill out the screen list */
	entryCount = 0;
	/* Add a .. when in a sub path */
	if (path[0]) {
		s_demos.listEntries[ entryCount ++] = "/..";
	}
	/* Add the directory names */
	i = MAX_ENTRIES - entryCount;
	i = i > s_demos.numDirs ? s_demos.numDirs : i ;
	nameRead = s_demos.dirNames;
	for ( ; i> 0; i--) {
		s_demos.listEntries[ entryCount ++] = nameRead;
		nameRead += strlen ( nameRead ) + 1;
	}
	/* Add the demo names */
	i = MAX_ENTRIES - entryCount;
	i = i > s_demos.numFiles ? s_demos.numFiles : i ;
	nameRead = s_demos.fileNames;
	for ( ; i> 0; i--) {
		s_demos.listEntries[ entryCount ++] = nameRead;
		nameRead += strlen ( nameRead ) + 1;
	}
	/* Reset the list state */
	s_demos.list.numitems = entryCount;
	s_demos.list.curvalue = 0;
	s_demos.list.oldvalue = 0;
	s_demos.list.top = 0;
}


/*
===============
Demos_MenuEvent
===============
*/
static void Demos_MenuEvent( void *ptr, int event ) {
	const char *entry;
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
	case ID_LIST:
		entry = s_demos.list.itemnames[s_demos.list.curvalue];
		if (!entry || !entry[0])
			break;
		if (entry[0] == '/') {
			if (!Q_stricmp( entry, "/..")) {
				char *lastSlash;
				lastSlash = strrchr( s_demos.path, '/' );
				if (lastSlash) {
					*lastSlash = 0;
				} else {
					s_demos.path[0] = 0;
				}
			} else {
				Q_strcat( s_demos.path, sizeof( s_demos.path ), entry );
			}
			UI_DemosMakeList( s_demos.path );
		} else {
			UI_ForceMenuOff ();
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo \"%s/%s\"\n", s_demos.path, entry) );
		}
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_UP:
		ScrollList_Key( &s_demos.list, K_PGUP );
		break;

	case ID_DOWN:
		ScrollList_Key( &s_demos.list, K_PGDN );
		break;
	}
}


/*
=================
UI_DemosMenu_Key
=================
*/
static sfxHandle_t UI_DemosMenu_Key( int key ) {
	menucommon_s	*item;

	item = Menu_ItemAtCursor( &s_demos.menu );

	return Menu_DefaultKey( &s_demos.menu, key );
}



static void Demos_MenuDraw( void ) {
	UI_BackGroundRender( 0, 0, 640, 480, 0.9f );
	Menu_Draw( &s_demos.menu );

}

/*
===============
Demos_MenuInit
===============
*/
static void Demos_MenuInit( void ) {

	memset( &s_demos, 0 ,sizeof(demos_t) );
	s_demos.menu.key = UI_DemosMenu_Key;

	Demos_Cache();

	s_demos.menu.fullscreen = qtrue;
	s_demos.menu.wrapAround = qtrue;
	s_demos.menu.draw = Demos_MenuDraw;

	s_demos.banner.generic.type		= MTYPE_BTEXT;
	s_demos.banner.generic.x		= 320;
	s_demos.banner.generic.y		= 16;
	s_demos.banner.string			= "DEMOS";
	s_demos.banner.color			= color_white;
	s_demos.banner.style			= UI_CENTER;

	s_demos.fstart.generic.type		= MTYPE_TEXT;
	s_demos.fstart.generic.x		= 85;
	s_demos.fstart.generic.y		= 55;
	s_demos.fstart.string			= "Folder:";
	s_demos.fstart.color			= color_yellow;
	s_demos.fstart.style			= UI_RIGHT | UI_SMALLFONT;

	s_demos.fname.generic.type		= MTYPE_TEXT;
	s_demos.fname.generic.x			= 85;
	s_demos.fname.generic.y			= 55;
	s_demos.fname.string			= s_demos.path;
	s_demos.fname.color				= color_red;
	s_demos.fname.style				= UI_LEFT | UI_SMALLFONT;

	s_demos.arrows.generic.type		= MTYPE_BITMAP;
	s_demos.arrows.generic.name		= ART_ARROWS;
	s_demos.arrows.generic.flags	= QMF_INACTIVE;
	s_demos.arrows.generic.x		= ARROWS_X;
	s_demos.arrows.generic.y		= ARROWS_Y;
	s_demos.arrows.width			= ARROWS_SIZE;
	s_demos.arrows.height			= ARROWS_SIZE*2;

	s_demos.up.generic.type			= MTYPE_BITMAP;
	s_demos.up.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_demos.up.generic.x			= ARROWS_X;
	s_demos.up.generic.y			= ARROWS_Y;
	s_demos.up.generic.id			= ID_UP;
	s_demos.up.generic.callback		= Demos_MenuEvent;
	s_demos.up.width				= ARROWS_SIZE;
	s_demos.up.height				= ARROWS_SIZE;
	s_demos.up.focuspic				= ART_ARROWTOP;

	s_demos.down.generic.type		= MTYPE_BITMAP;
	s_demos.down.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_demos.down.generic.x			= ARROWS_X;
	s_demos.down.generic.y			= ARROWS_Y + ARROWS_SIZE;
	s_demos.down.generic.id			= ID_DOWN;
	s_demos.down.generic.callback	= Demos_MenuEvent;
	s_demos.down.width				= ARROWS_SIZE;
	s_demos.down.height				= ARROWS_SIZE;
	s_demos.down.focuspic			= ART_ARROWBOT;

	s_demos.back.generic.type		= MTYPE_BITMAP;
	s_demos.back.generic.name		= ART_BACK0;
	s_demos.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.back.generic.id			= ID_BACK;
	s_demos.back.generic.callback	= Demos_MenuEvent;
	s_demos.back.generic.x			= 0;
	s_demos.back.generic.y			= 480-64;
	s_demos.back.width				= 128;
	s_demos.back.height				= 64;
	s_demos.back.focuspic			= ART_BACK1;

	s_demos.go.generic.type			= MTYPE_BITMAP;
	s_demos.go.generic.name			= ART_GO0;
	s_demos.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_demos.go.generic.id			= ID_GO;
	s_demos.go.generic.callback		= Demos_MenuEvent;
	s_demos.go.generic.x			= 640;
	s_demos.go.generic.y			= 480-64;
	s_demos.go.width				= 128;
	s_demos.go.height				= 64;
	s_demos.go.focuspic				= ART_GO1;

	s_demos.list.generic.type		= MTYPE_SCROLLLIST;
	s_demos.list.generic.flags		= QMF_PULSEIFFOCUS|QMF_DOUBLECLICK;
	s_demos.list.generic.callback	= Demos_MenuEvent;
	s_demos.list.generic.id			= ID_LIST;
	s_demos.list.generic.x			= 20;
	s_demos.list.generic.y			= 80;
	s_demos.list.generic.callback	= Demos_MenuEvent;
	s_demos.list.width				= 64;
	s_demos.list.height				= 20;
	s_demos.list.numitems			= 0;
	s_demos.list.itemnames			= (const char **)s_demos.listEntries;
	s_demos.list.columns			= 1;

	s_demos.path[0] = 0;
	UI_DemosMakeList( s_demos.path );

	Menu_AddItem( &s_demos.menu, &s_demos.banner );
	Menu_AddItem( &s_demos.menu, &s_demos.fstart );
	Menu_AddItem( &s_demos.menu, &s_demos.fname );
	Menu_AddItem( &s_demos.menu, &s_demos.list );
	Menu_AddItem( &s_demos.menu, &s_demos.arrows );
	Menu_AddItem( &s_demos.menu, &s_demos.up );
	Menu_AddItem( &s_demos.menu, &s_demos.down );
	Menu_AddItem( &s_demos.menu, &s_demos.back );
	Menu_AddItem( &s_demos.menu, &s_demos.go );
}

/*
=================
Demos_Cache
=================
*/
void Demos_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_GO0 );
	trap_R_RegisterShaderNoMip( ART_GO1 );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWTOP );
	trap_R_RegisterShaderNoMip( ART_ARROWBOT );
}

/*
===============
UI_DemosMenu
===============
*/
void UI_DemosMenu( void ) {
	Demos_MenuInit();
	UI_PushMenu( &s_demos.menu );
}
