/*
==========================================================================
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

CREDITS

=======================================================================
*/


#include "ui_local.h"

typedef struct {
	menuframework_s	menu;
	qboolean quit;
	qhandle_t fontShader;
	int	 showStart;
} creditsmenu_t;

static creditsmenu_t	s_credits;


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}
	if ( s_credits.quit )
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	else
		UI_PopMenu();
	return 0;
}

float renderTextChar( char ch, const vec3_t origin, const vec3_t axis[3], float scale, const vec4_t color, qhandle_t shader );
int textWidth( const char *line );
static int textLength( const char *line ) {
	int len = 0;
	while (*line) {
		int colorLen = Q_parseColorString( line, 0 );
		if ( colorLen ) {
			line += colorLen;
			continue;
		}
		len++;
		line++;
	}
	return len;
}

#define DROP_SPEED  500
#define DROP_DELAY  200
#define DROP_DEPTH  300

static void renderFallLine( const char *line, const vec3_t origin, const vec3_t axis[3], int time, float scale, qhandle_t shader ) {
	float totalWidth = scale * textWidth( line );
	float curWidth;
	vec4_t color;
	int t, i, charLen;

	color[0] = color[1] = color[2] = color[3] = 1.0f;
	t = 0;
	curWidth = 0;
	charLen = textLength( line );
	for ( i = 0; i < charLen; i++ ) {
		int delta;
		int colorLen = Q_parseColorString( line, color );
		line += colorLen;

		delta = time - t;
		if ( delta > 0) {
			float lerp;
			float y;
			vec3_t start, end;

			y = 0.5f * totalWidth - curWidth;
			VectorSet( start, 0, 0.2f * y , origin[2] * 0.5 );
			VectorSet( end, origin[0], origin[1] + y, origin[2] );
			if ( delta > DROP_SPEED)
				lerp = 1;
			else 
				lerp = delta / (float) DROP_SPEED;
			VectorSubtract( end, start, end );
			VectorMA( start, lerp, end, start );
			curWidth += renderTextChar( *line, start, axis, scale, color, shader );
		} else {
			return;
		}
		line++;
		t += DROP_DELAY;
	}
}

static void creditsRender( float x, float y, float w, float h ) {
	refdef_t refdef;
	int	 deltaTime;
	vec3_t angles, origin, axis[3];

	trap_R_ClearScene();

	deltaTime = uis.realtime - s_credits.showStart;
	VectorSet( angles, 90, 90, 0 );
	AnglesToAxis( angles, axis );

#define LINE_SIZE 30
	VectorSet( origin, 300, 0, 50 );
	renderFallLine( "Programming", origin, axis, deltaTime, 1.0f, s_credits.fontShader );
	origin[2] -= LINE_SIZE;
	renderFallLine( S_COLOR_WHITE"HMage", origin, axis, deltaTime - 1000, 1.0f, s_credits.fontShader );
	origin[2] -= LINE_SIZE;
	renderFallLine( "^1C^7a^1N^7a^1B^7i^1S", origin, axis, deltaTime - 2000, 1.0f, s_credits.fontShader );
	origin[2] -= LINE_SIZE;
	renderFallLine( S_COLOR_GREEN"ent", origin, axis, deltaTime - 3000, 1.0f, s_credits.fontShader );
	origin[2] -= LINE_SIZE * 3;
	renderFallLine( "Testing", origin, axis, deltaTime - 4000, 1.0f, s_credits.fontShader );
	origin[2] -= LINE_SIZE;
	renderFallLine( S_COLOR_YELLOW"Auri", origin, axis, deltaTime - 5000, 1.0f, s_credits.fontShader );

	/* Finalize the refdef */
	UI_AdjustFrom640( &x, &y, &w, &h );
	memset( &refdef, 0, sizeof( refdef ));
	AxisClear( refdef.viewaxis );
	refdef.rdflags = RDF_NOWORLDMODEL;
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;
	refdef.fov_x = 90;
	refdef.fov_y = atan2( refdef.height, (refdef.width / tan( refdef.fov_x / 360 * M_PI )) ) * 360 / M_PI;
	refdef.time = uis.realtime;
	trap_R_RenderScene( &refdef );
}

/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {

	UI_BackGroundRender( 0, 0, 640, 480, 0.8f );
	creditsRender( 0, 0, 640, 480 );

}


/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( qboolean quit ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	s_credits.quit = quit;
	s_credits.showStart = uis.realtime;
	s_credits.fontShader = trap_R_RegisterShaderNoMip( "mme/menu/scrollFont" );
	UI_PushMenu ( &s_credits.menu );
}
