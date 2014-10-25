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
// tr_main.c -- main control flow for each frame

#include "tr_local.h"

#include <string.h> // memcpy

trGlobals_t		tr;

static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


refimport_t	ri;
/*
=================
R_CullLocalBox

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLocalBox ( const vec3_t bounds[2]) {
	int		i, j;
	vec3_t	transformed[8];
	float	dists[8];
	vec3_t	v;
	cplane_t	*frust;
	int			anyBack;
	int			front, back;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// transform into world space
	for (i = 0 ; i < 8 ; i++) {
		v[0] = bounds[i&1][0];
		v[1] = bounds[(i>>1)&1][1];
		v[2] = bounds[(i>>2)&1][2];

		VectorCopy( tr.or.origin, transformed[i] );
		VectorMA( transformed[i], v[0], tr.or.axis[0], transformed[i] );
		VectorMA( transformed[i], v[1], tr.or.axis[1], transformed[i] );
		VectorMA( transformed[i], v[2], tr.or.axis[2], transformed[i] );
	}

	// check against frustum planes
	anyBack = 0;
	for (i = 0 ; i < 4 ; i++) {
		frust = &tr.viewParms.frustum[i];

		front = back = 0;
		for (j = 0 ; j < 8 ; j++) {
			dists[j] = DotProduct(transformed[j], frust->normal);
			if ( dists[j] > frust->dist ) {
				front = 1;
				if ( back ) {
					break;		// a point is in front
				}
			} else {
				back = 1;
			}
		}
		if ( !front ) {
			// all points were behind one of the planes
			return CULL_OUT;
		}
		anyBack |= back;
	}

	if ( !anyBack ) {
		return CULL_IN;		// completely inside frustum
	}

	return CULL_CLIP;		// partially clipped
}

/*
** R_CullLocalPointAndRadius
*/
int R_CullLocalPointAndRadius( const vec3_t pt, float radius )
{
	vec3_t transformed;

	R_LocalPointToWorld( pt, transformed );

	return R_CullPointAndRadius( transformed, radius );
}

/*
** R_CullPointAndRadius
*/
int R_CullPointAndRadius( const vec3_t pt, float radius )
{
	int		i;
	float	dist;
	cplane_t	*frust;
	qboolean mightBeClipped = qfalse;

	if ( r_nocull->integer ) {
		return CULL_CLIP;
	}

	// check against frustum planes
	for (i = 0 ; i < 4 ; i++) 
	{
		frust = &tr.viewParms.frustum[i];

		dist = DotProduct( pt, frust->normal) - frust->dist;
		if ( dist < -radius )
		{
			return CULL_OUT;
		}
		else if ( dist <= radius ) 
		{
			mightBeClipped = qtrue;
		}
	}

	if ( mightBeClipped )
	{
		return CULL_CLIP;
	}

	return CULL_IN;		// completely inside frustum
}


/*
=================
R_LocalNormalToWorld

=================
*/
void R_LocalNormalToWorld ( const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] * tr.or.axis[2][0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] * tr.or.axis[2][1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] * tr.or.axis[2][2];
}

/*
=================
R_LocalPointToWorld

=================
*/
void R_LocalPointToWorld ( const vec3_t local, vec3_t world) {
	world[0] = local[0] * tr.or.axis[0][0] + local[1] * tr.or.axis[1][0] + local[2] * tr.or.axis[2][0] + tr.or.origin[0];
	world[1] = local[0] * tr.or.axis[0][1] + local[1] * tr.or.axis[1][1] + local[2] * tr.or.axis[2][1] + tr.or.origin[1];
	world[2] = local[0] * tr.or.axis[0][2] + local[1] * tr.or.axis[1][2] + local[2] * tr.or.axis[2][2] + tr.or.origin[2];
}

/*
=================
R_WorldToLocal

=================
*/
void R_WorldToLocal (vec3_t world, vec3_t local) {
	local[0] = DotProduct(world, tr.or.axis[0]);
	local[1] = DotProduct(world, tr.or.axis[1]);
	local[2] = DotProduct(world, tr.or.axis[2]);
}

/*
==========================
R_TransformModelToClip

==========================
*/
void R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst ) {
	int i;

	for ( i = 0 ; i < 4 ; i++ ) {
		eye[i] = 
			src[0] * modelMatrix[ i + 0 * 4 ] +
			src[1] * modelMatrix[ i + 1 * 4 ] +
			src[2] * modelMatrix[ i + 2 * 4 ] +
			1 * modelMatrix[ i + 3 * 4 ];
	}

	for ( i = 0 ; i < 4 ; i++ ) {
		dst[i] = 
			eye[0] * projectionMatrix[ i + 0 * 4 ] +
			eye[1] * projectionMatrix[ i + 1 * 4 ] +
			eye[2] * projectionMatrix[ i + 2 * 4 ] +
			eye[3] * projectionMatrix[ i + 3 * 4 ];
	}
}

/*
==========================
R_TransformClipToWindow

==========================
*/
void R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window ) {
	normalized[0] = clip[0] / clip[3];
	normalized[1] = clip[1] / clip[3];
	normalized[2] = ( clip[2] + clip[3] ) / ( 2 * clip[3] );

	window[0] = 0.5f * ( 1.0f + normalized[0] ) * view->viewportWidth;
	window[1] = 0.5f * ( 1.0f + normalized[1] ) * view->viewportHeight;
	window[2] = normalized[2];

	window[0] = (int) ( window[0] + 0.5 );
	window[1] = (int) ( window[1] + 0.5 );
}


/*
==========================
myGlMultMatrix

==========================
*/
void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

/*
=================
R_RotateForEntity

Generates an orientation for an entity and viewParms
Does NOT produce any GL calls
Called by both the front end and the back end
=================
*/
void R_RotateForEntity( const sceneModel_t *ent, const viewParms_t *viewParms,
					   orientationr_t *or ) 
{
	float	glMatrix[16];
	vec3_t	delta;
	float	axisLength;

	VectorCopy( ent->origin, or->origin );

	VectorCopy( ent->axis[0], or->axis[0] );
	VectorCopy( ent->axis[1], or->axis[1] );
	VectorCopy( ent->axis[2], or->axis[2] );

	glMatrix[0] = or->axis[0][0];
	glMatrix[4] = or->axis[1][0];
	glMatrix[8] = or->axis[2][0];
	glMatrix[12] = or->origin[0];

	glMatrix[1] = or->axis[0][1];
	glMatrix[5] = or->axis[1][1];
	glMatrix[9] = or->axis[2][1];
	glMatrix[13] = or->origin[1];

	glMatrix[2] = or->axis[0][2];
	glMatrix[6] = or->axis[1][2];
	glMatrix[10] = or->axis[2][2];
	glMatrix[14] = or->origin[2];

	glMatrix[3] = 0;
	glMatrix[7] = 0;
	glMatrix[11] = 0;
	glMatrix[15] = 1;

	myGlMultMatrix( glMatrix, viewParms->world.modelMatrix, or->modelMatrix );

	// calculate the viewer origin in the model's space
	// needed for fog, specular, and environment mapping
	VectorSubtract( viewParms->or.origin, or->origin, delta );

	// compensate for scale in the axes if necessary
	if ( ent->nonNormalizedAxes ) {
		axisLength = VectorLength( ent->axis[0] );
		if ( !axisLength ) {
			axisLength = 0;
		} else {
			axisLength = 1.0f / axisLength;
		}
	} else {
		axisLength = 1.0f;
	}

	or->viewOrigin[0] = DotProduct( delta, or->axis[0] ) * axisLength;
	or->viewOrigin[1] = DotProduct( delta, or->axis[1] ) * axisLength;
	or->viewOrigin[2] = DotProduct( delta, or->axis[2] ) * axisLength;
}

/*
=================
R_RotateForViewer

Sets up the modelview matrix for a given viewParm
=================
*/
void R_RotateForWorld ( const orientationr_t* input, orientationr_t* world ) 
{
	float	viewerMatrix[16];
	const float	*origin = input->origin;

	Com_Memset ( world, 0, sizeof(*world));
	world->axis[0][0] = 1;
	world->axis[1][1] = 1;
	world->axis[2][2] = 1;

	// transform by the camera placement
	VectorCopy( origin, world->viewOrigin );
//	VectorCopy( origin, world->viewOrigin );

	viewerMatrix[0] = input->axis[0][0];
	viewerMatrix[4] = input->axis[0][1];
	viewerMatrix[8] = input->axis[0][2];
	viewerMatrix[12] = -origin[0] * viewerMatrix[0] + -origin[1] * viewerMatrix[4] + -origin[2] * viewerMatrix[8];

	viewerMatrix[1] = input->axis[1][0];
	viewerMatrix[5] = input->axis[1][1];
	viewerMatrix[9] = input->axis[1][2];
	viewerMatrix[13] = -origin[0] * viewerMatrix[1] + -origin[1] * viewerMatrix[5] + -origin[2] * viewerMatrix[9];

	viewerMatrix[2] = input->axis[2][0];
	viewerMatrix[6] = input->axis[2][1];
	viewerMatrix[10] = input->axis[2][2];
	viewerMatrix[14] = -origin[0] * viewerMatrix[2] + -origin[1] * viewerMatrix[6] + -origin[2] * viewerMatrix[10];

	viewerMatrix[3] = 0;
	viewerMatrix[7] = 0;
	viewerMatrix[11] = 0;
	viewerMatrix[15] = 1;

	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	myGlMultMatrix( viewerMatrix, s_flipMatrix, world->modelMatrix );

}

void R_RotateForViewer ( ) {
	R_RotateForWorld( &tr.viewParms.or, &tr.or );
	tr.viewParms.world = tr.or;
}




/*
** SetFarClip
*/
static void SetFarClip( void )
{
	float	farthestCornerDistance = 0;
	int		i;

	// if not rendering the world (icons, menus, etc)
	// set a 2k far clip plane
	if ( trScene.refdef->rdflags & RDF_NOWORLDMODEL ) {
		tr.viewParms.zFar = 2048;
		return;
	}

	//
	// set far clipping planes dynamically
	//
	farthestCornerDistance = 0;
	for ( i = 0; i < 8; i++ )
	{
		vec3_t v;
		vec3_t vecTo;
		float distance;

		if ( i & 1 )
		{
			v[0] = tr.viewParms.visBounds[0][0];
		}
		else
		{
			v[0] = tr.viewParms.visBounds[1][0];
		}

		if ( i & 2 )
		{
			v[1] = tr.viewParms.visBounds[0][1];
		}
		else
		{
			v[1] = tr.viewParms.visBounds[1][1];
		}

		if ( i & 4 )
		{
			v[2] = tr.viewParms.visBounds[0][2];
		}
		else
		{
			v[2] = tr.viewParms.visBounds[1][2];
		}

		VectorSubtract( v, tr.viewParms.or.origin, vecTo );

		distance = vecTo[0] * vecTo[0] + vecTo[1] * vecTo[1] + vecTo[2] * vecTo[2];

		if ( distance > farthestCornerDistance )
		{
			farthestCornerDistance = distance;
		}
	}
	tr.viewParms.zFar = sqrt( farthestCornerDistance );
}


/*
===============
R_SetupProjection
===============
*/
static void R_SetupProjection( void ) {
	float	xmin, xmax, ymin, ymax;
	float	width, height, depth;
	float	zNear, zFar, zProj, stereoSep;

	// dynamically compute far clip plane distance
	SetFarClip();

	//
	// set up projection matrix
	//
	zNear	= r_znear->value;
	zProj	= r_zproj->value;
	zFar	= tr.viewParms.zFar;
	stereoSep = r_stereoSeparation->value / 100.0f;

	ymax = zNear * tan( trScene.refdef->fov_y * M_PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( trScene.refdef->fov_x * M_PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zFar - zNear;
	
#if 1
	tr.viewParms.projectionMatrix[0] = 2 * zNear / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = (xmax + xmin + 2 * stereoSep) / width;	// normally 0
	tr.viewParms.projectionMatrix[12] = 2 * zProj * stereoSep / width;

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 * zNear / height;
	tr.viewParms.projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	tr.viewParms.projectionMatrix[13] = 0;

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -( zFar + zNear ) / depth;
	tr.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = -1;
	tr.viewParms.projectionMatrix[15] = 0;
#else 
	tr.viewParms.projectionMatrix[0] = 2 / width;
	tr.viewParms.projectionMatrix[4] = 0;
	tr.viewParms.projectionMatrix[8] = 0;
	tr.viewParms.projectionMatrix[12] = -(xmax + xmin) / (xmax - xmin);

	tr.viewParms.projectionMatrix[1] = 0;
	tr.viewParms.projectionMatrix[5] = 2 / height;
	tr.viewParms.projectionMatrix[9] = 0;
	tr.viewParms.projectionMatrix[13] = -(ymax + ymin) / (ymax - ymin);

	tr.viewParms.projectionMatrix[2] = 0;
	tr.viewParms.projectionMatrix[6] = 0;
	tr.viewParms.projectionMatrix[10] = -2 / depth;
	tr.viewParms.projectionMatrix[14] = -(zFar + zNear) / (zFar - zNear);

	tr.viewParms.projectionMatrix[3] = 0;
	tr.viewParms.projectionMatrix[7] = 0;
	tr.viewParms.projectionMatrix[11] = 1;
	tr.viewParms.projectionMatrix[15] = 0;


#endif
}

/*
=================
R_SetupFrustum

Setup that culling frustum planes for the current view
=================
*/
void R_SetupFrustum (void) {
	int		i;
	float	xs, xc;
	float	ang;
	float	stereoSep;
	vec3_t	origin;

	stereoSep = r_stereoSeparation->value / 100.0f;
	
	if(stereoSep == 0) {
		VectorCopy(tr.viewParms.or.origin, origin);
	} else {
		VectorMA(tr.viewParms.or.origin, stereoSep*20, tr.viewParms.or.axis[1], origin);
	}

	VectorCopy(tr.viewParms.or.origin, origin);


	ang = tr.viewParms.fovX / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.or.axis[0], xs, tr.viewParms.frustum[0].normal );
	VectorMA( tr.viewParms.frustum[0].normal, xc, tr.viewParms.or.axis[1], tr.viewParms.frustum[0].normal );

	VectorScale( tr.viewParms.or.axis[0], xs, tr.viewParms.frustum[1].normal );
	VectorMA( tr.viewParms.frustum[1].normal, -xc, tr.viewParms.or.axis[1], tr.viewParms.frustum[1].normal );

	ang = tr.viewParms.fovY / 180 * M_PI * 0.5f;
	xs = sin( ang );
	xc = cos( ang );

	VectorScale( tr.viewParms.or.axis[0], xs, tr.viewParms.frustum[2].normal );
	VectorMA( tr.viewParms.frustum[2].normal, xc, tr.viewParms.or.axis[2], tr.viewParms.frustum[2].normal );

	VectorScale( tr.viewParms.or.axis[0], xs, tr.viewParms.frustum[3].normal );
	VectorMA( tr.viewParms.frustum[3].normal, -xc, tr.viewParms.or.axis[2], tr.viewParms.frustum[3].normal );

	for (i=0 ; i<4 ; i++) {
		tr.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		tr.viewParms.frustum[i].dist = DotProduct (origin, tr.viewParms.frustum[i].normal);
		SetPlaneSignbits( &tr.viewParms.frustum[i] );
	}
}


/*
=================
R_MirrorPoint
=================
*/
void R_MirrorPoint (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	VectorSubtract( in, surface->origin, local );

	VectorClear( transformed );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(local, surface->axis[i]);
		VectorMA( transformed, d, camera->axis[i], transformed );
	}

	VectorAdd( transformed, camera->origin, out );
}

void R_MirrorVector (vec3_t in, orientation_t *surface, orientation_t *camera, vec3_t out) {
	int		i;
	float	d;

	VectorClear( out );
	for ( i = 0 ; i < 3 ; i++ ) {
		d = DotProduct(in, surface->axis[i]);
		VectorMA( out, d, camera->axis[i], out );
	}
}


/*
=============
R_PlaneForSurface
=============
*/
void R_PlaneForSurface ( const surfaceType_t *surfType, cplane_t *plane) {
	const srfTriangles_t	*tri;
	const srfPoly_t	*poly;
	const float		*v1, *v2, *v3;
	const vecSimd_t	*s;
	vec4_t			plane4;

	if (!surfType) {
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;
		return;
	}
	switch (*surfType) {
	case SF_FACE:
		*plane = ((srfFace_t *)surfType)->plane;
		return;
	case SF_TRIANGLES:
		tri = (srfTriangles_t *)surfType;
		v1 = tri->xyz[tri->indexes[0]];
		v2 = tri->xyz[tri->indexes[1]];
		v3 = tri->xyz[tri->indexes[2]];
		PlaneFromPoints( plane4, v1, v2, v3 );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	case SF_POLY:
		poly = (srfPoly_t *)surfType;
		s = (vecSimd_t*)(poly + 1);
		PlaneFromPoints( plane4, s[0], s[1], s[2] );
		VectorCopy( plane4, plane->normal ); 
		plane->dist = plane4[3];
		return;
	default:
		Com_Memset (plane, 0, sizeof(*plane));
		plane->normal[0] = 1;		
		return;
	}
}

/*
=================
R_GetPortalOrientation

entityNum is the entity that the portal surface is a part of, which may
be moving and rotating.

Returns qtrue if it should be mirrored
=================
*/
qboolean R_GetPortalOrientations( const drawSurf_t *drawSurf, 
							 orientation_t *surface, orientation_t *camera,
							 vec3_t pvsOrigin, qboolean *mirror ) {
	cplane_t	originalPlane, plane;
	scenePortal_t	*portal;
	float		d;
	vec3_t		transformed;
	unsigned int sort;

	// create plane axis for the portal we are seeing

	sort = drawSurf->sort;
	// rotate the plane if necessary
	if ( sort & QSORT_HAVEMODEL_MASK ) {
		const drawModel_t *drawModel;
		const surfaceType_t *surf;

		drawModel = (drawModel_t *) drawSurf->data;
		surf = drawModel->surface[ R_SortToIndex( sort ) ] ;

		R_PlaneForSurface( surf, &originalPlane );

		// get the orientation of the entity
		R_RotateForEntity( drawModel->model, &tr.viewParms, &tr.or );

		// rotate the plane, but keep the non-rotated version for matching
		// against the portalSurface entities
		R_LocalNormalToWorld( originalPlane.normal, plane.normal );
		plane.dist = originalPlane.dist + DotProduct( plane.normal, tr.or.origin );

		// translate the original plane
		originalPlane.dist = originalPlane.dist + DotProduct( originalPlane.normal, tr.or.origin );
	} else {
		R_PlaneForSurface( (surfaceType_t *) drawSurf->data, &originalPlane );
		plane = originalPlane;
	}

	VectorCopy( plane.normal, surface->axis[0] );
	PerpendicularVector( surface->axis[1], surface->axis[0] );
	CrossProduct( surface->axis[0], surface->axis[1], surface->axis[2] );

	// locate the portal entity closest to this plane.
	// origin will be the origin of the portal, origin2 will be
	// the origin of the camera
	for ( portal = trScene.portals;  portal; portal = portal->next ) {
		d = DotProduct( portal->origin, originalPlane.normal ) - originalPlane.dist;
		if ( d > 64 || d < -64) {
			continue;
		}

		// get the pvsOrigin from the entity
		VectorCopy( portal->pvsOrigin, pvsOrigin );
		
		// if the entity is just a mirror, don't use as a camera point
		if ( VectorCompare( portal->origin, portal->pvsOrigin) ) {
			VectorScale( plane.normal, plane.dist, surface->origin );
			VectorCopy( surface->origin, camera->origin );
			VectorSubtract( vec3_origin, surface->axis[0], camera->axis[0] );
			VectorCopy( surface->axis[1], camera->axis[1] );
			VectorCopy( surface->axis[2], camera->axis[2] );

			*mirror = qtrue;
			return qtrue;
		}

		// project the origin onto the surface plane to get
		// an origin point we can rotate around
		d = DotProduct( portal->origin, plane.normal ) - plane.dist;
		VectorMA( portal->origin, -d, surface->axis[0], surface->origin );
			
		// now get the camera origin and orientation
		VectorCopy( portal->pvsOrigin, camera->origin );
		AxisCopy( portal->axis, camera->axis );
		VectorSubtract( vec3_origin, camera->axis[0], camera->axis[0] );
		VectorSubtract( vec3_origin, camera->axis[1], camera->axis[1] );

		// optionally rotate
		if ( portal->rotate ) {
			// if a speed is specified
			if ( portal->rotateSpeed ) {
				// continuous rotate
				d = (trScene.refdef->time/1000.0f) * portal->rotateSpeed + (trScene.refdef->timeFraction/1000.0f) * portal->rotateSpeed;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			} else {
				// bobbing rotate, with skinNum being the rotation offset
				d = sin( trScene.refdef->time * 0.003 + trScene.refdef->timeFraction * 0.003 );
				d = portal->rotateOffset + d * 4;
				VectorCopy( camera->axis[1], transformed );
				RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
				CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
			}
		}
		else if ( portal->rotateOffset ) {
			d = portal->rotateOffset;
			VectorCopy( camera->axis[1], transformed );
			RotatePointAroundVector( camera->axis[1], camera->axis[0], transformed, d );
			CrossProduct( camera->axis[0], camera->axis[1], camera->axis[2] );
		}
		*mirror = qfalse;
		return qtrue;
	}

	// if we didn't locate a portal entity, don't render anything.
	// We don't want to just treat it as a mirror, because without a
	// portal entity the server won't have communicated a proper entity set
	// in the snapshot

	// unfortunately, with local movement prediction it is easily possible
	// to see a surface before the server has communicated the matching
	// portal surface entity, so we don't want to print anything here...

	//ri.Printf( PRINT_ALL, "Portal surface without a portal entity\n" );

	return qfalse;
}

/*
** SurfIsOffscreen
**
** Determines if a surface is completely offscreen.
*/
static qboolean SurfIsOffscreen( const drawSurf_t *drawSurf ) {
	shader_t *shader;
	const surfaceType_t *srf;
	float	distance;
	unsigned sort = drawSurf->sort;

	if ( sort & QSORT_HAVEMODEL_MASK ) {
		const drawModel_t *drawModel = (drawModel_t *)drawSurf->data;
		srf = drawModel->surface[ R_SortToIndex( sort ) ];
		R_RotateForEntity( drawModel->model, &tr.viewParms, &tr.or );
	} else {
		srf = (surfaceType_t *)drawSurf->data;
		//Rotate the view so we can correctly check for visibily
		R_RotateForViewer();
	}


	switch ( *srf ) {
	case SF_FACE:
		distance = Distance( ((srfFace_t *)srf)->xyz[0], tr.or.origin );
		distance -= 64; //Slightly random value, portals shouldn't be that huge
		break;
	case SF_GRID:
		distance = Distance( ((srfGridMesh_t *)srf)->meshOrigin, tr.or.origin );
		distance -= ((srfGridMesh_t *)srf)->meshRadius;
		break;
	case SF_TRIANGLES:
		distance = Distance( ((srfTriangles_t *)srf)->origin, tr.or.origin );
		distance -= ((srfTriangles_t *)srf)->radius;
		break;
	default:
		return qfalse;
	}
	

	shader = R_SortToShader( sort );
	if ( shader->portalRange > 1 && distance > shader->portalRange ) {
		return qtrue;
	}

	return qfalse;
}

/*
========================
R_MirrorViewBySurface

Returns qtrue if another view has been rendered
========================
*/
qboolean R_MirrorViewBySurface ( const drawSurf_t *drawSurf ) {
	viewParms_t		newParms;
	viewParms_t		oldParms;
	orientation_t	surface, camera;

	// don't recursively mirror
	if (tr.viewParms.isPortal) {
//		ri.Printf( PRINT_DEVELOPER, "WARNING: recursive mirror/portal found\n" );
		return qfalse;
	}

	if ( r_noportals->integer || (r_fastsky->integer == 1) ) {
		return qfalse;
	}

	// trivially reject portal/mirror
	if ( SurfIsOffscreen( drawSurf ) ) {
//		return qfalse;
	}

	oldParms = tr.viewParms;
	newParms = tr.viewParms;
	newParms.isPortal = oldParms.isPortal + 1;

	if ( !R_GetPortalOrientations( drawSurf, &surface, &camera, newParms.pvsOrigin, &newParms.isMirror ) ) {
		return qfalse;		// bad portal, no portalentity
	}

	R_MirrorPoint (oldParms.or.origin, &surface, &camera, newParms.or.origin );

	VectorSubtract( vec3_origin, camera.axis[0], newParms.portalPlane.normal );
	newParms.portalPlane.dist = DotProduct( camera.origin, newParms.portalPlane.normal );
	
	R_MirrorVector (oldParms.or.axis[0], &surface, &camera, newParms.or.axis[0]);
	R_MirrorVector (oldParms.or.axis[1], &surface, &camera, newParms.or.axis[1]);
	R_MirrorVector (oldParms.or.axis[2], &surface, &camera, newParms.or.axis[2]);

	// OPTIMIZE: restrict the viewport on the mirrored view

	// render the mirror view
	R_RenderView (&newParms);

	tr.viewParms = oldParms;

	return qtrue;
}

/*
=================
R_SpriteFogNum

See if a sprite is inside a fog volume
=================
*/
int R_FogFromRadius( const vec3_t origin, const float radius ) {
	int				i;
	const fog_t			*fog;

	if ( trScene.refdef->rdflags & RDF_NOWORLDMODEL ) {
		return 0;
	}

	for ( i = 1 ; i < tr.world->numfogs ; i++ ) {
		fog = &tr.world->fogs[i];
		if ( origin[0] - radius >= fog->bounds[1][0] ) 
			continue;
		if ( origin[0] + radius <= fog->bounds[0][0] ) 
			continue;
		if ( origin[1] - radius >= fog->bounds[1][1] )
			continue;
		if ( origin[1] + radius <= fog->bounds[0][1] )
			continue;
		if ( origin[2] - radius >= fog->bounds[1][2] )
			continue;
		if ( origin[2] + radius <= fog->bounds[0][2] )
			continue;
		return i;
	}
	return 0;
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
=================
qsort replacement

=================
*/
static __inline void SWAP_DRAW_SURF(drawSurf_t* a, drawSurf_t* b) {
	drawSurf_t t;
	memcpy(&t, a, sizeof(t));
	memcpy(a, b, sizeof(t));
	memcpy(b, &t, sizeof(t));
}

/* this parameter defines the cutoff between using quick sort and
   insertion sort for arrays; arrays with lengths shorter or equal to the
   below value use insertion sort */

#define CUTOFF 8            /* testing shows that this is good value */

static void shortsort( drawSurf_t *lo, drawSurf_t *hi ) {
    drawSurf_t	*p, *max;

    while (hi > lo) {
        max = lo;
        for (p = lo + 1; p <= hi; p++ ) {
            if ( p->sort > max->sort ) {
                max = p;
            }
        }
        SWAP_DRAW_SURF(max, hi);
        hi--;
    }
}


/* sort the array between lo and hi (inclusive)
FIXME: this was lifted and modified from the microsoft lib source...
 */

void qsortFast (
    void *base,
    unsigned num,
    unsigned width
    )
{
    char *lo, *hi;              /* ends of sub-array currently sorting */
    char *mid;                  /* points to middle of subarray */
    char *loguy, *higuy;        /* traveling pointers for partition step */
    unsigned size;              /* size of the sub-array */
    char *lostk[30], *histk[30];
    int stkptr;                 /* stack for saving sub-array to be processed */

#if 0
	if ( sizeof(drawSurf_t) != 8 ) {
		ri.Error( ERR_DROP, "change SWAP_DRAW_SURF macro" );
	}
#endif

    /* Note: the number of stack entries required is no more than
       1 + log2(size), so 30 is sufficient for any array */

    if (num < 2 || width == 0)
        return;                 /* nothing to do */

    stkptr = 0;                 /* initialize stack */

    lo = base;
    hi = (char *)base + width * (num-1);        /* initialize limits */

    /* this entry point is for pseudo-recursion calling: setting
       lo and hi and jumping to here is like recursion, but stkptr is
       prserved, locals aren't, so we preserve stuff on the stack */
recurse:

    size = (hi - lo) / width + 1;        /* number of el's to sort */

    /* below a certain size, it is faster to use a O(n^2) sorting method */
    if (size <= CUTOFF) {
         shortsort((drawSurf_t *)lo, (drawSurf_t *)hi);
    }
    else {
        /* First we pick a partititioning element.  The efficiency of the
           algorithm demands that we find one that is approximately the
           median of the values, but also that we select one fast.  Using
           the first one produces bad performace if the array is already
           sorted, so we use the middle one, which would require a very
           wierdly arranged array for worst case performance.  Testing shows
           that a median-of-three algorithm does not, in general, increase
           performance. */

        mid = lo + (size / 2) * width;      /* find middle element */
        SWAP_DRAW_SURF((drawSurf_t *)mid, (drawSurf_t *)lo); /* swap it to beginning of array */

       
        /* We now wish to partition the array into three pieces, one
           consisiting of elements <= partition element, one of elements
           equal to the parition element, and one of element >= to it.  This
           is done below; comments indicate conditions established at every
           step. */

        loguy = lo;
        higuy = hi + width;

        /* Note that higuy decreases and loguy increases on every iteration,
           so loop must terminate. */
        for (;;) {
            /* lo <= loguy < hi, lo < higuy <= hi + 1,
               A[i] <= A[lo] for lo <= i <= loguy,
               A[i] >= A[lo] for higuy <= i <= hi */

            do  {
                loguy += width;
            } while (loguy <= hi &&  
				( ((drawSurf_t *)loguy)->sort <= ((drawSurf_t *)lo)->sort ) );

            /* lo < loguy <= hi+1, A[i] <= A[lo] for lo <= i < loguy,
               either loguy > hi or A[loguy] > A[lo] */

            do  {
                higuy -= width;
            } while (higuy > lo && 
				( ((drawSurf_t *)higuy)->sort >= ((drawSurf_t *)lo)->sort ) );

            /* lo-1 <= higuy <= hi, A[i] >= A[lo] for higuy < i <= hi,
               either higuy <= lo or A[higuy] < A[lo] */

            if (higuy < loguy)
                break;

            /* if loguy > hi or higuy <= lo, then we would have exited, so
               A[loguy] > A[lo], A[higuy] < A[lo],
               loguy < hi, highy > lo */

            SWAP_DRAW_SURF((drawSurf_t *)loguy, (drawSurf_t *)higuy);

            /* A[loguy] < A[lo], A[higuy] > A[lo]; so condition at top
               of loop is re-established */
        }

        /*     A[i] >= A[lo] for higuy < i <= hi,
               A[i] <= A[lo] for lo <= i < loguy,
               higuy < loguy, lo <= higuy <= hi
           implying:
               A[i] >= A[lo] for loguy <= i <= hi,
               A[i] <= A[lo] for lo <= i <= higuy,
               A[i] = A[lo] for higuy < i < loguy */

        SWAP_DRAW_SURF((drawSurf_t *)lo, (drawSurf_t *)higuy); /* put partition element in place */

        /* OK, now we have the following:
              A[i] >= A[higuy] for loguy <= i <= hi,
              A[i] <= A[higuy] for lo <= i < higuy
              A[i] = A[lo] for higuy <= i < loguy    */

        /* We've finished the partition, now we want to sort the subarrays
           [lo, higuy-1] and [loguy, hi].
           We do the smaller one first to minimize stack usage.
           We only sort arrays of length 2 or more.*/

        if ( higuy - 1 - lo >= hi - loguy ) {
            if (lo + width < higuy) {
                lostk[stkptr] = lo;
                histk[stkptr] = higuy - width;
                ++stkptr;
            }                           /* save big recursion for later */

            if (loguy < hi) {
                lo = loguy;
                goto recurse;           /* do small recursion */
            }
        }
        else {
            if (loguy < hi) {
                lostk[stkptr] = loguy;
                histk[stkptr] = hi;
                ++stkptr;               /* save big recursion for later */
            }

            if (lo + width < higuy) {
                hi = higuy - width;
                goto recurse;           /* do small recursion */
            }
        }
    }

    /* We have sorted the array, except for any pending sorts on the stack.
       Check if there are any, and do them. */

    --stkptr;
    if (stkptr >= 0) {
        lo = lostk[stkptr];
        hi = histk[stkptr];
        goto recurse;           /* pop subarray from stack */
    }
    else
        return;                 /* all subarrays done */
}


//==========================================================================================

/*
=================
R_AddDrawSurf
=================
*/



void R_AddDrawSurf( const void *data, const shader_t *shader, int fogIndex, int mask ) {
	drawSurf_t	*draw;
	int size;

#if defined(_DEBUG)
	assert( data && shader  );
#endif
	size = (trScene.numDrawSurfs ) * sizeof( drawSurf_t );
	if ( trScene.backEnd->used + size + sizeof( drawSurf_t ) > trScene.backEnd->size ) {
		ri.Error( ERR_DROP, "RenderScene ran out of storage, increase r_backEndMegs" );
	}
	draw = (drawSurf_t*)( trScene.backEnd->data + trScene.backEnd->used + size );
	trScene.numDrawSurfs++;
	draw->sort = mask;
	draw->sort |= (shader->sortedIndex << QSORT_SHADER_SHIFT);
	draw->sort |= ( fogIndex << QSORT_FOG_SHIFT );
	draw->data = data;
}


/*
=================
R_DecomposeSort
=================
*/
void R_DecomposeSort( unsigned sort, shader_t **shader, int *fogNum ) {
	*fogNum = ( sort & QSORT_FOG_MASK ) >> QSORT_FOG_SHIFT;
	*shader = tr.sortedShaders[ ( sort & QSORT_SHADER_MASK ) >> QSORT_SHADER_SHIFT ];
}

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	// sort the drawsurfs by sort type, then orientation, then shader
	qsortFast (drawSurfs, numDrawSurfs, sizeof(drawSurf_t) );
}

/*
=================
R_CopySortSurfs
=================
*/


qboolean R_HandlePortalSurfaces( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	// check for any pass through drawing, which
	// may cause another view to be rendered first
	shader_t		*shader;
	int				i;
	qboolean		donePortal = qfalse;

	for ( i = 0 ; i < numDrawSurfs ; i++ ) {
		drawSurf_t *surf = drawSurfs+i;

		shader = R_SortToShader( surf->sort );
		/* Array sorted so no more beyond portal */
		if ( shader->sort > SS_PORTAL)
			break;

		if ( shader->sort == SS_BAD )
			break;

		// if the mirror was completely clipped away, we may need to check another surface
		if ( R_MirrorViewBySurface( surf ) ) {
			// this is a debug option to see exactly what is being mirrored
			donePortal = qtrue;
			break;		// only one mirror view at a time
		}
	}
	return donePortal;
}

/*
=============
R_AddEntitySurfaces
=============
*/
void R_AddEntitySurfaces (void) {
	srfEntity_t	*ent;
	srfSprite_t *sprite;
	srfPoly_t	*poly;
	srfBeam_t	*beam;
	srfRings_t	*rings;
	int			skipMask;

	if ( !r_drawentities->integer ) {
		return;
	}

	if (tr.viewParms.isPortal)
		skipMask = RF_FIRST_PERSON;
	else
		skipMask = RF_THIRD_PERSON;

	for (ent = trScene.entities; ent; ent = ent->next ) {
		switch ( ent->surfaceType ) {
		case SF_SPRITE:
			sprite = (srfSprite_t *)ent;
			if ( sprite->renderfx & skipMask )
				continue;
			if ( R_CullPointAndRadius( sprite->origin, sprite->radius) == CULL_OUT )
				continue;
			if ( sprite->renderfx & RF_CULLRADIUS && DistanceSquared( sprite->origin, tr.viewParms.pvsOrigin ) < sprite->radius * sprite->radius )
				continue;
			sprite->fogNum = R_FogFromRadius( sprite->origin, sprite->radius );
			R_AddDrawSurf( &sprite->surfaceType, sprite->shader, sprite->fogNum, 0 );
			break;
		case SF_RINGS:
			rings = (srfRings_t *)ent;
			if ( rings->renderfx & skipMask)
				continue;
			//TODO rings fog??
			R_AddDrawSurf( &rings->surfaceType, rings->shader, 0, 0 );
			break;
		case SF_BEAM:
			beam = (srfBeam_t *)ent;
			if ( beam->renderfx & skipMask )
				continue;
			//TODO beam fog??
			R_AddDrawSurf( &beam->surfaceType, beam->shader, 0, 0 );
			break;
		case SF_POLY:
			poly = (srfPoly_t *)ent;
//			if ( R_CullPointAndRadius( sprite->origin, sprite->radius) == CULL_OUT )
//				continue;
			R_AddDrawSurf( &poly->surfaceType, poly->sh, poly->fogNum, 0 );
			break;
		}
	}
}


void R_AddModelSurfaces( void ) {
	int				skipMask;
	sceneModel_t	*ent;

	if ( !r_drawmodels->integer ) {
		return;
	}

	if (tr.viewParms.isPortal)
		skipMask = RF_FIRST_PERSON;
	else
		skipMask = RF_THIRD_PERSON;

	for ( ent = trScene.models; ent; ent = ent->next ) {
		if ( ent->renderfx & skipMask)
			continue;
		// we must set up parts of tr.or for model culling
		R_RotateForEntity( ent, &tr.viewParms, &tr.or );

		if (!ent->model) {
			//CANATODO, use different kind of missing model thingie
//			R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
		} else {
			switch ( ent->model->type ) {
			case MOD_MESH:
				R_AddMD3Surfaces( ent );
				break;
			case MOD_MD4:
				R_AddAnimSurfaces( ent );
				break;
#ifdef RAVENMD4
			case MOD_MDR:
				R_MDRAddAnimSurfaces( ent );
				break;
#endif
			case MOD_BRUSH:
				R_AddBrushModelSurfaces( ent );
				break;
			case MOD_BAD:		// null model axis
//				if ( (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal) {
//					break;
//				}
//				shader = R_GetShaderByHandle( ent->e.customShader );
//				R_AddDrawSurf( &entitySurface, tr.defaultShader, 0, 0 );
				break;
			default:
				ri.Error( ERR_DROP, "R_AddEntitySurfaces: Bad modeltype" );
				break;
			}
		}
	}
}

#include "../qcommon/cm_local.h"

void R_AddDebugSurfaces( void ) {
	if ( r_showtriggers->integer ) {
		int i, b ;
		for ( i = 0; i < cm.numSubModels; i++ ) {
			cmodel_t *cmod = &cm.cmodels[i];
			cLeaf_t *leaf = &cmod->leaf;
			for ( b = 0; b < leaf->numLeafBrushes; b++ ) {
				int brushnum = cm.leafbrushes[leaf->firstLeafBrush+b];
				cbrush_t* brush = &cm.brushes[brushnum];
				if ( ! (brush->contents & CONTENTS_TRIGGER ) )
					continue;



			}
		}
	}
}

/*
====================
R_GenerateDrawSurfs
====================
*/
void R_GenerateDrawSurfs( void ) {
	/* First surfaces don't have any model link */
	R_AddWorldSurfaces ();

	// set the projection matrix with the minimum zfar
	// now that we have the world bounded
	// this needs to be done before entities are
	// added, because they use the projection
	// matrix for lod calculation
	R_SetupProjection ();

	R_AddEntitySurfaces ();
	
	R_AddModelSurfaces ();

	R_AddDebugSurfaces ();

}


/*
================
R_DebugPolygon
================
*/
void R_DebugPolygon( int color, int numPoints, float *points ) {
	int		i;

	GL_State( GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// draw solid shade

	qglColor3f( color&1, (color>>1)&1, (color>>2)&1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();

	// draw wireframe outline
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	qglDepthRange( 0, 0 );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < numPoints ; i++ ) {
		qglVertex3fv( points + i * 3 );
	}
	qglEnd();
	qglDepthRange( 0, 1 );
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics( void ) {
	if ( !r_debugSurface->integer ) {
		return;
	}

	// the render thread can't make callbacks to the main thread
	R_SyncRenderThread();

	GL_Bind( tr.whiteImage);
	GL_Cull( CT_FRONT_SIDED );
	ri.CM_DrawDebugSurface( R_DebugPolygon );
}


/*
================
R_RenderView

A view may be either the actual camera view,
or a mirror / remote location
================
*/
void R_RenderView (viewParms_t *parms) {
	drawSurf_t	*drawSurfs;
	int numSurfaces, size;
	//Start positions of the different sort tables

	if ( parms->viewportWidth <= 0 || parms->viewportHeight <= 0 ) {
		return;
	}

	tr.viewCount++;

	tr.viewParms = *parms;
	tr.viewParms.frameSceneNum = tr.frameSceneNum;
	tr.viewParms.frameCount = tr.frameCount;

	tr.viewCount++;

	// set viewParms.world
	R_RotateForViewer ();
	R_SetupFrustum ();

	trScene.numDrawSurfs = 0;
	//Allocate special command buffer block, wil hackly resize it later
	drawSurfs = (drawSurf_t	*)R_GetCommandBuffer( RC_ALLOC, 0 ); 
	R_GenerateDrawSurfs();
	//Copy this before we check for portals
	numSurfaces = trScene.numDrawSurfs;
	trScene.numDrawSurfs = 0;
	/* Fix up the previous allocated command buffer with correct size */
	size = numSurfaces * sizeof ( drawSurf_t );
	((commandHeader_t *)drawSurfs)[-1].size += size;
	trScene.backEnd->used += size;
	/* Sort all surfaces so shaders group up and are rendered in correct order */
	R_SortDrawSurfs( drawSurfs, numSurfaces );
	/* Check for portal surfaces, they'll be rendered first */
	if (!(R_HandlePortalSurfaces( drawSurfs, numSurfaces ) && r_portalOnly->integer)) {
		R_AddDrawSurfCmd( drawSurfs, numSurfaces );
	}
	// draw main system development information (surface outlines, etc)
	R_DebugGraphics();
}



