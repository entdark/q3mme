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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (float x, float y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += (2 + CHAR_WIDTH*(width - l))*cgs.widthRatioCoef;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH*cgs.widthRatioCoef, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH*cgs.widthRatioCoef;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK

/*
================
CG_Draw3DModel

================
*/
extern void trap_MME_TimeFraction( float timeFraction );
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;
	trap_MME_TimeFraction(cg.timeFraction);

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( (cg.time / 2000.0) + cg.timeFraction / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = ((cg.time - cg.damageTime ) + cg.timeFraction) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( ( cg.time - cg.headStartTime ) + cg.timeFraction ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size*cgs.widthRatioCoef, size, 
				cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}

/*
================
CG_DrawStatusBar

================
*/
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, cg.snap->ps.persistant[PERS_TEAM] );

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( (cg.time / 1000.0) + cg.timeFraction / 1000.0 );
		CG_Draw3DModel( (CHAR_WIDTH*3 + TEXT_ICON_SPACE)*cgs.widthRatioCoef, 432, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + (CHAR_WIDTH*3 + TEXT_ICON_SPACE)*cgs.widthRatioCoef );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( (cg.time & 2047) + cg.timeFraction ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + (CHAR_WIDTH*3 + TEXT_ICON_SPACE)*cgs.widthRatioCoef, 432, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}

	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( (CHAR_WIDTH*3 + TEXT_ICON_SPACE)*cgs.widthRatioCoef, 432, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + (CHAR_WIDTH*3 + TEXT_ICON_SPACE)*cgs.widthRatioCoef, 432, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE, cgs.media.armorIcon );
		}

	}
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*name;
	int			clientNum;

	if ( !cg.playerPredicted)
		return y;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size*cgs.widthRatioCoef, y, size*cgs.widthRatioCoef, size, clientNum, angles );

	name = cgs.clientinfo[clientNum].name;
	y += size;
	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH)*cgs.widthRatioCoef, y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}



/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;

		CG_DrawBigString( 635 - w, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;
	while ( cg.cpma.detected && cg.playerCent ) {
		unsigned int clientNum = cg.playerCent->currentState.clientNum;
		int temp;
		clientInfo_t* ci;
		areaInfo_t* ai;

		if ( clientNum >= MAX_CLIENTS )
			break;
		ci = cgs.clientinfo + clientNum;
		if ( ci->area >= MAX_AREAS )
			break;
		ai = cgs.areaInfo + ci->area;
		if ( ai->timeLimit <= 0 )
			break;
		if ( ai->timeStart <= 0 )
			break;
		temp = ai->timeStart + (ai->timeLimit * 60 * 1000) - cg.time;
		if ( temp <= 0 )
			break;
		msec = temp;
		break;
	}

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}




/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawFollow

================
*/
static float CG_DrawFollow( float y ) {
	int playerNum;
	clientInfo_t *ci;
	const char *s;

	if ( !cg.playerCent )
		return y;

	if ( !cg.playerCent ) {
		return y;
	}

	playerNum = cg.playerCent->currentState.clientNum;

	if ( playerNum < 0 || playerNum >= MAX_CLIENTS ) {
		return y;
	}
	ci = &cgs.clientinfo[playerNum];
	if ( ci->infoValid ) {
		s = ci->name;
	} else {
		s = va( "Player %d", playerNum );
	}

	y -= BIGCHAR_HEIGHT - 4;
	CG_DrawBigString( 640 - ( Q_PrintStrlen(s ) * BIGCHAR_WIDTH)*cgs.widthRatioCoef, y, s, 0.5 );
	return y;
}


/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( !cg.playerPredicted )
		return y;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}
	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

    if (item) {

		  color = 1;

		  y -= ICON_SIZE;

		  trap_R_SetColor( colors[color] );
		  CG_DrawField( 640 - (640 - x)*cgs.widthRatioCoef, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
		  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			  trap_R_SetColor( NULL );
		  } else {
			  vec4_t	modulate;

			  f = ( ( t - cg.time ) - cg.timeFraction ) / POWERUP_BLINK_TIME;
			  f -= (int)f;
			  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			  trap_R_SetColor( modulate );
		  }

		  if ( cg.powerupActive == sorted[i] && 
			  cg.time - cg.powerupTime < PULSE_TIME ) {
			  f = 1.0f - ( ( cg.time - cg.powerupTime ) + cg.timeFraction ) / PULSE_TIME;
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		  } else {
			  size = ICON_SIZE;
		  }

		  CG_DrawPic( 640 - size*cgs.widthRatioCoef, y + ICON_SIZE / 2 - size / 2, 
			  size*cgs.widthRatioCoef, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	float		x, w;
	vec4_t		color;
	float		y1;
	//gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if ( cg.cpma.detected || ( CG_GameType() >= gameTypeTeamStart ) ) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = (CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8)*cgs.widthRatioCoef;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4*cgs.widthRatioCoef, y, s, 1.0F);

		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = (CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8)*cgs.widthRatioCoef;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4*cgs.widthRatioCoef, y, s, 1.0F);
	} else {
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = (CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8)*cgs.widthRatioCoef;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4*cgs.widthRatioCoef, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = (CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8)*cgs.widthRatioCoef;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4*cgs.widthRatioCoef, y, s, 1.0F);
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = (CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8)*cgs.widthRatioCoef;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	}
	return y1 - 8;
}
#endif

/*
=====================
CG_DrawLowerRight

=====================
*/
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if ( cg.playerPredicted && cg_drawScores.integer ) {
        y = CG_DrawScores( y );
	} else {
		y -= BIGCHAR_HEIGHT + 16;
	}

	if (cg_drawPowerups.integer)
		y = CG_DrawPowerups( y );

	if ( cg_drawFollow.integer ) {
        y = CG_DrawFollow( y );
	}
}

/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;

	if ( cg.playerCent->currentState.eFlags & EF_DEAD ) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColor( cg.itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 8, y, ICON_SIZE*cgs.widthRatioCoef, ICON_SIZE, cg_items[ value ].icon );
			CG_DrawBigString( ICON_SIZE*cgs.widthRatioCoef + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), bg_itemlist[ value ].pickup_name, fadeColor[0] );
			trap_R_SetColor( NULL );
		}
	}
	
	return y;
}
#endif // MISSIONPACK

static void CG_DrawMovementKeys( void ) {
	usercmd_t cmd = { 0 };
	char str1[32] = { 0 }, str2[32] = { 0 };
	float w1 = 0.0f, w2 = 0.0f, height = 0.0f;
	float x, y;
	float scale = cg_drawMovementKeysScale.value;

	if ( !cg.playerPredicted )
		return;

	if ( !cg_drawMovementKeys.integer || !cg.snap ) //RAZTODO: works with demo playback??
		return;

	if ( cg.clientNum == cg.predictedPlayerState.clientNum && !cg.demoPlayback )
		trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
	else {
		int moveDir = cg.snap->ps.movementDir;
		float xyspeed = sqrtf( cg.snap->ps.velocity[0]*cg.snap->ps.velocity[0] + cg.snap->ps.velocity[1]*cg.snap->ps.velocity[1] );

		if ( (cg.snap->ps.pm_flags & PMF_JUMP_HELD) )
			cmd.upmove = 1;
		else if ( (cg.snap->ps.pm_flags & PMF_DUCKED) )
			cmd.upmove = -1;

		if ( xyspeed < 10 )
			moveDir = -1;

		switch ( moveDir ) {
		case 0: // W
			cmd.forwardmove = 1;
			break;
		case 1: // WA
			cmd.forwardmove = 1;
			cmd.rightmove = -1;
			break;
		case 2: // A
			cmd.rightmove = -1;
			break;
		case 3: // AS
			cmd.rightmove = -1;
			cmd.forwardmove = -1;
			break;
		case 4: // S
			cmd.forwardmove = -1;
			break;
		case 5: // SD
			cmd.forwardmove = -1;
			cmd.rightmove = 1;
			break;
		case 6: // D
			cmd.rightmove = 1;
			break;
		case 7: // DW
			cmd.rightmove = 1;
			cmd.forwardmove = 1;
			break;
		default:
			break;
		}
	}
	
	Com_sprintf( str1, sizeof(str1), va( "^%cv ^%cW ^%c^", (cmd.upmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove > 0) ? COLOR_RED : COLOR_WHITE, (cmd.upmove > 0) ? COLOR_RED : COLOR_WHITE ) );
	Com_sprintf( str2, sizeof(str2), va( "^%cA ^%cS ^%cD", (cmd.rightmove < 0) ? COLOR_RED : COLOR_WHITE,
		(cmd.forwardmove < 0) ? COLOR_RED : COLOR_WHITE, (cmd.rightmove > 0) ? COLOR_RED : COLOR_WHITE ) );

	if ( cgs.textFontValid ) {
		scale *= 0.5;
		w1 = CG_Text_Width( "v W ^", scale, 0 );
		w2 = CG_Text_Width( "A S D", scale, 0 );
		height = CG_Text_Height( "A S D v W ^", scale, 0 ) + scale * BIGCHAR_HEIGHT;;
		x = cg.moveKeysPos[0] - max( w1, w2 ) / 2.0f;
		y = cg.moveKeysPos[1] + scale * BIGCHAR_HEIGHT;
		CG_Text_Paint( x, y, scale, colorWhite, str1, qtrue );
		CG_Text_Paint( x, y + height, scale, colorWhite, str2, qtrue );
	} else {
		w1 = scale * BIGCHAR_WIDTH * CG_DrawStrlen( "v W ^" );
		w2 = scale * BIGCHAR_WIDTH * CG_DrawStrlen( "A S D" );
		height = scale * BIGCHAR_HEIGHT;
		x = cg.moveKeysPos[0] - max( w1*cgs.widthRatioCoef, w2*cgs.widthRatioCoef ) / 2.0f;
		y = cg.moveKeysPos[1];
		CG_DrawStringExt( x, y, str1, colorWhite, qfalse, qtrue, scale * BIGCHAR_WIDTH*cgs.widthRatioCoef, scale * BIGCHAR_WIDTH, 0 );
		CG_DrawStringExt( x, y + height, str2, colorWhite, qfalse, qtrue, scale * BIGCHAR_WIDTH*cgs.widthRatioCoef, scale * BIGCHAR_WIDTH, 0 );
	}
}

/*
=====================
CG_DrawLowerLeft

=====================
*/
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	if (cg_drawPickupItem.integer)
		y = CG_DrawPickupItem( y );
}


//===========================================================================================

/*
===================
CG_DrawHoldableItem
===================
*/
static void CG_DrawHoldableItem( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}

}

/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.rewardStack > 0) {
			for(i = 0; i < cg.rewardStack; i++) {
				cg.rewardSound[i] = cg.rewardSound[i+1];
				cg.rewardShader[i] = cg.rewardShader[i+1];
				cg.rewardCount[i] = cg.rewardCount[i+1];
			}
			cg.rewardTime = cg.time;
			cg.rewardStack--;
			color = CG_FadeColor( cg.rewardTime, REWARD_TIME );
			trap_S_StartLocalSound(cg.rewardSound[0], CHAN_ANNOUNCER);
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.rewardCount[0] >= /*10*/ mov_rewardCount.integer ) {
		y = 56;
		x = 320 - (ICON_SIZE/2)*cgs.widthRatioCoef;
		CG_DrawPic( x, y, (ICON_SIZE-4)*cgs.widthRatioCoef, ICON_SIZE-4, cg.rewardShader[0] );
		if (cg.rewardCount[0] > 1 && mov_rewardCount.integer) {
			Com_sprintf(buf, sizeof(buf), "%d", cg.rewardCount[0]);
			x = ( SCREEN_WIDTH - SMALLCHAR_WIDTH * CG_DrawStrlen( buf )*cgs.widthRatioCoef ) / 2;
			CG_DrawStringExt( x, y+ICON_SIZE, buf, color, qfalse, qtrue,
									SMALLCHAR_WIDTH*cgs.widthRatioCoef, SMALLCHAR_HEIGHT, 0 );
		}
	}
	else {

		count = cg.rewardCount[0];

		y = 56;
		x = 320 - count * (ICON_SIZE/2)*cgs.widthRatioCoef;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE-4)*cgs.widthRatioCoef, ICON_SIZE-4, cg.rewardShader[0] );
			x += ICON_SIZE*cgs.widthRatioCoef;
		}
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}
	if (cg.demoPlayback) {
		snap->ping = (snap->serverTime - snap->ps.commandTime) - cg.frametime;
	}
	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;  // bk010215 - FIXME char message[1024];

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted"; // bk 010215 - FIXME
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48*cgs.widthRatioCoef;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, i;
	float	x, y;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer || !cg.playerPredicted ) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = (float)SCREEN_WIDTH - 48.0f*cgs.widthRatioCoef;
	y = (float)SCREEN_HEIGHT - 48.0f;

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48.0f*cgs.widthRatioCoef, 48.0f, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48.0f*2.0f;
	ah = 48.0f;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3.0f;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( colorYellow );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + (0.5*aw - a*0.5)*cgs.widthRatioCoef, mid - v, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( colorBlue );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - a*0.5)*cgs.widthRatioCoef, mid, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( colorYellow );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( colorGreen );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - 0.5*a)*cgs.widthRatioCoef, ay + ah - v, 0.5*cgs.widthRatioCoef, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( colorRed );
			}
			trap_R_DrawStretchPic( ax + (0.5*aw - 0.5*a)*cgs.widthRatioCoef, ay + ah - range, 0.5*cgs.widthRatioCoef, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( !cg.demoPlayback && (cg_nopredict.integer || cg_synchronousClients.integer) ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	float	x, y, w;
	float	*color;
	float	scale;
	float	charW, charH;

	if ( !cg.centerPrintTime ) {
		return;
	}

	charW = BIGCHAR_WIDTH;
	charH = BIGCHAR_HEIGHT;
	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );
	start = cg.centerPrint;
	scale = cg_centerScale.value;
	y = SCREEN_HEIGHT * 0.30 - scale * cg.centerPrintLines * BIGCHAR_HEIGHT / 2;
	if ( cgs.textFontValid ) {
		scale *= 0.5;
		y += scale * BIGCHAR_HEIGHT;
	}
	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;
		if ( cgs.textFontValid ) {
			w = CG_Text_Width( linebuffer, scale, 0 );
			x = ( SCREEN_WIDTH - w ) / 2;
			CG_Text_Paint( x, y, scale, color, linebuffer, qtrue );
			y += 1.3 * CG_Text_Height( linebuffer, scale, 0 );;
		} else {
			w = cg_centerScale.value * charW * CG_DrawStrlen( linebuffer );
			x = ( SCREEN_WIDTH - w*cgs.widthRatioCoef ) / 2;
			CG_DrawStringExt( x, y, linebuffer, color, qfalse, qtrue, scale * charW*cgs.widthRatioCoef, scale * charW, 0 );
			y += scale * charW * 1.5;
		}

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



static void CG_DrawSpeedometer(void) {
	float	x, y, w;
	vec4_t	color = {1.0f, 1.0f, 1.0f, 1.0f};
	float	scale;
	float	charW;
	float	speed;
	vec3_t	velocity;
	char	*speedText;//, *speedFormat;
	
	const	char *format;
	qboolean haveTag = qfalse;
	char	outBuf[512];
	int		outIndex = 0;
	int		outLeft = sizeof(outBuf) - 1;
	char	lastColor[16];
	int		testColor, colorLen = 0;

	if (!cg_drawSpeedometer.integer || !cg.snap)
		return;
	
	charW = BIGCHAR_WIDTH;
	trap_R_SetColor( color );
	scale = cg_drawSpeedometerScale.value;
	y = cg.speedPos[1] + BIGCHAR_HEIGHT;
	
	if (cg.playerPredicted) {
		VectorCopy(cg.predictedPlayerState.velocity, velocity);
	} else {
		VectorCopy(cg.playerCent->currentState.pos.trDelta, velocity);
	}
	speed = sqrtf(velocity[0] * velocity[0] + velocity[1] * velocity[1]);

	//speed formatting begin
	format = cg_drawSpeedometerFormat.string;
	if (!format || !format[0]) 
		format = "Speed: %tups";

	while (*format && outLeft  > 0) {
		if (haveTag) {
			char ch = *format++;
			haveTag = qfalse;
			switch (ch) {
			case 's':		//Speed
				Com_sprintf( outBuf + outIndex, outLeft, "%4", ((int)speed));
				outIndex += strlen( outBuf + outIndex );
				break;
			case 't':		//Speed tabulated
				Com_sprintf( outBuf + outIndex, outLeft, "%4d", ((int)speed));
				outIndex += 4;//strlen( outBuf + outIndex );
				break;
			case '%':
				outBuf[outIndex++] = '%';
				break;
			default:
				continue;
			}
			outLeft = sizeof(outBuf) - outIndex - 1;
			if (colorLen && (outLeft > colorLen)) {
				memcpy( outBuf + outIndex, lastColor, colorLen ); 
				outLeft -= colorLen;
				outIndex += colorLen;
			}
			continue;
		}
		testColor = Q_parseColorString( format, 0 );
		if (testColor ) {
			if (testColor < sizeof( lastColor)) {
                colorLen = testColor;
				memcpy( lastColor, format, colorLen );
			}
			if (testColor < outLeft) {
				memcpy( outBuf + outIndex, format, testColor );
				outIndex += testColor;
				outLeft -= testColor;
			}
			format += testColor;
			continue;
		}
		if (*format == '%') {
			haveTag = qtrue;
			format++;
			continue;
		}
		outBuf[outIndex++] = *format++;
		outLeft = sizeof(outBuf) - outIndex - 1;
	}
	outBuf[ outIndex ] = 0;
	speedText = outBuf;
	//speed formatting end

	if ( cgs.textFontValid ) {
		scale *= 0.5;
		y += scale * BIGCHAR_HEIGHT;
		w = CG_Text_Width( speedText, scale, 0 );
		x = cg.speedPos[0] - w / 2.0f;
		CG_Text_Paint( x, y, scale, color, speedText, qtrue );
	} else {
		w = cg_drawSpeedometerScale.value * charW * CG_DrawStrlen( speedText );
		x = cg.speedPos[0] - (w / 2.0f)*cgs.widthRatioCoef;
		CG_DrawStringExt( x, y, speedText, color, qfalse, qtrue, scale * charW*cgs.widthRatioCoef, scale * charW, 0 );
	}
}

/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.playerPredicted && cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = (cg.time - cg.itemPickupBlendTime) + cg.timeFraction;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640( &x, &y, &w, &h );

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	trap_R_DrawStretchPic( x + cg.refdef.x + 0.5 * (cg.refdef.width - w*cgs.widthRatioCoef), 
		y + cg.refdef.y + 0.5 * (cg.refdef.height - h), 
		w*cgs.widthRatioCoef, h, 0, 0, 1, 1, hShader );
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.playerCent->currentState.number, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = trap_CM_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH*cgs.widthRatioCoef;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
	trap_R_SetColor( NULL );
}



static qboolean CG_DrawScoreboard( void ) {
	return CG_DrawOldScoreboard();
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}

/*====================================
chatbox functionality -rww
====================================*/
#define	CHATBOX_CUTOFF_LEN	550
#define	CHATBOX_FONT_HEIGHT	20

//utility func, insert a string into a string at the specified
//place (assuming this will not overflow the buffer)
static void CG_ChatBox_StrInsert(char *buffer, int place, char *str) {
	int insLen = strlen(str);
	int i = strlen(buffer);
	int k = 0;

	buffer[i+insLen+1] = 0; //terminate the string at its new length
	while (i >= place) {
		buffer[i+insLen] = buffer[i];
		i--;
	}

	i++;
	while (k < insLen) {
		buffer[i] = str[k];
		i++;
		k++;
	}
}

//add chatbox string
void CG_ChatBox_AddString(char *chatStr) {
	chatBoxItem_t *chat = &cg.chatItems[cg.chatItemActive];
	float chatLen;

	if (mov_chatBox.integer<=0) {
	//don't bother then.
		return;
	}

	memset(chat, 0, sizeof(chatBoxItem_t));

	if (strlen(chatStr) > sizeof(chat->string)) {
	//too long, terminate at proper len.
		chatStr[sizeof(chat->string)-1] = 0;
	}

	strcpy(chat->string, chatStr);
	chat->time = cg.time + mov_chatBox.integer;

	chat->lines = 1;

	chatLen = CG_Text_Width(chat->string, 1.0f, 0);
	if (chatLen > CHATBOX_CUTOFF_LEN) {
	//we have to break it into segments...
        int i = 0;
		int lastLinePt = 0;
		char s[2];

		chatLen = 0;
		while (chat->string[i]) {
			s[0] = chat->string[i];
			s[1] = 0;
			chatLen += CG_Text_Width(s, 0.65f, 0);

			if (chatLen >= CHATBOX_CUTOFF_LEN) {
				int j = i;
				while (j > 0 && j > lastLinePt) {
					if (chat->string[j] == ' ') {
						break;
					}
					j--;
				}
				if (chat->string[j] == ' ') {
					i = j;
				}

                chat->lines++;
				CG_ChatBox_StrInsert(chat->string, i, "\n");
				i++;
				chatLen = 0;
				lastLinePt = i+1;
			}
			i++;
		}
	}

	cg.chatItemActive++;
	if (cg.chatItemActive >= MAX_CHATBOX_ITEMS) {
		cg.chatItemActive = 0;
	}
}

//insert item into array (rearranging the array if necessary)
void CG_ChatBox_ArrayInsert(chatBoxItem_t **array, int insPoint, int maxNum, chatBoxItem_t *item) {
    if (array[insPoint]) { //recursively call, to move everything up to the top
		if (insPoint+1 >= maxNum) {
			CG_Error("CG_ChatBox_ArrayInsert: Exceeded array size");
		}
		CG_ChatBox_ArrayInsert(array, insPoint+1, maxNum, array[insPoint]);
	}
	//now that we have moved anything that would be in this slot up, insert what we want into the slot
	array[insPoint] = item;
}

//go through all the chat strings and draw them if they are not yet expired
static ID_INLINE void CG_ChatBox_DrawStrings(void) {
	chatBoxItem_t *drawThese[MAX_CHATBOX_ITEMS];
	int numToDraw = 0;
	int linesToDraw = 0;
	int i = 0;
	float x = 44.0f*cgs.widthRatioCoef;
	float y = cg.scoreBoardShowing ? 475.0 : mov_chatBoxHeight.value;
	float fontScale = 0.65f * mov_chatBoxScale.value;

	if (!mov_chatBox.integer) {
		return;
	}
//	if (cg.chatItems->time > mov_chatBox.integer + cg.time) {
//		cg.chatItems->time = cg.time;
//		return;
//	}
	memset(drawThese, 0, sizeof(drawThese));

	while (i < MAX_CHATBOX_ITEMS) {
		if (cg.chatItems[i].time >= cg.time) {
			int check = numToDraw;
			int insertionPoint = numToDraw;
			while (check >= 0) {
				if (drawThese[check] && cg.chatItems[i].time < drawThese[check]->time) {
				//insert here
					insertionPoint = check;
				}
				check--;
			}
			CG_ChatBox_ArrayInsert(drawThese, insertionPoint, MAX_CHATBOX_ITEMS, &cg.chatItems[i]);
			numToDraw++;
			linesToDraw += cg.chatItems[i].lines;
		}
		i++;
	}

	if (!numToDraw) { //nothing, then, just get out of here now.
		return;
	}

	//move initial point up so we draw bottom-up (visually)
	y -= (CHATBOX_FONT_HEIGHT*fontScale)*linesToDraw;

	//we have the items we want to draw, just quickly loop through them now
	i = 0;
	if ( cgs.textFontValid ) {
		fontScale *= 0.5;
		y += fontScale * BIGCHAR_HEIGHT;
	}
	while (i < numToDraw) {
		if ( cgs.textFontValid ) {
			CG_Text_Paint(x, y, fontScale, colorWhite, drawThese[i]->string, qtrue);
			y += ((CHATBOX_FONT_HEIGHT*fontScale)*drawThese[i]->lines);
			y += CG_Text_Height( drawThese[i]->string, fontScale, 0 );
		} else {
			CG_DrawStringExt( x, y, drawThese[i]->string, colorWhite, qfalse, qtrue, fontScale * BIGCHAR_WIDTH*cgs.widthRatioCoef, fontScale * BIGCHAR_WIDTH, 0 );
			y += ((CHATBOX_FONT_HEIGHT*fontScale)*drawThese[i]->lines);
		}
		i++;
	}
	trap_R_SetColor( NULL );
}

/*
=================
CG_Draw2D
=================
*/
void CG_Draw2D( void ) {
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		CG_ChatBox_DrawStrings();
		return;
	}

	if ( !cg.playerCent )
		return;
	
	if ( cg.playerPredicted ) {
		if ( cgs.clientinfo[ cg.playerCent->currentState.clientNum].team == TEAM_SPECTATOR ) {
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
		} else {
			// don't draw any status if dead or the scoreboard is being explicitly shown
			if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {
				CG_DrawStatusBar();
				CG_DrawAmmoWarning();
				CG_DrawCrosshair();
				CG_DrawCrosshairNames();
				CG_DrawWeaponSelect();
				CG_DrawHoldableItem();
				CG_DrawReward();
			}
		}
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && !(cg.playerCent->currentState.eFlags & EF_DEAD) ) {
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
		}
		CG_DrawSpeedometer();
		CG_ChatBox_DrawStrings();
	}
	CG_DrawLagometer();
	CG_DrawUpperRight();
	CG_DrawLowerRight();
	CG_DrawLowerLeft();

	CG_DrawSpeedometer();
	CG_DrawMovementKeys();
			
	// don't draw center string if scoreboard is up
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing) {
		CG_DrawCenterString();
	}
	
	// always draw chat
	CG_ChatBox_DrawStrings();
}


static void CG_DrawTourneyScoreboard( void ) {
#ifdef MISSIONPACK
#else
	CG_DrawOldTourneyScoreboard();
#endif
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	float		separation;
	vec3_t		baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	switch ( stereoView ) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error( "CG_DrawActive: Undefined stereoView" );
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );
	if ( separation != 0 ) {
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 ) {
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// draw status bar and other floating elements
 	CG_Draw2D();
}



