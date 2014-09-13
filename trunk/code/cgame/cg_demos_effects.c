/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

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

#include "cg_demos.h"

#define SPEED_SHIFT 14

static demoEffectPoint_t *demoEffectPointAlloc(void) {
	demoEffectPoint_t * point = (demoEffectPoint_t *)malloc(sizeof (demoEffectPoint_t));
	if (!point)
		CG_Error("Out of memory");
	memset( point, 0, sizeof( demoEffectPoint_t ));
	return point;
}

static void demoEffectPointFree( demoEffectParent_t*parent, demoEffectPoint_t *point ) {
	if (point->prev) 
		point->prev->next = point->next;
	else 
		parent->points = point->next;
	if (point->next ) 
		point->next->prev = point->prev;
	free( point );
}

demoEffectPoint_t *demoEffectPointSynch( demoEffectParent_t*parent, int time ) {
	demoEffectPoint_t *point;
	if ( !parent || !parent->points )
		return 0;
	
	point = parent->points;
	for( ;point->next && point->next->time <= time; point = point->next);
	return point;
}

void demoEffectPointMatch( const demoEffectPoint_t *point, int mask, const demoEffectPoint_t *match[4] ) {
	const demoEffectPoint_t *p;

	p = point;
	while (p && !(p->flags & mask))
		p = p->prev;

	if (!p) {
		match[0] = 0;
		match[1] = 0;
		p = point;
		while (p && !(p->flags & mask))
			p = p->next;
		if (!p) {
			match[2] = 0;
			match[3] = 0;
			return;
		}
	} else {
		match[1] = p;
		p = p->prev;
		while (p && !(p->flags & mask))
			p = p->prev;
		match[0] = p;
	}
	p = point->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}

void demoEffectMatchAt( const demoEffectPoint_t *p, int time, int mask, const demoEffectPoint_t *match[4] ) {
	match[0] = match[1] = 0;
	while( p ) {
		if (p->time > time)
			break;
		if (p->flags & mask) {
			match[0] = match[1];
			match[1] = p;
		}
		p = p->next;
	}
	while (p && !(p->flags & mask))
		p = p->next;
	match[2] = p;
	if (p)
		p = p->next;
	while (p && !(p->flags & mask))
		p = p->next;
	match[3] = p;
}


static demoEffectPoint_t *demoEffectPointAdd( demoEffectParent_t *parent, int time ) {
	demoEffectPoint_t *point;
	demoEffectPoint_t *newPoint;
	if ( !parent )
		return 0;

	point = demoEffectPointSynch( parent, time );
	if (!point || point->time > time) {
		newPoint = demoEffectPointAlloc();
		newPoint->next = parent->points;
		if (parent->points)
			parent->points->prev = newPoint;
		parent->points = newPoint;
		newPoint->prev = 0;
	} else if (point->time == time) {
		newPoint = point;
	} else {
		newPoint = demoEffectPointAlloc();
		newPoint->prev = point;
		newPoint->next = point->next;
		if (point->next)
			point->next->prev = newPoint;
		point->next = newPoint;
	}
	newPoint->time = time;
	return newPoint;
}

static qboolean demoEffectPointDel( demoEffectParent_t *parent, int playTime ) {
	demoEffectPoint_t *point = demoEffectPointSynch( parent, playTime );

	if (!point || !point->time == playTime || !parent )
		return qfalse;
	demoEffectPointFree( parent, point );
	return qtrue;
}

demoEffectParent_t *demoEffectParentCreate( void ) {
	demoEffectParent_t *parent;
	parent = (demoEffectParent_t *) malloc( sizeof( demoEffectParent_t ));
	if (!parent)
		CG_Error("Out of memory");
	memset( parent, 0, sizeof( *parent ));

	parent->next = demo.effect.list;
	if ( demo.effect.list )
		demo.effect.list->prev = parent;
	demo.effect.list = parent;
	parent->active = qtrue;
	parent->flags = EFFECT_ORIGIN | EFFECT_ANGLES | EFFECT_SIZE | EFFECT_COLOR;
	Vector4Set( parent->color, 1, 1, 1, 1 );
	parent->size = 1.0f;
	return parent;
}

demoEffectParent_t *demoEffectParentClear( demoEffectParent_t *parent ) {
	demoEffectParent_t *ret;
	demoEffectPoint_t *next, *point;

	if (!parent)
		return 0;
	point = parent->points;
	while ( point ) {
		next = point->next;
		demoEffectPointFree( parent, point );
		point = next;
	}
	ret = parent->next;
	if (!ret)
		ret = parent->prev;

	if (parent->prev) 
		parent->prev->next = parent->next;
	else 
		demo.effect.list = parent->next;
	if (parent->next ) 
		parent->next->prev = parent->prev;
	free( parent );
	return ret;
}

typedef struct {
	int		time;
	int		flags;
	vec3_t	origin, angles;
	vec4_t	color;
	float	size;
} parseEffectPoint_t;

static qboolean demoEffectParsePointTime( BG_XMLParse_t *parse,const char *script, void *data) {
	parseEffectPoint_t *point = (parseEffectPoint_t *)data;
	if (!script[0]) 
		return qfalse;
	point->time = atoi( script );
	return qtrue;
}
static qboolean demoEffectParsePointAngles( BG_XMLParse_t *parse,const char *line, void *data) {
	parseEffectPoint_t *point = (parseEffectPoint_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &point->angles[0], &point->angles[1], &point->angles[2] );
	point->flags |= EFFECT_ANGLES;
	return qtrue;
}
static qboolean demoEffectParsePointOrigin( BG_XMLParse_t *parse,const char *line, void *data) {
	parseEffectPoint_t *point = (parseEffectPoint_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &point->origin[0], &point->origin[1], &point->origin[2] );
	point->flags |= EFFECT_ORIGIN;
	return qtrue;
}
static qboolean demoEffectParsePointColor( BG_XMLParse_t *parse,const char *line, void *data) {
	parseEffectPoint_t *point = (parseEffectPoint_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f %f", &point->color[0], &point->color[1], &point->color[2], &point->color[3] );
	point->flags |= EFFECT_COLOR;
	return qtrue;
}
static qboolean demoEffectParsePointSize( BG_XMLParse_t *parse,const char *script, void *data) {
	parseEffectPoint_t *point = (parseEffectPoint_t *)data;
	if (!script[0]) 
		return qfalse;
	point->flags |= EFFECT_SIZE;
	point->size = atof( script );
	return qtrue;
}

static qboolean demoEffectParsePoint( BG_XMLParse_t *parse,const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	demoEffectParent_t *parent;
	demoEffectPoint_t *point;
	parseEffectPoint_t pointLoad;

	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"time", 0, demoEffectParsePointTime},
		{"origin", 0, demoEffectParsePointOrigin},
		{"angles", 0, demoEffectParsePointAngles},
		{"color", 0, demoEffectParsePointColor},
		{"size", 0, demoEffectParsePointSize},
		{0, 0, 0}
	};
	memset( &pointLoad, 0, sizeof( pointLoad ));
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, &pointLoad )) {
		return qfalse;
	}
	parent = (demoEffectParent_t *)data;
	point = demoEffectPointAdd( parent, pointLoad.time );
	if (!point)
		return qfalse;
	VectorCopy( pointLoad.angles, point->angles );
	VectorCopy( pointLoad.origin, point->origin );
	Vector4Copy( pointLoad.color, point->color );
	point->flags = pointLoad.flags;
	return qtrue;
}

static qboolean demoEffectParseParentAngles( BG_XMLParse_t *parse,const char *line, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &parent->angles[0], &parent->angles[1], &parent->angles[2] );
	return qtrue;
}
static qboolean demoEffectParseParentOrigin( BG_XMLParse_t *parse,const char *line, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f", &parent->origin[0], &parent->origin[1], &parent->origin[2] );
	return qtrue;
}
static qboolean demoEffectParseParentColor( BG_XMLParse_t *parse,const char *line, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!line[0]) 
		return qfalse;
	sscanf( line, "%f %f %f %f", &parent->color[0], &parent->color[1], &parent->color[2], &parent->color[3] );
	return qtrue;
}
static qboolean demoEffectParseParentSize( BG_XMLParse_t *parse,const char *script, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!script[0]) 
		return qfalse;
	parent->size = atof( script );
	return qtrue;
}
static qboolean demoEffectParseParentLocked( BG_XMLParse_t *parse,const char *script, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!script[0]) 
		return qfalse;
	parent->locked = atoi( script );
	return qtrue;
}
static qboolean demoEffectParseParentScript( BG_XMLParse_t *parse,const char *script, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!script[0]) 
		return qfalse;
	Q_strncpyz( parent->scriptName, script, sizeof( parent->scriptName ));
	return qtrue;
}
static qboolean demoEffectParseParentShader( BG_XMLParse_t *parse,const char *shader, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!shader[0]) 
		return qfalse;
	Q_strncpyz( parent->shaderName, shader, sizeof( parent->shaderName ));
	return qtrue;
}
static qboolean demoEffectParseParentModel( BG_XMLParse_t *parse,const char *model, void *data) {
	demoEffectParent_t *parent = (demoEffectParent_t *)data;
	if (!model[0]) 
		return qfalse;
	Q_strncpyz( parent->modelName, model, sizeof( parent->modelName ));
	return qtrue;
}


static qboolean demoEffectParseParent( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	demoEffectParent_t *parent;
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"point",	demoEffectParsePoint,	0},
		{"origin", 0, demoEffectParseParentOrigin},
		{"angles", 0, demoEffectParseParentAngles},
		{"color", 0, demoEffectParseParentColor},
		{"size", 0, demoEffectParseParentSize},
		{"script", 0, demoEffectParseParentScript},
		{"shader", 0, demoEffectParseParentShader},
		{"model", 0, demoEffectParseParentModel},
		{0, 0, 0}
	};
	parent = demoEffectParentCreate();
	if (!parent)
		return BG_XMLError( parse, "Can't allocate parent" );
	data = parent;
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, data ))
		return qfalse;
	return qtrue;
}

static qboolean demoEffectParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t scriptParseBlock[] = {
		{"parent",	demoEffectParsePoint,	0},
		{0, 0, 0}
	};
	if (!BG_XMLParse( parse, fromBlock, scriptParseBlock, data ))
		return qfalse;
	return qtrue;
}


void demoEffectSave( fileHandle_t fileHandle ) {
	demoEffectPoint_t *point = 0;
	demoEffectParent_t *parent;

	demoSaveLine( fileHandle, "<effect>\n" );
	parent = demo.effect.list;
	while (parent) {
		demoEffectPoint_t *point = parent->points;

		demoSaveLine( fileHandle, "\t<parent>\n");
		demoSaveLine( fileHandle, "\t\t<locked>%d</locked>\n", parent->locked );
		demoSaveLine( fileHandle, "\t\t<origin>%9.4f %9.4f %9.4f</origin>\n", parent->origin[0], parent->origin[1], parent->origin[2] );
		demoSaveLine( fileHandle, "\t\t<angles>%9.4f %9.4f %9.4f</angles>\n", parent->angles[0], parent->angles[1], parent->angles[2] );
		demoSaveLine( fileHandle, "\t\t\t<color>%9.4f %9.4f %9.4f</color>\n", parent->color[0], parent->color[1], parent->color[2], parent->color[3] );
		demoSaveLine( fileHandle, "\t\t<size>%9.4f</size>\n", parent->size );
		if ( parent->scriptName[0] ) 
			demoSaveLine( fileHandle, "\t\t<script>%s</script>\n", parent->scriptName );
		if ( parent->shaderName[0] ) 
			demoSaveLine( fileHandle, "\t\t<shader>%s</script>\n", parent->shaderName );
		if ( parent->modelName[0] ) 
			demoSaveLine( fileHandle, "\t\t<model>%s</model>\n", parent->modelName );
		while ( point ) {
			demoSaveLine( fileHandle, "\t\t<point>\n");
			demoSaveLine( fileHandle, "\t\t\t<time>%10d</time>\n", point->time );
			if ( point->flags & EFFECT_ORIGIN )
				demoSaveLine( fileHandle, "\t\t\t<origin>%9.4f %9.4f %9.4f</origin>\n", point->origin[0], point->origin[1], point->origin[2] );
			if ( point->flags & EFFECT_ANGLES )
				demoSaveLine( fileHandle, "\t\t\t<angles>%9.4f %9.4f %9.4f</angles>\n", point->angles[0], point->angles[1], point->angles[2] );
			if ( point->flags & EFFECT_SIZE )
				demoSaveLine( fileHandle, "\t\t\t<size>%9.4f</size>\n", point->size );
			if ( point->flags & EFFECT_COLOR )
				demoSaveLine( fileHandle, "\t\t\t<color>%9.4f %9.4f %9.4f</color>\n", point->color[0], point->color[1], point->color[2], point->color[3] );
			demoSaveLine( fileHandle, "\t\t</point>\n");
			point = point->next;
		}
		demoSaveLine( fileHandle, "<\t/parent>\n");
		parent = parent->next;
	}
	demoSaveLine( fileHandle, "</effect>\n" );
}

static void demoEffectMatchTimes( const demoEffectPoint_t *match[4], int times[4] ) {
	times[1] = match[1]->time;
	times[2] = match[2]->time;
	if (match[0]) {
		times[0] = match[0]->time;
	} else {
		times[0] = times[1] - (times[2] - times[1]);
	}
	if (match[3]) {
		times[3] = match[3]->time;
	} else {
		times[3] = times[2] + (times[2] - times[1]);
	}
}

static void demoEffectMatchOrigin( const demoEffectPoint_t *match[4], vec3_t origins[4] ) {
	VectorCopy( match[1]->origin, origins[1] );
	VectorCopy( match[2]->origin, origins[2] );
	if (match[0]) {
		VectorCopy( match[0]->origin, origins[0] );
	} else {
		VectorSubDelta( origins[1], origins[2], origins[0] );
	}
	if (match[3]) {
		VectorCopy( match[3]->origin, origins[3] );
	} else {
		VectorAddDelta( origins[1], origins[2], origins[3] );
	}
}

static void demoEffectOriginAt( demoEffectParent_t *parent, int time, float timeFraction, vec3_t origin ) {
	demoEffectPoint_t *match[4];
	vec3_t origins[4];
	int	times[4];
	float lerp;

	demoEffectMatchAt( parent->points, time, EFFECT_ORIGIN, match );
	if (!match[1] ) {
		if ( match[0] ) {
			VectorCopy( match[0]->origin, origin );
		} else if ( match[2] ) {
			VectorCopy( match[2]->origin, origin );
		} else {
			VectorCopy( parent->origin, origin );
		}
		return;
	} else if (!match[2] ) {
		VectorCopy( match[1]->origin, origin );
		return;
	} 
	demoEffectMatchOrigin( match, origins );
	demoEffectMatchTimes( match, times );
	lerp = ((time - times[1]) + timeFraction) / (times[2] - times[1]);
	VectorTimeSpline( lerp, times, origins[0], origin, 3 );
}

static void demoEffectSizeAt( demoEffectParent_t *parent, int time, float timeFraction, float *size ) {
	demoEffectPoint_t *match[4];
	float sizes[4];
	int	times[4];
	float lerp;

	demoEffectMatchAt( parent->points, time, EFFECT_SIZE, match );
	if (!match[1] ) {
		if ( match[0] ) {
			size[0] = match[0]->size;
		} else if ( match[2] ) {
			size[0] = match[2]->size;
		} else {
			size[0] = parent->size;;
		}
		return;
	} else if (!match[2] ) {
		size[0] = match[1]->size;
		return;
	} 
	sizes[1] = match[1]->size;
	sizes[2] = match[2]->size;
	if (match[0]) {
		sizes[0] = match[0]->size;
	} else {
		sizes[0] = sizes[1] - (sizes[2] - sizes[1]);
	}
	if (match[3]) {
		sizes[3] = match[3]->size;
	} else {
		sizes[3] = sizes[2] + (sizes[2] - sizes[1]);
	}
	demoEffectMatchTimes( match, times );
	lerp = ((time - times[1]) + timeFraction) / (times[2] - times[1]);
	VectorTimeSpline( lerp, times, sizes, size, 1 );
}


static void demoEffectAnglesAt( demoEffectParent_t *parent, int time, float timeFraction, vec3_t angles ) {
	demoEffectPoint_t *match[4];
	Quat_t quats[4], qr;
	int	times[4];
	vec3_t tempAngles;
	float lerp;

	demoEffectMatchAt( parent->points, time, EFFECT_ANGLES, match );
	if (!match[1] ) {
		if ( match[0] ) {
			VectorCopy( match[0]->angles, angles );
		} else if ( match[2] ) {
			VectorCopy( match[2]->angles, angles );
		} else {
			VectorCopy( parent->angles, angles );
		}
		return;
	} else if (!match[2] ) {
		VectorCopy( match[1]->angles, angles );
		return;
	}
	QuatFromAngles( match[1]->angles, quats[1] );
	QuatFromAnglesClosest( match[2]->angles, quats[1], quats[2] );
	if (!match[0]) {
		VectorSubtract( match[2]->angles, match[1]->angles, tempAngles );
		VectorAdd( tempAngles, match[1]->angles, tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[1], quats[0] );
	} else {
		QuatFromAnglesClosest( match[0]->angles, quats[1], quats[0] );
	}
	if (!match[3]) {
		VectorSubtract( match[2]->angles, match[1]->angles, tempAngles );
		VectorAdd( tempAngles, match[2]->angles, tempAngles );
		QuatFromAnglesClosest( tempAngles, quats[2], quats[3] );
	} else {
		QuatFromAnglesClosest( match[3]->angles, quats[2], quats[3] );
	}
	demoEffectMatchTimes( match, times );
	lerp = ((time - times[1]) + timeFraction) / (times[2] - times[1]);
	QuatTimeSpline( lerp, times, quats, qr );
	QuatToAngles( qr, angles );
}


static void demoEffectDrawPath( demoEffectPoint_t *point, const vec4_t color) {
	int i;
	polyVert_t verts[4];
	qboolean skipFirst = qtrue;
	vec3_t origin, lastOrigin; 
	demoEffectPoint_t *match[4];
	vec3_t control[4];
	float lerp, lerpFactor;
	int len, steps;
	int times[4] = {0, 1, 2, 3};

	demoEffectPointMatch( point, CAM_ORIGIN, match );
	if (match[1] != point)
		return;
	if (!match[2])
		return;
	demoEffectMatchOrigin( match, control );
	
	len = VectorDistance( control[1], control[2] );
	steps = len / 25;
	if (steps < 5) 
		steps = 5;
	demoDrawSetupVerts( verts, color );
	lerpFactor = 0.9999f / steps;

	for ( i = 0; i <= steps; i++) {
		lerp = i * lerpFactor;
		VectorTimeSpline( lerp, times, control[0], origin, 3);
		if (skipFirst) {
			skipFirst = qfalse;
			VectorCopy( origin, lastOrigin );
			continue;
		}
		demoDrawRawLine( origin, lastOrigin, 1, verts );
		VectorCopy( origin, lastOrigin);
	}
}


void demoEffectDraw( int time, float timeFraction ) {
	demoEffectParent_t *parent;
	parent = demo.effect.list;

	for (; parent; parent = parent->next ) {
		demoEffectPoint_t *point;
		vec3_t origin, angles, dir;
		float size;
		
		if ( demo.viewType != viewEffect || (!parent->locked && parent->points ) || parent != demo.effect.active ) {
			vec3_t end;
			demoEffectOriginAt( parent, time, timeFraction, origin );
			demoEffectAnglesAt( parent, time, timeFraction, angles );
			demoEffectSizeAt( parent, time, timeFraction, &size );
			
			AngleForward( angles, dir );
			demoDrawCross( origin, colorWhite );
			VectorMA( origin, 250, dir, end );
			demoDrawLine( origin, end, colorYellow );
		}
		point = demoEffectPointSynch( parent, time - 10000 );
		while ( point && point->time < time + 100000 ) {
			if ( point->flags & EFFECT_ORIGIN ) 
				demoDrawCross( point->origin, colorRed );
			demoEffectDrawPath( point, colorRed );
			point = point->next;
		}
	}
}

void demoEffectUpdate( int time, float timeFraction ) {
	demoEffectParent_t *parent;
	parent = demo.effect.list;

	for (; parent; parent = parent->next ) {
		fxParent_t	fxParent;
		vec4_t	color;

		if (!parent->active )
			continue;

		color[0] = color[1] = color[2] = color[3] = 1.0f;
		if ( parent->locked ) {
			demoEffectOriginAt( parent, time, timeFraction, fxParent.origin );
			demoEffectAnglesAt( parent, time, timeFraction, fxParent.angles );
			demoEffectSizeAt( parent, time, timeFraction, &fxParent.size );
		} else {
			VectorCopy( parent->origin, fxParent.origin );
			VectorCopy( parent->angles, fxParent.angles );
			Vector4Copy( parent->color, color );
			fxParent.size = parent->size;
		}
		color[0] = color[1] = color[2] = color[3] = 1.0f;
		AngleForward( fxParent.angles, fxParent.dir );
		fxParent.shader = parent->shader;
		fxParent.model = parent->model;
		fxParent.color[0] = color[0] * 255;
		fxParent.color[1] = color[1] * 255;
		fxParent.color[2] = color[2] * 255;
		fxParent.color[3] = color[3] * 255;
		fxParent.life = (time - parent->points->time + timeFraction) * 0.001f; 
		fxParent.flags = FXP_ORIGIN | FXP_ANGLES | FXP_DIR | FXP_SIZE | FXP_SHADER | FXP_COLOR | FXP_MODEL | FXP_LIFE;
		trap_FX_Run( parent->script, &fxParent, parent );
	}
}

void demoEffectMove( void ) {
	float *origin, *angles, *size, *color;
	demoEffectParent_t *parent;
	if (!demo.effect.active)
		return;

	parent = demo.effect.active;
	if (!parent->locked ) {
		origin = parent->origin;
		angles = parent->angles;
		color = parent->color;
		size = &parent->size;
	} else {
		demoEffectPoint_t * point = demoEffectPointSynch( parent, demo.play.time );
		if ( !point || point->time != demo.play.time || demo.play.fraction) 
			return;
		origin = point->origin;
		angles = point->angles;
		color = point->color;
		size = &point->size;
	}

	if (demo.cmd.buttons & BUTTON_ATTACK) {
		vec3_t moveAngles;
		/* First clear some related values */
		if (!(demo.oldcmd.buttons & BUTTON_ATTACK)) {
			VectorClear( parent->velocity );
		}
		if (demo.cmd.upmove > 0) {
			angles[ROLL] -= demo.cmd.angles[YAW];
			AnglesNormalize180( angles );
			return;
		}
		VectorAdd( angles, demo.cmdDeltaAngles, angles );
		AnglesNormalize180( angles );
		VectorCopy( angles, moveAngles );
		moveAngles[ROLL] = 0;
		demoMovePoint( origin, parent->velocity, moveAngles );
	} else if (demo.cmd.upmove > 0) {
		*size -= demo.serverDeltaTime * 700 * demo.cmdDeltaAngles[YAW];
		if (*size < 0)
			*size = 0;
	} else if ( (demo.cmd.upmove < 0) || (demo.cmd.forwardmove < 0) || (demo.cmd.rightmove < 0)  ) {
		vec3_t moveAngles;
		VectorCopy( angles, moveAngles );
		moveAngles[ROLL] = 0;
		demoMoveDirection( origin, moveAngles );
	}

}

void demoEffectCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		if (!demo.effect.active) {
			CG_DemosAddLog("No effects Created");
			return;
		}
		demo.effect.active->locked = !demo.effect.active->locked;
		if (demo.effect.active->locked) 
			CG_DemosAddLog("Effect Group locked");
		else 
			CG_DemosAddLog("Effect Group unlocked");
	} else if (!Q_stricmp(cmd, "add")) {
		demoEffectPoint_t *point = demoEffectPointAdd( demo.effect.active, demo.play.time );
		if ( point ) {
			VectorCopy( demo.viewOrigin, point->origin );
			VectorCopy( demo.viewAngles, point->angles );
			point->size = demo.effect.active->size;
			Vector4Copy( demo.effect.active->color, point->color );
			point->flags = demo.effect.active->flags;
			CG_DemosAddLog("Added effect point");
		} else {
			CG_DemosAddLog("Failed to add effect point");
		}
	} else if (!Q_stricmp(cmd, "del")) {
		if (demoEffectPointDel( demo.effect.active, demo.play.time)) {
			CG_DemosAddLog("Deleted effect point");
		} else {
			CG_DemosAddLog("Failed to delete effect point");
		}
	} else if (!Q_stricmp(cmd, "create")) {
		demo.effect.active = demoEffectParentCreate();
		CG_DemosAddLog("Added effect group");
	} else if (!Q_stricmp(cmd, "clear") || !Q_stricmp(cmd, "destory") ) {
		demo.effect.active = demoEffectParentClear( demo.effect.active );
	} else if (!Q_stricmp(cmd, "next")) {
		demoEffectPoint_t *point;
		if ( demo.cmd.upmove > 0 ) {
			if ( demo.effect.active )
				demo.effect.active = demo.effect.active->next;
			if ( !demo.effect.active )
				demo.effect.active = demo.effect.list;
			return;
		}
		point = demoEffectPointSynch( demo.effect.active, demo.play.time );
		if (!point)
			return;
		if (point->next)
			point = point->next;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	} else if (!Q_stricmp(cmd, "prev")) {
		demoEffectPoint_t *point;
		if ( demo.cmd.upmove > 0 ) {
			if ( demo.effect.active )
				demo.effect.active = demo.effect.active->prev;
			if ( !demo.effect.active ) {
				demo.effect.active = demo.effect.list;
				while ( demo.effect.active && demo.effect.active->next )
					demo.effect.active = demo.effect.active->next;
			}
			return;
		}
		point = demoEffectPointSynch( demo.effect.active, demo.play.time );
		if (!point)
			return;
		if (point->prev)
			point = point->prev;
		demo.play.time = point->time;
		demo.play.fraction = 0;
	}
}
