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
#define	MAIN_STARS				200
#define	MAIN_RAILS				20
#define	MAIN_RAIL_FADE			500
// S_COLOR_BLUE
static char *circleText = "^1Q^7uake^13 ^1M^7ovie ^1M^7akers ^1E^7dition";

typedef qhandle_t boxShader_t[6];
static boxShader_t boxCenterShaders;
static boxShader_t boxPlatformShaders;
typedef struct {
	vec3_t	pos, velocity;
	vec3_t	rotate, spin;
	int		hits;
	byte	color[3];
	float	scale;
} renderStar_t;

typedef struct {
	vec3_t	start, end;
	byte	color1[3];
	byte	color2[3];
	int		lifeStart, lifeEnd;
} renderRail_t;

typedef struct {
	playerInfo_t	info;
	int				shotDelay;
	vec3_t			moveOrigin, moveAngles;
	int				moveEnd;
	vec3_t			angles, origin;
	renderStar_t	*target;
} renderPlayer_t;

typedef struct {
	qhandle_t		starModel;
	qhandle_t		starShader;
	qhandle_t		fontShader;
	qhandle_t		fontShadowShader;
	qhandle_t		centerTopShader;
	qhandle_t		centerSideShader;
	qhandle_t		railRingsShader;
	sfxHandle_t		railFireSound;
	qhandle_t		railCoreShader;
	qhandle_t		darkenShader;
	qhandle_t		backgroundShader;

	qboolean		cached;
	renderStar_t	stars[MAIN_STARS];
	renderRail_t	rails[MAIN_RAILS];
	renderPlayer_t	player;
	int				seed;
} background_t;

static background_t s_bg;

static float globeRand( float base, float range ) {
	float r = Q_crandom( &s_bg.seed) * range;
	if (r < 0)
		return r - base;
	else
		return r + base;
}


static float getAngle( int interval ) {
	interval *= 1000;
	return 360.0 * ( uis.realtime % interval ) / interval;
}

static float getSwing( int interval, float range ) {
	float angle = getAngle( interval );
	return range * sin(DEG2RAD(angle));
}

extern int	propMap[128][3];
static void renderBox( const vec3_t origin, const vec3_t axis[3], const vec3_t dim, const vec4_t color, const qhandle_t shaders[6] ) {
	int s, i;
	polyVert_t verts[4];
	static float mulTable[8][2] = {
		{ -1, -1 }, { -1, 1 }, { 1, 1}, { 1, -1 }
	};

	for (i = 0;i<4;i++) {
		verts[i].modulate[0] = color[0] * 0xff;
		verts[i].modulate[1] = color[1] * 0xff;
		verts[i].modulate[2] = color[2] * 0xff;
		verts[i].modulate[3] = color[3] * 0xff;
	}

	for (s = 0; s<6 ; s++ ) {
		float scale = s & 1 ? -1 : 1;
		int d = s >> 1;
		for ( i=0 ; i<4 ; i++) {
			polyVert_t *v = verts + (i ^ (( (i & s & 1) << 1)));
			v->xyz[d] = dim[d] * scale;
			v->xyz[(d+1) % 3] = dim[(d+1) % 3] * mulTable[i][0];
			v->xyz[(d+2) % 3] = dim[(d+2) % 3] * mulTable[i][1];
			v->st[0] = mulTable[i][0];
			v->st[1] = mulTable[i][1];
			VectorRotateDirect( v->xyz, axis );
			VectorAdd( origin, v->xyz, v->xyz );
		}
		trap_R_AddPolyToScene( shaders[s], 4, verts );
	}
}

int textWidth( const char *line ) {
	int width = 0;	
	while (line[0]) {
		int colorLen;
		char ch;

		colorLen = Q_parseColorString( line, 0 );
		if (colorLen) {
			line += colorLen;
			continue;
		}
		ch = line[0] & 127;
		if ( ch == '\n') {
			break;
		} else if ( ch == ' ' ) {
			width += PROP_SPACE_WIDTH;
			continue;
		} else if ( propMap[ch][2] != -1 ) {
			width += propMap[ch][2];
		}
		line++;
	}
	return width;
}

static int textHeight( const char *line ) {
	int lines = 1;	
	while (line[0]) {
		if (line[0] == '\n')
			lines++;
		line++;
	}
	return lines * (PROP_HEIGHT + PROP_GAP_WIDTH);
}

float renderTextChar( char ch, const vec3_t origin, const vec3_t axis[3], float scale, const vec4_t color, qhandle_t shader ) {
	float fcol, frow, fwidth, fheight, aw;
	polyVert_t verts[4];
	int i;

	for (i = 0;i<4;i++) {
		verts[i].modulate[0] = color[0] * 0xff;
		verts[i].modulate[1] = color[1] * 0xff;
		verts[i].modulate[2] = color[2] * 0xff;
		verts[i].modulate[3] = color[3] * 0xff;
	}
	ch &= 127;
	if ( ch == '\n') {
		return 0;
	} else if ( ch == ' ' ) {
		return scale * PROP_SPACE_WIDTH;
	} else if ( propMap[ch][2] != -1 ) {
		fcol = (float)propMap[ch][0] / 256.0f;
		frow = (float)propMap[ch][1] / 256.0f;
		fwidth = (float)propMap[ch][2] / 256.0f;
		fheight = (float)PROP_HEIGHT / 256.0f;
		aw = (float)propMap[ch][2] * 1;
	} else {
		return 0;
	}
	verts[2].st[0] = fcol + fwidth;
	verts[2].st[1] = frow + fheight;
	verts[1].st[0] = fcol;
	verts[1].st[1] = frow + fheight;
	verts[0].st[0] = fcol;
	verts[0].st[1] = frow;
	verts[3].st[0] = fcol + fwidth;
	verts[3].st[1] = frow;

	verts[0].xyz[0] = 0;
	verts[0].xyz[1] = PROP_HEIGHT * scale;
	verts[1].xyz[0] = 0;
	verts[1].xyz[1] = 0;

	verts[2].xyz[0] = aw * scale;
	verts[2].xyz[1] = 0;
	verts[3].xyz[0] = aw * scale;
	verts[3].xyz[1] = PROP_HEIGHT * scale;

	verts[0].xyz[2] = verts[1].xyz[2] = 
	verts[2].xyz[2] = verts[3].xyz[2] = 0;
	for (i = 0;i<4;i++) {
		VectorRotateDirect( verts[i].xyz, axis );
		VectorAdd( verts[i].xyz, origin, verts[i].xyz );
	}
	trap_R_AddPolyToScene( shader, 4, verts );
	return (aw + PROP_GAP_WIDTH) * scale;
}

void renderTextSurface( const char *line, const vec3_t origin, const vec3_t axis[3], float scale, qhandle_t shader ) {
	int i;

	polyVert_t verts[4];

	float	ax, ay,aw, ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;	
	int		colorLen;
	vec3_t	color;

	for (i = 0;i<4;i++) {
		verts[i].modulate[0] = 0xff;
		verts[i].modulate[1] = 0xff;
		verts[i].modulate[2] = 0xff;
		verts[i].modulate[3] = 0xff;
	}
	ah = PROP_HEIGHT * scale;
	ax = textWidth( line ) * scale * -0.5;
	ay = textHeight( line ) * scale * -0.5;
	for( ;line[0];line++) {
		char ch;	
		colorLen = Q_parseColorString( line, color );
		if (colorLen ) {
			for (i = 0;i<4;i++) {
				verts[i].modulate[0] = color[0] * 255;
				verts[i].modulate[1] = color[1] * 255;
				verts[i].modulate[2] = color[2] * 255;;
				verts[i].modulate[3] = 0xff;
			}
			line += colorLen - 1;
			continue;
		}
		ch = line[0] & 127;
		if ( ch == '\n') {
			ax = textWidth( line + 1 ) * scale * -0.5;;
			ay -= scale * (PROP_HEIGHT + PROP_GAP_WIDTH);
			continue;
		} else if ( ch == ' ' ) {
			ax += scale * PROP_SPACE_WIDTH;
			continue;
		} else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * 1;
		} else {
			continue;

		}

		verts[2].st[0] = fcol + fwidth;
		verts[2].st[1] = frow + fheight;

		verts[1].st[0] = fcol;
		verts[1].st[1] = frow + fheight;

		verts[0].st[0] = fcol;
		verts[0].st[1] = frow;

		verts[3].st[0] = fcol + fwidth;
		verts[3].st[1] = frow;


		verts[0].xyz[0] = ax;
		verts[0].xyz[1] = ay + ah;
		verts[1].xyz[0] = ax;
		verts[1].xyz[1] = ay;

		ax += aw * scale;
		verts[2].xyz[0] = ax;
		verts[2].xyz[1] = ay;
		verts[3].xyz[0] = ax;
		verts[3].xyz[1] = ay + ah;

		verts[0].xyz[2] = verts[1].xyz[2] = 
		verts[2].xyz[2] = verts[3].xyz[2] = 0;
		for (i = 0;i<4;i++) {
			VectorRotateDirect( verts[i].xyz, axis );
			VectorAdd( verts[i].xyz, origin, verts[i].xyz );
		}
		ax += PROP_GAP_WIDTH * scale;
		trap_R_AddPolyToScene( shader, 4, verts );
	}
}

static float charFillCoords( char ch, polyVert_t verts[4] ) {
	float fcol, frow, fwidth, fheight, aw;

	ch &= 127;
	if ( ch == ' ' ) {
		return 0;
	} else if ( propMap[ch][2] != -1 ) {
		fcol = (float)propMap[ch][0] / 256.0f;
		frow = (float)propMap[ch][1] / 256.0f;
		fwidth = (float)propMap[ch][2] / 256.0f;
		fheight = (float)PROP_HEIGHT / 256.0f;
		aw = (float)propMap[ch][2] * 1;
	} else {
		return 0;
	}

	verts[1].st[0] = fcol;
	verts[1].st[1] = frow;

	verts[0].st[0] = fcol;
	verts[0].st[1] = frow + fheight;

	verts[3].st[0] = fcol + fwidth;
	verts[3].st[1] = frow + fheight;

	verts[2].st[0] = fcol + fwidth;
	verts[2].st[1] = frow;
	return aw;
}

void renderTextCylinder( const char *line, const vec3_t origin, const vec3_t axis[3], float radius, float scale, qhandle_t shader ) {
	int i;
	polyVert_t verts[4];

	float	aw;
	int		colorLen;
	vec3_t	color;
	float	angle = 0;
	const float	radiusScale = 0.95 * scale / ( radius );

	for (i = 0;i<4;i++) {
		verts[i].modulate[0] = 0xff;
		verts[i].modulate[1] = 0xff;
		verts[i].modulate[2] = 0xff;
		verts[i].modulate[3] = 0xff;
	}

	for( ;line[0];line++) {
		char ch;	
		colorLen = Q_parseColorString( line, color );
		if (colorLen ) {
			for (i = 0;i<4;i++) {
				verts[i].modulate[0] = color[0] * 255;
				verts[i].modulate[1] = color[1] * 255;
				verts[i].modulate[2] = color[2] * 255;;
				verts[i].modulate[3] = 0xff;
			}
			line += colorLen - 1;
			continue;
		}
		ch = line[0] & 127;
		aw = charFillCoords( ch, verts );

		if (!aw) {
			angle += PROP_SPACE_WIDTH * radiusScale;
			continue;
		}

		verts[0].xyz[0] = verts[1].xyz[0] = cos(angle) * radius;
		verts[0].xyz[1] = verts[1].xyz[1] = sin(angle) * radius;
		angle += aw * radiusScale;
		verts[2].xyz[0] = verts[3].xyz[0] = cos(angle) * radius;
		verts[2].xyz[1] = verts[3].xyz[1] = sin(angle) * radius;
		angle += PROP_GAP_WIDTH * radiusScale;

		verts[0].xyz[2] = 0; 
		verts[1].xyz[2] = PROP_HEIGHT * scale;
		verts[2].xyz[2] = PROP_HEIGHT * scale;	
		verts[3].xyz[2] = 0;

		for (i = 0;i<4;i++) {
			VectorRotateDirect( verts[i].xyz, axis );
			VectorAdd( verts[i].xyz, origin, verts[i].xyz );
		}
		trap_R_AddPolyToScene( shader, 4, verts );
	}
}


static void renderTextCircle( const char *line, const vec3_t origin, const vec3_t axis[3], float radius, float scale, qhandle_t shader ) {
	int i;
	polyVert_t verts[4];

	float	aw;
	int		colorLen;
	vec3_t	color;
	float	angle = 0;
	float	topMul;
	//Slightly lower scale looks better
	const float	radiusScale = -scale / ( radius );

	for (i = 0;i<4;i++) {
		verts[i].modulate[0] = 0xff;
		verts[i].modulate[1] = 0xff;
		verts[i].modulate[2] = 0xff;
		verts[i].modulate[3] = 0xff;
	}
	radius -= PROP_HEIGHT;
	topMul = (radius + PROP_HEIGHT * scale) / radius;
	for( ;line[0];line++) {
		char ch;	
		colorLen = Q_parseColorString( line, color );
		if (colorLen ) {
			for (i = 0;i<4;i++) {
				verts[i].modulate[0] = color[0] * 255;
				verts[i].modulate[1] = color[1] * 255;
				verts[i].modulate[2] = color[2] * 255;;
				verts[i].modulate[3] = 0xff;
			}
			line += colorLen - 1;
			continue;
		}

		ch = line[0] & 127;
		aw = charFillCoords( ch, verts );

		if (!aw) {
			angle += PROP_SPACE_WIDTH * radiusScale;
			continue;
		}

		verts[0].xyz[0] = cos(angle) * radius;
		verts[0].xyz[1] = sin(angle) * radius;
		verts[1].xyz[0] = verts[0].xyz[0] * topMul;
		verts[1].xyz[1] = verts[0].xyz[1] * topMul;

		angle += aw * radiusScale;
		verts[3].xyz[0] = cos(angle) * radius;
		verts[3].xyz[1] = sin(angle) * radius;
		verts[2].xyz[0] = verts[3].xyz[0] * topMul;
		verts[2].xyz[1] = verts[3].xyz[1] * topMul;

		verts[0].xyz[2] = verts[1].xyz[2] = 
		verts[2].xyz[2] = verts[3].xyz[2] = 0;
		for (i = 0;i<4;i++) {
			VectorRotateDirect( verts[i].xyz, axis );
			VectorAdd( verts[i].xyz, origin, verts[i].xyz );
		}
		angle += PROP_GAP_WIDTH * radiusScale;
		trap_R_AddPolyToScene( shader, 4, verts );
	}
}

static void starOrigin( const renderStar_t *star, vec3_t origin ) {
	float	angle, sp, sy,  cp, cy;
	
	angle = star->pos[YAW] * (M_PI*2 / 360);sy = sin(angle);cy = cos(angle);
	angle = star->pos[PITCH] * (M_PI*2 / 360);sp = sin(angle);cp = cos(angle);

	origin[0] = cp*cy*star->pos[2];
	origin[1] = cp*sy*star->pos[2];
	origin[2] = -sp*star->pos[2];
}


static void renderStars( void ) {
	int i;
	refEntity_t ref;

	memset( &ref, 0, sizeof(ref) );
	ref.reType = RT_MODEL;
	ref.hModel = s_bg.starModel;
	ref.customShader = s_bg.starShader;
	ref.nonNormalizedAxes = qtrue;

	for (i=0;i<MAIN_STARS;i++) {
		int j;
		renderStar_t *star = s_bg.stars + i;

		star->pos[YAW] = AngleNormalize180( star->pos[YAW] + uis.frametime * star->velocity[YAW]);
		star->pos[PITCH] = AngleNormalize180( star->pos[PITCH] + uis.frametime * star->velocity[PITCH]);

		starOrigin( star, ref.origin );
		/* Rotate the star */
		for (j=0;j<3;j++)
			star->rotate[j] = AngleNormalize180( star->rotate[j] + uis.frametime * star->spin[j]);
		AnglesToAxis( star->rotate, ref.axis );

		VectorScale( ref.axis[0], star->scale, ref.axis[0] );
		VectorScale( ref.axis[1], star->scale, ref.axis[1] );
		VectorScale( ref.axis[2], star->scale, ref.axis[2] );

		ref.shaderRGBA[0] = star->color[0];
		ref.shaderRGBA[1] = star->color[1];
		ref.shaderRGBA[2] = star->color[2];
		ref.shaderRGBA[3] = 255;

		trap_R_AddRefEntityToScene( &ref );
	}
}

#define TURN_SPEED (0.001f * 360)

static renderStar_t *findTarget( const vec3_t curAngles  ) {
	int possible[MAIN_STARS];
	int count = 0;
	int i, lowest;

	lowest = 1 << 30;
	for (i = 0;i<MAIN_STARS;i++) {
		int deltaTime;
		float deltaAngle;
		renderStar_t *star = &s_bg.stars[i];

		if ( star->hits > lowest) 
			continue;

		deltaAngle = AngleNormalize180( curAngles[YAW] - star->pos[YAW] );

		if ( deltaAngle < 0) {
			deltaTime = deltaAngle / ( +TURN_SPEED - star->velocity[YAW]);
		} else {
			deltaTime = deltaAngle / ( -TURN_SPEED - star->velocity[YAW]);
		} 

		deltaAngle = AngleNormalize180( star->pos[PITCH] + deltaTime * star->velocity[PITCH]);
		if ( deltaAngle > 2)
			continue;
		if ( deltaAngle < -50 )
			continue;

		if ( star->hits < lowest) {
			lowest = star->hits;
			count = 0;
		}

		possible[count++] = i;
	}
	if (!count)
		return 0;
#if 0
	for (i = 0; i < count;i++) {
		refEntity_t ref;
		renderStar_t *star = &s_bg.stars[possible[i]];
		float	angle, sp, sy,  cp, cy;


		memset( &ref, 0, sizeof( ref ));
		ref.customShader = s_bg.starShader;
		VectorSet( ref.origin, 0, 0, 30 );
		angle = star->pos[YAW] * (M_PI*2 / 360);sy = sin(angle);cy = cos(angle);
		angle = star->pos[PITCH] * (M_PI*2 / 360);sp = sin(angle);cp = cos(angle);
		ref.oldorigin[0] = cp*cy*star->pos[2];
		ref.oldorigin[1] = cp*sy*star->pos[2];
		ref.oldorigin[2] = -sp*star->pos[2];
		
		ref.shaderRGBA[0] = ref.shaderRGBA[1] = ref.shaderRGBA[2] = ref.shaderRGBA[3] = 0xff;
		ref.reType = RT_RAIL_CORE;
		ref.radius = 2;
		AxisClear( ref.axis );
		trap_R_AddRefEntityToScene( &ref );
	}
#endif
	return s_bg.stars + possible[rand() % count];
}

static void addRail( const vec3_t start, const vec3_t end, const vec3_t color1, const vec3_t color2, int lifeTime ) {
	int i;

	for (i = 0;i<MAIN_RAILS;i++) {
		renderRail_t *rail = s_bg.rails + i;
		
		if ( rail->lifeEnd >= uis.realtime )
			continue;
		VectorCopy( start, rail->start );
		VectorCopy( end, rail->end );

		rail->color1[0] = 255 * color1[0];
		rail->color1[1] = 255 * color1[1];
		rail->color1[2] = 255 * color1[2];
		rail->color2[0] = 255 * color2[0];
		rail->color2[1] = 255 * color2[1];
		rail->color2[2] = 255 * color2[2];
		rail->lifeStart = uis.realtime;
		rail->lifeEnd = uis.realtime + lifeTime;
		return;
	}
}
static void renderRails( void ) {
	int i;
	refEntity_t ref;

	memset( &ref, 0, sizeof( ref ));
	AxisClear( ref.axis );

	for (i = 0;i<MAIN_RAILS;i++) {
		int fade;
		renderRail_t *rail = s_bg.rails + i;
		
		if ( rail->lifeEnd <= uis.realtime )
			continue;
		fade = (255 * ( rail->lifeEnd - uis.realtime )) / ( rail->lifeEnd - rail->lifeStart );
		VectorCopy( rail->start, ref.origin );
		VectorCopy( rail->end, ref.oldorigin );
		ref.shaderRGBA[0] = (rail->color1[0] * fade) / 255;
		ref.shaderRGBA[1] = (rail->color1[1] * fade) / 255;
		ref.shaderRGBA[2] = (rail->color1[2] * fade) / 255;
		ref.shaderRGBA[3] = fade;
	
		ref.reType = RT_RAIL_CORE;
		ref.customShader = s_bg.railCoreShader;
		trap_R_AddRefEntityToScene( &ref );
		ref.shaderRGBA[0] = (rail->color2[0] * fade) / 255;
		ref.shaderRGBA[1] = (rail->color2[1] * fade) / 255;
		ref.shaderRGBA[2] = (rail->color2[2] * fade) / 255;
		ref.shaderRGBA[3] = fade;

		ref.reType = RT_RAIL_RINGS;
		ref.customShader = s_bg.railRingsShader;
		trap_R_AddRefEntityToScene( &ref );
	}
}


static void renderPlayer( void ) {
	renderPlayer_t *player = &s_bg.player;
	renderStar_t *target;

//	if (0 //findTarget( player->angles )) {
//
//	} else 
	
	if ( player->info.legsAnimationTimer || player->info.torsoAnimationTimer) {
	} else if ( player->shotDelay > 0) {
		player->shotDelay -= uis.frametime;
		player->angles[PITCH] = 0;
	} else if (player->target ) {
		vec2_t diff;
		float distance, canReach;
		renderStar_t *star = player->target;
	
		diff[0] = AngleNormalize180( player->angles[0] - star->pos[0] );
		diff[1] = AngleNormalize180( player->angles[1] - star->pos[1] );

		distance = sqrt( diff[0]*diff[0] + diff[1]*diff[1]);
		canReach = (uis.frametime * TURN_SPEED);
		if (distance < canReach ) {
			char c;
			vec3_t start, end, color1, color2;

			player->angles[0] = star->pos[0];
			player->angles[1] = star->pos[1];
			player->shotDelay = 1200;
			player->target = 0;

			/* Hacky way of getting the rail start */
			AngleVectors( player->angles, start, color1, 0);
			VectorScale( start, 13.5f, start );
			VectorMA( start, 12, color1, start );
			start[2] += player->origin[2] + 22;
			/* Recalculate star pos for end */
			starOrigin( star, end );

			c = (rand() % 24) + 'a';
			Q_parseColor( &c, defaultColors, color1 );
			c = (rand() % 24) + 'a';
			Q_parseColor( &c, defaultColors, color2 );
			
			star->color[0] = 255 * color1[0];
			star->color[1] = 255 * color1[1];
			star->color[2] = 255 * color1[2];
			star->hits++;
			addRail( start, end, color1, color2, 3000 );

			UI_PlayerInfo_SetInfo( &player->info, LEGS_IDLE, TORSO_ATTACK2, player->angles, player->angles, WP_RAILGUN, qfalse );
			trap_S_StartLocalSound( s_bg.railFireSound, 0 );

		} else {
			canReach /= distance;
			player->angles[0] = LerpAngle( player->angles[0], star->pos[0], canReach );
			player->angles[1] = LerpAngle( player->angles[1], star->pos[1], canReach );
			UI_PlayerInfo_SetInfo( &player->info, LEGS_IDLE, TORSO_STAND, player->angles, player->angles, WP_RAILGUN, qfalse );
		}
	} else {
		player->angles[PITCH] = 0;
		switch (Q_rand( &s_bg.seed) % 15) {
		case 0:
			UI_PlayerInfo_SetInfo( &player->info, LEGS_JUMP, TORSO_STAND, player->angles, player->angles, WP_RAILGUN, qfalse );
			UI_PlayerInfo_Sound( &player->info, "jump1" );
			break;
		case 1:
			UI_PlayerInfo_SetInfo( &player->info, LEGS_IDLE, TORSO_GESTURE, player->angles, player->angles, WP_RAILGUN, qfalse );
			UI_PlayerInfo_Sound( &player->info, "taunt" );
			break;
		default:
			target = findTarget( player->angles );
			if (!target)
				break;
			player->target = target;
			UI_PlayerInfo_SetInfo( &player->info, LEGS_IDLE, TORSO_STAND, player->angles, player->angles, WP_RAILGUN, qfalse );
			break;
		}
	}
	UI_RenderPlayer( player->origin, &player->info );

}

static renderTextBox( const char *line, const vec3_t origin, const vec3_t axis[3], float scale, const vec3_t dim, qhandle_t shaders[6] ) {
	vec3_t textOrigin;
	renderBox( origin, axis, dim, colorWhite, shaders );
	VectorCopy( origin, textOrigin );
	VectorMA( textOrigin, dim[2] + 0.2, axis[2], textOrigin );
	renderTextSurface( line, textOrigin, axis, scale, s_bg.fontShader );
}

void UI_BackGroundRender ( float x, float y, float w, float h, float dimming ) {
	vec3_t origin, target, forward, angles;
	float angle, distance;
	vec3_t axis[3];
	refdef_t refdef;

	static vec3_t boxDim = { 80, 80, 8 };
	static vec3_t boxOrigin = { 0, 0, -8 };
	static vec4_t boxColor = { 0.1f, 0.1f, 0.7f, 1.0f };

	UI_AdjustFrom640( &x, &y, &w, &h );

	trap_R_DrawStretchPic( x, y, w, h, 0, 0, 1, 1, s_bg.backgroundShader );

	/* Main distance to target slightly fluctuates */
	angle = DEG2RAD(getAngle( 20 ));
	distance = 240 + getSwing( 15, 40 ) + getSwing( 35, 50 );

	origin[0] = sin( angle ) * distance;
	origin[1] = cos( angle ) * distance;
	origin[2] = 150 + getSwing( 23, 50 );

	distance = 100 + getSwing( 19, 10 );

	angle -= 0.5 * M_PI + getSwing( 101, M_PI * 0.1);
//	angle -= 0.5 * M_PI;
	target[0] = sin( angle ) * distance;
	target[1] = cos( angle ) * distance;
//	target[0] = 0;
//	target[1] = 0;
	target[2] =  getSwing( 21, 30 );

	VectorSubtract( target, origin, forward );
	vectoangles( forward, angles );

	angles[ROLL] = getSwing( 40, 10 );
	memset( &refdef, 0, sizeof( refdef ));
	/* Setup the refdef */
	VectorCopy( origin, refdef.vieworg );
	AnglesToAxis( angles, refdef.viewaxis );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;
	refdef.fov_x = 95;
	refdef.fov_y = atan2( refdef.height, (refdef.width / tan( refdef.fov_x / 360 * M_PI )) ) * 360 / M_PI;
	refdef.time = uis.realtime;
	refdef.rdflags = RDF_NOWORLDMODEL;

	trap_R_ClearScene();

	/* Create the box bottom */
	renderBox( boxOrigin, axisDefault, boxDim, boxColor, boxCenterShaders );
	renderPlayer();
	renderRails();

	VectorClear( origin );
	origin[2] = 100;
	angles[PITCH] = 0;
	angles[YAW] = getAngle( 7 );
	angles[PITCH] = getSwing( 33, 20 );
	AnglesToAxis( angles, axis );
//	renderTextCylinder( "Hurray a new q3mmme version has arrived", origin, axis, 120, 1, s_bg.fontShader );
//	VectorCopy( boxOrigin, origin );
//	origin[2] += 50;
//	renderTextBox( "^1P^0rogammaing\n^1C^0a^1N^0a^1B^0i^1S", origin, axis, boxDim, 0.5 );
	
	/* Create the scrolling text */
	angles[PITCH] = 0;
	angles[YAW] = -getAngle( 4 );
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );
	
	VectorCopy( boxOrigin, origin );
	origin[2] += boxDim[2] + 0.5;
	renderTextCircle( circleText, origin, axis, 80, 0.9f, s_bg.fontShadowShader );
	origin[2] += 5;
	renderTextCircle( circleText, origin, axis, 80, 0.9f, s_bg.fontShader );

	/* Add some rotating stars */
	angles[PITCH] = 0;
	angles[YAW] = getAngle( 20 );
	angles[ROLL] = 0;
	VectorSet( origin, 50, 50, 50 );

	renderStars();
	trap_R_AddLightToScene( vec3_origin, 1000, 1, 1, 1 );
	trap_R_RenderScene( &refdef );

	if ( dimming ) {
		vec4_t color = { 0, 0, 0, 1.0f - dimming };
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, y, w, h, 0, 0, 0, 0, s_bg.darkenShader );
		trap_R_SetColor( NULL );
	}
}


/*
===============
MainMenu_Cache
===============
*/
void UI_BackGroundCache( void ) {
	int i;
	if (s_bg.cached)
		return;

	s_bg.seed = 0x235123;
	s_bg.cached = qtrue;
	s_bg.starModel = trap_R_RegisterModel( "data/star.md3" );
	s_bg.starShader = trap_R_RegisterShader( "mme/menu/star" );
	s_bg.fontShader = trap_R_RegisterShaderNoMip( "mme/menu/scrollFont" );
	s_bg.fontShadowShader = trap_R_RegisterShader( "mme/menu/scrollFontShadow" );
	s_bg.centerSideShader = trap_R_RegisterShader( "mme/menu/boxCenterSide" );
	s_bg.centerTopShader = trap_R_RegisterShader( "mme/menu/boxCenterTop" );
	s_bg.darkenShader = trap_R_RegisterShader( "mme/menu/darken" );
	s_bg.backgroundShader = trap_R_RegisterShader( "mme/menu/background" );
	s_bg.railRingsShader = trap_R_RegisterShader( "railDisc" );
	s_bg.railCoreShader = trap_R_RegisterShader( "railCore" );
	s_bg.railFireSound = trap_S_RegisterSound( "sound/weapons/railgun/railgf1a.wav", qfalse );
	
	boxCenterShaders[0] = boxCenterShaders[1] =
		boxCenterShaders[2] = boxCenterShaders[3] =
			s_bg.centerSideShader;
	boxCenterShaders[4] = boxCenterShaders[5] =
		s_bg.centerTopShader;

	UI_PlayerInfo_SetModel( &s_bg.player.info, "sarge" );
	s_bg.player.origin[2] = 24;
	
	for(i = 0;i<MAIN_STARS;i++) {
		int j;
		renderStar_t *star = s_bg.stars + i;
		
		star->pos[0] = Q_crandom( &s_bg.seed ) * 180;
		star->pos[1] = Q_crandom( &s_bg.seed ) * 180;
		star->pos[2] = 200 + Q_random( &s_bg.seed ) * 150;
		
		star->velocity[YAW] = 0.001f * globeRand( 12, 30 );
		star->velocity[PITCH] = 0.001f * globeRand( 5, 10 );
		star->velocity[PITCH] = 0.001f * globeRand( 12, 30 );
		

		star->scale = 4 + Q_random( &s_bg.seed ) * 2;
		
		for (j = 0;j < 3;j++) {
			star->rotate[j] = Q_random( &s_bg.seed ) * 360;
			star->spin[j] = 0.001f * globeRand( 50, 50 );
			star->color[j] = colorYellow[j] * 255;
		}
	}
}


