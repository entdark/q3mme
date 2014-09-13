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

#include "tr_local.h"

trScene_t trScene;

void *R_SceneAlloc( int size ) {
	/* Align to multiple of a pointer and allocate from the rear */
	size = (size + (sizeof( void *) - 1)) & ~((sizeof( void *) - 1));
	if ( trScene.backEnd->used + size > trScene.backEnd->size ) {
		ri.Error( ERR_DROP, "RenderScene ran out of storage, increase r_backEndMegs" );
		return 0;
	}
	trScene.backEnd->size -= size;
	return trScene.backEnd->data + trScene.backEnd->size;;
}

void *R_SceneAllocEntity( int size ) {
	srfEntity_t *ent;
	ent = R_SceneAlloc( size );
	if (!ent)
		return 0;
	ent->next = trScene.entities;
	trScene.entities = ent;
	return ent;
}

void *R_SceneAllocModel( int size ) {
	sceneModel_t *ent;
	ent = R_SceneAlloc( size );
	if (!ent)
		return 0;
	ent->next = trScene.models;
	trScene.models = ent;
	return ent;
}

/*
====================
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame( void ) {
	if ( r_smp->integer ) {
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame ^= 1;
	} else {
		tr.smpFrame = 0;
	}
	trScene.backEnd = backEndData[tr.smpFrame];
	trScene.backEnd->used = 0;
	/* Leave enough size for the end of list cmd */
	trScene.backEnd->size = tr.backendSize;
	memset( trScene.backEnd->data, 0, sizeof( commandHeader_t ));
	trScene.refdef = 0;
	RE_ClearScene();
}

/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene( void ) {
	if (trScene.usedRefDef || !trScene.refdef)
		trScene.refdef = R_SceneAlloc( sizeof( trRefdef_t ));
	trScene.refdef->numDlights = 0;
	trScene.models = 0;
	trScene.entities = 0;
	trScene.portals = 0;
	trScene.usedRefDef = qfalse;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

int R_FogFromBounds( const vec3_t bounds[2] ) {
	int fogIndex;
	const fog_t *fog;

	// if no world is loaded
	if ( tr.world == NULL ) 
		return 0;
	// see if it is in a fog volume
	if ( tr.world->numfogs == 1 )
		return 0;
	for ( fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++ ) {
		fog = &tr.world->fogs[fogIndex]; 
		if ( bounds[1][0] >= fog->bounds[0][0]
			&& bounds[1][1] >= fog->bounds[0][1]
			&& bounds[1][2] >= fog->bounds[0][2]
			&& bounds[0][0] <= fog->bounds[1][0]
			&& bounds[0][1] <= fog->bounds[1][1]
			&& bounds[0][2] <= fog->bounds[1][2] ) 
		{
			return fogIndex;
		}
	}
	return 0;
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	srfPoly_t	*poly;
	int			i, j;
	int			allocSize;
	shader_t	*sh;

	if ( !tr.registered ) {
		return;
	}

	sh = R_GetShaderByHandle( hShader );
	if (sh->defaultShader) {
//		ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n");
		return;
	}

	allocSize = sizeof( srfPoly_t ) + numVerts * ( sizeof( vecSimd_t ) + sizeof (vec2_t) + sizeof( color4ub_t ));

	for ( j = 0; j < numPolys; j++ ) {
		vecSimd_t *xyz;
		vec2_t *st;
		color4ub_t *color;

		poly = R_SceneAllocEntity( allocSize );
		if (!poly)
			return;
		poly->surfaceType = SF_POLY;
		poly->numVerts = numVerts;
		poly->sh = sh;

		xyz = ((vecSimd_t *)( poly + 1));
		st = ((vec2_t*)( xyz + numVerts));
		color = (color4ub_t*)( st + numVerts );
		
		VectorCopy( verts->xyz, poly->bounds[0] );
		VectorCopy( verts->xyz, poly->bounds[1] );
		VectorCopy( verts->xyz, xyz[0] );
		st[0][0] = verts->st[0];
		st[0][1] = verts->st[1];
		Byte4Copy( verts->modulate, color[0] );
		verts++;xyz++; st++ ; color++;
		for (i = numVerts -1; i > 0; i--, verts++, xyz++, st++, color++ ) {
			VectorCopy( verts->xyz, xyz[0] );
			AddPointToBounds( verts->xyz, poly->bounds[0], poly->bounds[1] );
			st[0][0] = verts->st[0];
			st[0][1] = verts->st[1];
			Byte4Copy( verts->modulate, color[0] );
		}
		poly->fogNum = R_FogFromBounds( poly->bounds );
	}
}


//=================================================================================


void RE_AddBeamToScene( const refBeam_t *ref  ) {
	srfBeam_t *beam;
	beam = R_SceneAllocEntity( sizeof( srfBeam_t ));

	if (!beam)
		return;
	beam->shader = R_GetShaderByHandle( ref->shader );
	beam->surfaceType = SF_BEAM;
	beam->width = ref->width;
	beam->rotateAngle = ref->rotateAngle;
	beam->renderfx = ref->renderfx;
	beam->shaderTime = ref->shaderTime;
	Byte4Copy( ref->color, beam->color );
	VectorCopy( ref->start, beam->start );
	VectorCopy( ref->end, beam->end );
}

void RE_AddRingsToScene( const refRings_t *ref  ) {
	srfRings_t *rings;
	rings = R_SceneAllocEntity( sizeof( srfRings_t ));

	if (!rings)
		return;

	rings->shader = R_GetShaderByHandle( ref->shader );
	rings->surfaceType = SF_RINGS;
	rings->radius = ref->radius;
	rings->stepSize = ref->stepSize;
	rings->renderfx = ref->renderfx;
	rings->shaderTime = ref->shaderTime;
	Byte4Copy( ref->color, rings->color );
	VectorCopy( ref->start, rings->start );
	VectorCopy( ref->end, rings->end );
}


void RE_AddSpriteToScene( const refSprite_t *ref  ) {
	srfSprite_t *sprite;
	sprite = R_SceneAllocEntity( sizeof( srfSprite_t ));
	if (!sprite)
		return;
	sprite->shader = R_GetShaderByHandle( ref->shader );
	sprite->surfaceType = SF_SPRITE;
	sprite->radius = ref->radius;
	sprite->rotation = ref->rotation;
	sprite->renderfx = ref->renderfx;
	sprite->shaderTime = ref->shaderTime;
	Byte4Copy( ref->color, sprite->color );
	VectorCopy( ref->origin, sprite->origin );
}

void RE_AddModelToScene( const refModel_t *rm ) {
	sceneModel_t *ent;

	ent = R_SceneAllocModel( sizeof( sceneModel_t ));
	if (!ent)
		return;

	ent->renderfx = rm->renderfx;
	ent->model = R_GetModelByHandle( rm->hModel );
	ent->shaderTime = rm->shaderTime;
	ent->customShader = rm->customShader;
	ent->customSkin = rm->customSkin;
	ent->skinNum = rm->skinNum;
	Byte4Copy( rm->shaderRGBA, ent->shaderRGBA );

	ent->frame = rm->frame;
	ent->oldframe = rm->oldframe;
	ent->backlerp = rm->backlerp;

	VectorCopy( rm->origin, ent->origin );
	VectorCopy( rm->axis[0], ent->axis[0] );
	VectorCopy( rm->axis[1], ent->axis[1] );
	VectorCopy( rm->axis[2], ent->axis[2] );
	ent->nonNormalizedAxes = rm->nonNormalizedAxes;
	
	ent->shadowPlane = rm->shadowPlane;
	VectorCopy( rm->lightingOrigin, ent->lightingOrigin );

	// Clear these, will be filled in again later
	ent->lightingCalculated = qfalse;
}
/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene( const refEntity_t *ent ) {
	srfSprite_t *sprite;
	sceneModel_t *model;
	scenePortal_t *portal;
	srfBeam_t *beam;
	srfRings_t *rings;

	if ( !tr.registered ) {
		return;
	}

	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}
	switch ( ent->reType ) {
	case RT_SPRITE:
		sprite = R_SceneAllocEntity( sizeof( srfSprite_t ));
		if (!sprite)
			return;
		sprite->surfaceType = SF_SPRITE;
		sprite->radius = ent->radius;
		sprite->rotation = ent->rotation;
		sprite->shader = R_GetShaderByHandle( ent->customShader );
		sprite->renderfx = ent->renderfx;
		sprite->shaderTime = ent->shaderTime * 1000;
		Byte4Copy( ent->shaderRGBA, sprite->color );
		VectorCopy( ent->origin, sprite->origin );
		break;
	case RT_BEAM:
		beam = R_SceneAllocEntity( sizeof( srfBeam_t ));
		if (!beam)
			return;
		beam->surfaceType = SF_BEAM;
		beam->shader = R_GetShaderByHandle( ent->customShader );
		beam->shaderTime = ent->shaderTime * 1000;
		beam->rotateAngle = 60;
		beam->width = 4;
		beam->renderfx = ent->renderfx;
		Byte4Copy( ent->shaderRGBA, beam->color );
		VectorCopy( ent->origin, beam->start );
		VectorCopy( ent->oldorigin, beam->end );
		break;
	case RT_PORTALSURFACE:
		portal = R_SceneAlloc( sizeof( scenePortal_t ));
		if (!portal)
			return;
		VectorCopy( ent->origin, portal->origin );
		VectorCopy( ent->oldorigin, portal->pvsOrigin );
		VectorCopy( ent->axis[0], portal->axis[0] );
		VectorCopy( ent->axis[1], portal->axis[1] );
		VectorCopy( ent->axis[2], portal->axis[2] );

		portal->rotate = ent->oldframe;
		portal->rotateOffset = ent->skinNum;
		portal->rotateSpeed = ent->frame;
			 
		portal->next = trScene.portals;
		trScene.portals = portal;
		break;
	case RT_RAIL_CORE:
		beam = R_SceneAllocEntity( sizeof( srfBeam_t ));
		if (!beam)
			return;
		beam->surfaceType = SF_BEAM;
		VectorCopy( ent->origin, beam->start );
		VectorCopy( ent->oldorigin, beam->end );
		beam->width = r_railCoreWidth->value;
		beam->rotateAngle = r_railCoreRotate->value;
		beam->shader = R_GetShaderByHandle( ent->customShader );
		beam->renderfx = ent->renderfx;
		beam->shaderTime = ent->shaderTime * 1000;
		Byte4Copy( ent->shaderRGBA, beam->color );
		break;
	case RT_RAIL_RINGS:
		rings = R_SceneAllocEntity( sizeof( srfRings_t ));
		if (!rings)
			return;
		rings->surfaceType = SF_RINGS;
		VectorCopy( ent->origin, rings->start );
		VectorCopy( ent->oldorigin, rings->end );
		rings->radius = r_railWidth->value;
		rings->stepSize = r_railSegmentLength->value;
		rings->shader = R_GetShaderByHandle( ent->customShader );
		rings->renderfx = ent->renderfx;
		rings->shaderTime = ent->shaderTime * 1000;
		Byte4Copy( ent->shaderRGBA, rings->color );
		break;
	case RT_LIGHTNING:
		beam = R_SceneAllocEntity( sizeof( srfBeam_t ));
		if (!beam)
			return;
		beam->surfaceType = SF_BEAM;
		VectorCopy( ent->origin, beam->start );
		VectorCopy( ent->oldorigin, beam->end );
		beam->width = 16;
		beam->rotateAngle = 45;
		beam->shader = R_GetShaderByHandle( ent->customShader );
		beam->renderfx = ent->renderfx;
		beam->shaderTime = ent->shaderTime * 1000;
		Byte4Copy( ent->shaderRGBA, beam->color );
		break;
	case RT_MODEL:
		model = R_SceneAllocModel( sizeof( sceneModel_t ));
		if (!model)
			return;
		model->renderfx = ent->renderfx;
		model->model = R_GetModelByHandle( ent->hModel );
		model->shaderTime = ent->shaderTime * 1000;
		model->customShader = ent->customShader;
		model->customSkin = ent->customSkin;
		model->skinNum = ent->skinNum;
		Byte4Copy( ent->shaderRGBA, model->shaderRGBA );

		model->frame = ent->frame;
		model->oldframe = ent->oldframe;
		model->backlerp = ent->backlerp;

		VectorCopy( ent->origin, model->origin );
		VectorCopy( ent->axis[0], model->axis[0] );
		VectorCopy( ent->axis[1], model->axis[1] );
		VectorCopy( ent->axis[2], model->axis[2] );
		model->nonNormalizedAxes = ent->nonNormalizedAxes;
		
		model->shadowPlane = ent->shadowPlane;
		VectorCopy( ent->lightingOrigin, model->lightingOrigin );

		// Clear these, will be filled in again later
		model->lightingCalculated = qfalse;
		break;
	}
}


/*
=====================
RE_AddDynamicLightToScene

=====================
*/
void RE_AddDynamicLightToScene( const vec3_t org, float intensity, float r, float g, float b, int additive ) {
	dlight_t	*dl;

	if ( !tr.registered ) {
		return;
	}
	if ( trScene.refdef->numDlights >= MAX_REF_LIGHTS ) {
		return;
	}
	if ( intensity <= 0 ) {
		return;
	}
	dl = &trScene.refdef->dlights[ trScene.refdef->numDlights++ ];
	VectorCopy (org, dl->origin);
	dl->radius = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->additive = additive;
}

/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qfalse );
}

/*
=====================
RE_AddAdditiveLightToScene

=====================
*/
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	RE_AddDynamicLightToScene( org, intensity, r, g, b, qtrue );
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
static qboolean timeFractionSet = qfalse;
void RE_RenderScene( const refdef_t *fd ) {
	viewParms_t		parms;
	trRefdef_t		*refdef;
	int				startTime;

	if ( !tr.registered ) {
		return;
	}
	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer ) {
		return;
	}

	startTime = ri.Milliseconds();

	if (!tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		ri.Error (ERR_DROP, "R_RenderScene: NULL worldmodel");
	}
	
	trScene.usedRefDef = qtrue;
	refdef = trScene.refdef;

	refdef->x = fd->x;
	refdef->y = fd->y;
	refdef->width = fd->width;
	refdef->height = fd->height;
	refdef->fov_x = fd->fov_x;
	refdef->fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, refdef->vieworg );
	VectorCopy( fd->viewaxis[0], refdef->viewaxis[0] );
	VectorCopy( fd->viewaxis[1], refdef->viewaxis[1] );
	VectorCopy( fd->viewaxis[2], refdef->viewaxis[2] );

	refdef->time = fd->time;
	if (!timeFractionSet)
		refdef->timeFraction = 0.0f;
	timeFractionSet = qfalse;
	refdef->rdflags = fd->rdflags;

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	refdef->areamaskModified = qfalse;
	if ( ! (refdef->rdflags & RDF_NOWORLDMODEL) ) {
		int		areaDiff;
		int		i;

		// compare the area bits
		areaDiff = 0;
		for (i = 0 ; i < MAX_MAP_AREA_BYTES/4 ; i++) {
			areaDiff |= ((int *)refdef->areamask)[i] ^ ((int *)fd->areamask)[i];
			((int *)refdef->areamask)[i] = ((int *)fd->areamask)[i];
		}

		if ( areaDiff ) {
			// a door just opened or something
			refdef->areamaskModified = qtrue;
		}
	}

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ( r_dynamiclight->integer == 0 || r_vertexLight->integer == 1 || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		refdef->numDlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset( &parms, 0, sizeof( parms ) );
	parms.viewportX = refdef->x;
	parms.viewportY = glConfig.vidHeight - ( refdef->y + refdef->height );
	parms.viewportWidth = refdef->width;
	parms.viewportHeight = refdef->height;
	parms.isPortal = qfalse;

	parms.fovX = refdef->fov_x;
	parms.fovY = refdef->fov_y;

	VectorCopy( refdef->vieworg, parms.or.origin );
	VectorCopy( refdef->viewaxis[0], parms.or.axis[0] );
	VectorCopy( refdef->viewaxis[1], parms.or.axis[1] );
	VectorCopy( refdef->viewaxis[2], parms.or.axis[2] );
	VectorCopy( parms.or.origin, parms.pvsOrigin );

	R_RenderView( &parms );

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}

void R_MME_TimeFraction(float timeFraction) {
	trScene.refdef->timeFraction = timeFraction;
	timeFractionSet = qtrue;
}
