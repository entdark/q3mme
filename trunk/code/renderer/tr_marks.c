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
// tr_marks.c -- polygon projection on the world polygons

#include "tr_local.h"
//#include "assert.h"

#define MAX_VERTS_ON_POLY		64
#define MAX_DECAL_FRAGMENTS		256
#define MAX_DECAL_POINTS		(MAX_DECAL_FRAGMENTS * 3)

#define MARKER_OFFSET			0	// 1

/*
=============
R_ChopPolyBehindPlane

Out must have space for two more vertexes than in
=============
*/
#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
static void R_ChopPolyBehindPlane( int numInPoints, vec3_t inPoints[MAX_VERTS_ON_POLY],
								   int *numOutPoints, vec3_t outPoints[MAX_VERTS_ON_POLY], 
							vec3_t normal, vec_t dist, vec_t epsilon) {
	float		dists[MAX_VERTS_ON_POLY+4];
	int			sides[MAX_VERTS_ON_POLY+4];
	int			counts[3];
	float		dot;
	int			i, j;
	float		*p1, *p2, *clip;
	float		d;

	// don't clip if it might overflow
	if ( numInPoints >= MAX_VERTS_ON_POLY - 2 ) {
		*numOutPoints = 0;
		return;
	}

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0 ; i < numInPoints ; i++ ) {
		dot = DotProduct( inPoints[i], normal );
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*numOutPoints = 0;

	if ( !counts[0] ) {
		return;
	}
	if ( !counts[1] ) {
		*numOutPoints = numInPoints;
		Com_Memcpy( outPoints, inPoints, numInPoints * sizeof(vec3_t) );
		return;
	}

	for ( i = 0 ; i < numInPoints ; i++ ) {
		p1 = inPoints[i];
		clip = outPoints[ *numOutPoints ];
		
		if ( sides[i] == SIDE_ON ) {
			VectorCopy( p1, clip );
			(*numOutPoints)++;
			continue;
		}
	
		if ( sides[i] == SIDE_FRONT ) {
			VectorCopy( p1, clip );
			(*numOutPoints)++;
			clip = outPoints[ *numOutPoints ];
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}
			
		// generate a split point
		p2 = inPoints[ (i+1) % numInPoints ];

		d = dists[i] - dists[i+1];
		if ( d == 0 ) {
			dot = 0;
		} else {
			dot = dists[i] / d;
		}

		// clip xyz

		for (j=0 ; j<3 ; j++) {
			clip[j] = p1[j] + dot * ( p2[j] - p1[j] );
		}

		(*numOutPoints)++;
	}
}

/*
=================
R_BoxSurfaces_r

=================
*/
void R_BoxSurfaces_r(mnode_t *node, vec3_t mins, vec3_t maxs, surfaceType_t **list, int listsize, int *listlength, vec3_t dir) {

	int			s, c;
	msurface_t	*surf, **mark;

	// do the tail recursion in a loop
	while ( node->contents == -1 ) {
		s = BoxOnPlaneSide( mins, maxs, node->plane );
		if (s == 1) {
			node = node->children[0];
		} else if (s == 2) {
			node = node->children[1];
		} else {
			R_BoxSurfaces_r(node->children[0], mins, maxs, list, listsize, listlength, dir);
			node = node->children[1];
		}
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while (c--) {
		//
		if (*listlength >= listsize) break;
		//
		surf = *mark;
		// check if the surface has NOIMPACT or NOMARKS set
		if ( ( surf->shader->surfaceFlags & ( SURF_NOIMPACT | SURF_NOMARKS ) )
			|| ( surf->shader->contentFlags & CONTENTS_FOG ) ) {
			surf->viewCount = tr.viewCount;
		}
		// extra check for surfaces to avoid list overflows
		else if (*(surf->data) == SF_FACE) {
			// the face plane should go through the box
			s = BoxOnPlaneSide( mins, maxs, &(( srfFace_t* ) surf->data)->plane );
			if (s == 1 || s == 2) {
				surf->viewCount = tr.viewCount;
			} else if (DotProduct((( srfFace_t * ) surf->data)->plane.normal, dir) > -0.5) {
			// don't add faces that make sharp angles with the projection direction
				surf->viewCount = tr.viewCount;
			}
		}
		else if (*(surfaceType_t *) (surf->data) != SF_GRID) surf->viewCount = tr.viewCount;
		// check the viewCount because the surface may have
		// already been added if it spans multiple leafs
		if (surf->viewCount != tr.viewCount) {
			surf->viewCount = tr.viewCount;
			list[*listlength] = (surfaceType_t *) surf->data;
			(*listlength)++;
		}
		mark++;
	}
}

/*
=================
R_AddMarkFragments

=================
*/
void R_AddMarkFragments(int numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY],
				   int numPlanes, vec3_t *normals, float *dists,
				   int maxPoints, vec3_t pointBuffer,
				   int maxFragments, markFragment_t *fragmentBuffer,
				   int *returnedPoints, int *returnedFragments ) {
	int pingPong, i;
	markFragment_t	*mf;

	// chop the surface by all the bounding planes of the to be projected polygon
	pingPong = 0;

	for ( i = 0 ; i < numPlanes ; i++ ) {

		R_ChopPolyBehindPlane( numClipPoints, clipPoints[pingPong],
						   &numClipPoints, clipPoints[!pingPong],
							normals[i], dists[i], 0.5 );
		pingPong ^= 1;
		if ( numClipPoints == 0 ) {
			break;
		}
	}
	// completely clipped away?
	if ( numClipPoints == 0 ) {
		return;
	}

	// add this fragment to the returned list
	if ( numClipPoints + (*returnedPoints) > maxPoints ) {
		return;	// not enough space for this polygon
	}

	mf = fragmentBuffer + (*returnedFragments);
	mf->firstPoint = (*returnedPoints);
	mf->numPoints = numClipPoints;
	Com_Memcpy( pointBuffer + (*returnedPoints) * 3, clipPoints[pingPong], numClipPoints * sizeof(vec3_t) );

	(*returnedPoints) += numClipPoints;
	(*returnedFragments)++;
}

/*
=================
R_MarkFragments

=================
*/
int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) {
	int				numsurfaces, numPlanes;
	int				i, j, k, m, n;
	surfaceType_t	*surfaces[64];
	vec3_t			mins, maxs;
	int				returnedFragments;
	int				returnedPoints;
	vec3_t			normals[MAX_VERTS_ON_POLY+2];
	float			dists[MAX_VERTS_ON_POLY+2];
	vec3_t			clipPoints[2][MAX_VERTS_ON_POLY];
	int				numClipPoints;
	const float		*v;
	srfFace_t		*surf;
	srfGridMesh_t	*cv;
	drawVert_t		*dv;
	vec3_t			normal;
	vec3_t			projectionDir;
	vec3_t			v1, v2;
	const int		*indexes;

	//increment view count for double check prevention
	tr.viewCount++;

	//
	VectorNormalize2( projection, projectionDir );
	// find all the brushes that are to be considered
	ClearBounds( mins, maxs );
	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t	temp;

		AddPointToBounds( points[i], mins, maxs );
		VectorAdd( points[i], projection, temp );
		AddPointToBounds( temp, mins, maxs );
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA( points[i], -20, projectionDir, temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if (numPoints > MAX_VERTS_ON_POLY) numPoints = MAX_VERTS_ON_POLY;
	// create the bounding planes for the to be projected polygon
	for ( i = 0 ; i < numPoints ; i++ ) {
		VectorSubtract(points[(i+1)%numPoints], points[i], v1);
		VectorAdd(points[i], projection, v2);
		VectorSubtract(points[i], v2, v2);
		CrossProduct(v1, v2, normals[i]);
		VectorNormalizeFast(normals[i]);
		dists[i] = DotProduct(normals[i], points[i]);
	}
	// add near and far clipping planes for projection
	VectorCopy(projectionDir, normals[numPoints]);
	dists[numPoints] = DotProduct(normals[numPoints], points[0]) - 32;
	VectorCopy(projectionDir, normals[numPoints+1]);
	VectorInverse(normals[numPoints+1]);
	dists[numPoints+1] = DotProduct(normals[numPoints+1], points[0]) - 20;
	numPlanes = numPoints + 2;

	numsurfaces = 0;
	R_BoxSurfaces_r(tr.world->nodes, mins, maxs, surfaces, 64, &numsurfaces, projectionDir);
	//assert(numsurfaces <= 64);
	//assert(numsurfaces != 64);

	returnedPoints = 0;
	returnedFragments = 0;

	for ( i = 0 ; i < numsurfaces ; i++ ) {

		if (*surfaces[i] == SF_GRID) {

			cv = (srfGridMesh_t *) surfaces[i];
			for ( m = 0 ; m < cv->height - 1 ; m++ ) {
				for ( n = 0 ; n < cv->width - 1 ; n++ ) {
					// We triangulate the grid and chop all triangles within
					// the bounding planes of the to be projected polygon.
					// LOD is not taken into account, not such a big deal though.
					//
					// It's probably much nicer to chop the grid itself and deal
					// with this grid as a normal SF_GRID surface so LOD will
					// be applied. However the LOD of that chopped grid must
					// be synced with the LOD of the original curve.
					// One way to do this; the chopped grid shares vertices with
					// the original curve. When LOD is applied to the original
					// curve the unused vertices are flagged. Now the chopped curve
					// should skip the flagged vertices. This still leaves the
					// problems with the vertices at the chopped grid edges.
					//
					// To avoid issues when LOD applied to "hollow curves" (like
					// the ones around many jump pads) we now just add a 2 unit
					// offset to the triangle vertices.
					// The offset is added in the vertex normal vector direction
					// so all triangles will still fit together.
					// The 2 unit offset should avoid pretty much all LOD problems.

					numClipPoints = 3;

					dv = cv->verts + m * cv->width + n;

					VectorCopy(dv[0].xyz, clipPoints[0][0]);
					VectorMA(clipPoints[0][0], MARKER_OFFSET, dv[0].normal, clipPoints[0][0]);
					VectorCopy(dv[cv->width].xyz, clipPoints[0][1]);
					VectorMA(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1]);
					VectorCopy(dv[1].xyz, clipPoints[0][2]);
					VectorMA(clipPoints[0][2], MARKER_OFFSET, dv[1].normal, clipPoints[0][2]);
					// check the normal of this triangle
					VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
					VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
					CrossProduct(v1, v2, normal);
					VectorNormalizeFast(normal);
					if (DotProduct(normal, projectionDir) < -0.1) {
						// add the fragments of this triangle
						R_AddMarkFragments(numClipPoints, clipPoints,
										   numPlanes, normals, dists,
										   maxPoints, pointBuffer,
										   maxFragments, fragmentBuffer,
										   &returnedPoints, &returnedFragments );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;	// not enough space for more fragments
						}
					}

					VectorCopy(dv[1].xyz, clipPoints[0][0]);
					VectorMA(clipPoints[0][0], MARKER_OFFSET, dv[1].normal, clipPoints[0][0]);
					VectorCopy(dv[cv->width].xyz, clipPoints[0][1]);
					VectorMA(clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1]);
					VectorCopy(dv[cv->width+1].xyz, clipPoints[0][2]);
					VectorMA(clipPoints[0][2], MARKER_OFFSET, dv[cv->width+1].normal, clipPoints[0][2]);
					// check the normal of this triangle
					VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
					VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
					CrossProduct(v1, v2, normal);
					VectorNormalizeFast(normal);
					if (DotProduct(normal, projectionDir) < -0.05) {
						// add the fragments of this triangle
						R_AddMarkFragments(numClipPoints, clipPoints,
										   numPlanes, normals, dists,
										   maxPoints, pointBuffer,
										   maxFragments, fragmentBuffer,
										   &returnedPoints, &returnedFragments );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;	// not enough space for more fragments
						}
					}
				}
			}
		}
		else if (*surfaces[i] == SF_FACE) {
			surf = (srfFace_t *) surfaces[i];
			// check the normal of this face
			if (DotProduct(surf->plane.normal, projectionDir) > -0.5) {
				continue;
			}

			/*
			VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
			VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
			CrossProduct(v1, v2, normal);
			VectorNormalize(normal);
			if (DotProduct(normal, projectionDir) > -0.5) continue;
			*/
			for ( k = 0 ; k < surf->numIndexes ; k += 3 ) {
				indexes = surf->indexes + k;
				for ( j = 0 ; j < 3 ; j++ ) {
					v = surf->xyz[indexes[j]];
					VectorMA( v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j] );
				}
				// add the fragments of this face
				R_AddMarkFragments( 3 , clipPoints,
								   numPlanes, normals, dists,
								   maxPoints, pointBuffer,
								   maxFragments, fragmentBuffer,
								   &returnedPoints, &returnedFragments );
				if ( returnedFragments == maxFragments ) {
					return returnedFragments;	// not enough space for more fragments
				}
			}
			continue;
		}
		else {
			// ignore all other world surfaces
			// might be cool to also project polygons on a triangle soup
			// however this will probably create huge amounts of extra polys
			// even more than the projection onto curves
			continue;
		}
	}
	return returnedFragments;
}

static struct {
	srfDecal_t			*list;
	srfDecal_t			*listSearch;
	int					allocSize;			//Keep this for easy reset
	int					surfIndex;			//Increase this with each alloc, keep sorting the same each frame
} trDecal;

static cvar_t *r_decalSize;

static void R_DecalStats( void ) {
	int activeCount, freeCount;
	int activeSize, freeSize;
	srfDecal_t *decal;

	decal = trDecal.list;
	activeCount = freeCount = 0;
	activeSize = freeSize = 0;
	while ( 1 ) {
		if ( decal->shader ) {
			activeCount++;
			activeSize += decal->size;
		} else {
			freeCount++;
			freeSize += decal->size;
		}
		if ( decal->lastOne )
			break;
		decal = (srfDecal_t *)(((byte *)decal) + decal->size);
	}
	Com_Printf("Active %d size %d, free %d size %d, total %d\n", activeCount, activeSize, freeCount, freeSize, activeSize + freeSize );
}


/* Find an empty decal with enough size to store the maximum data */
static srfDecal_t *R_DecalAlloc( void ) {
	int needSize;
	srfDecal_t *decal;

	needSize = sizeof( srfDecal_t );
	needSize += MAX_DECAL_POINTS * sizeof (vec3_t);
	needSize += MAX_DECAL_POINTS * sizeof (vec2_t);
	needSize += MAX_DECAL_FRAGMENTS;
	//Align on a pointer size
	needSize = (needSize + sizeof( void * )) & ~(sizeof( void * ));

	decal = trDecal.listSearch;
	while ( decal ) {
		if ( decal->shader || decal->size < needSize ) {
			if ( !decal->lastOne ) {
				decal = (srfDecal_t *)(((byte *)decal) + decal->size);
			} else {
				decal = trDecal.list;
			}
			if ( decal == trDecal.listSearch ) {
				Com_DPrintf( "Ran out of decal space, resetting\n" );
				R_DecalReset();
				return 0;
			}
			continue;
		}
		return decal;
	}
	return 0;
}

static void R_DecalResize( srfDecal_t *decal, int newSize ) {
	int leftSize;

	newSize = (newSize + 3) & ~3;
	leftSize = decal->size - newSize;
	if ( leftSize < 0) {
		Com_Printf("OMG Decal fuckup\n" );
		return;
	}
	if ( leftSize > 32 ) {
		srfDecal_t *newDecal;
		decal->size = newSize;
		newDecal = (srfDecal_t *)(((byte *)decal) + newSize);
		newDecal->shader = 0;
		newDecal->size = leftSize;
		newDecal->lastOne = decal->lastOne;
		decal->lastOne = qfalse;
		//Store the next search to be in the newly resized one
		trDecal.listSearch = newDecal;
	}
}

static qboolean R_DecalPoints( int numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY], 
						 int numPlanes, vec3_t *normals, float *dists,
						 int *numPoints, int maxPoints, vec3_t *points,
						 int *numFragments, int maxFragments, byte *fragments
						 ) {
	int pingPong, i;

	// chop the surface by all the bounding planes of the to be projected polygon
	pingPong = 0;

	for ( i = 0 ; i < numPlanes ; i++ ) {

		R_ChopPolyBehindPlane( numClipPoints, clipPoints[pingPong],
						   &numClipPoints, clipPoints[!pingPong],
							normals[i], dists[i], 0.5 );
		pingPong ^= 1;
		if ( numClipPoints == 0 ) {
			return qtrue;
		}
	}
	if (*numPoints + numClipPoints > maxPoints)
		return qfalse;
	if (*numFragments >= maxFragments)
		return qfalse;
	for	(i = 0;i<numClipPoints;i++) {
		VectorCopy( clipPoints[pingPong][i], points[*numPoints]);
		(*numPoints)++;
	}
	fragments[*numFragments] = numClipPoints;
	(*numFragments)++;

	return qtrue;
}

void R_DecalProject( qhandle_t shader, const vec3_t origin, const vec3_t dir, const vec4_t color, float orientation, float radius, int flags, int lifeTime ) {
	vec3_t			projectPoints[4];
	vec3_t			projectOrigin;
	vec3_t			projectDir;
	float			projectDistance;
	vec3_t			up, right;

	float			texCoordScale;
	int				numFragments;
	int				numPoints;
	markFragment_t	markFragments[MAX_DECAL_FRAGMENTS];

	int				i;
	srfDecal_t		*decal;
	vec3_t			*xyz;
	vec2_t			*st;
	byte			*fragments;

	if (!shader)
		return;

	decal = R_DecalAlloc( );
	if (!decal)
		return;

	projectDistance = VectorNormalize2( dir, projectDir );

	PerpendicularVector( up, projectDir );
	RotatePointAroundVector( right, projectDir, up, orientation );
	CrossProduct( projectDir, right, up );

	texCoordScale = 0.5 * 1.0 / radius;
//	VectorMA( origin, -radius, projectDir, projectOrigin );
	VectorCopy( origin, projectOrigin );
	projectDistance += radius;

	// create the full polygon
	for ( i = 0 ; i < 3 ; i++ ) {
		projectPoints[0][i] = projectOrigin[i] - radius * up[i] - radius * right[i];
		projectPoints[1][i] = projectOrigin[i] + radius * up[i] - radius * right[i];
		projectPoints[2][i] = projectOrigin[i] + radius * up[i] + radius * right[i];
		projectPoints[3][i] = projectOrigin[i] - radius * up[i] + radius * right[i];
	}
	VectorScale( dir, -20, projectDir );
	numFragments = R_MarkFragments( 4, projectPoints, projectDir, MAX_DECAL_POINTS, &decal->xyz[0][0], MAX_DECAL_FRAGMENTS, markFragments );
	if (!numFragments)
		return;
	//total amount of points making up all the framents
	numPoints = markFragments[numFragments - 1].firstPoint + markFragments[ numFragments - 1 ].numPoints;
	decal->shader = R_GetShaderByHandle( shader );
	decal->flags = flags;
	if ( decal->flags & DECAL_TEMP ) {
		decal->frameCount = tr.frameCount;
	} else {
		decal->frameCount = 0;
		decal->endTime = -lifeTime;
	}
	decal->color[0] = 255*color[0];
	decal->color[1] = 255*color[1];
	decal->color[2] = 255*color[2];
	decal->color[3] = 255*color[3];

	decal->type = SF_DECAL;
	VectorCopy( origin, decal->cullOrigin );
	decal->cullRadius = radius;
	decal->fogIndex = R_FogFromRadius( decal->cullOrigin, decal->cullRadius );
	decal->numFragments = numFragments;
	decal->numVerts = numPoints;
	xyz = decal->xyz;
	st = (vec2_t *)(xyz + numPoints);
	//Generate texture coordinates
	for (i = 0;i<numPoints;i++) {
		vec3_t delta;

		VectorSubtract( xyz[i], origin, delta );
		st[i][0] = 0.5 + DotProduct( delta, right ) * texCoordScale;
		st[i][1] = 0.5 + DotProduct( delta, up ) * texCoordScale;
	}
	fragments = (byte*)( st + numPoints );
	//Add the fragment point counts
	for ( i = 0; i<numFragments;i++ ) {
		fragments[i] = markFragments[i].numPoints;
	}
	//Finalize decal with a surfindex to help with overlapping decals being rendered in same order
	decal->surfIndex = trDecal.surfIndex & (( 1 << QSORT_INDEX_BITS ) - 1);
	trDecal.surfIndex++;
	R_DecalResize( decal, sizeof( srfDecal_t ) + sizeof( float ) * 5 * numPoints + numFragments );
	return;
}

void R_DecalAddSurfaces( void ) {
	srfDecal_t *decal, *prevDecal;
	decal = trDecal.list;
	prevDecal = 0;

	while ( decal ) {
		srfDecal_t *nextDecal;

		if ( !decal->lastOne )
			nextDecal = (srfDecal_t *)(((byte *)decal) + decal->size);
		else
			nextDecal = 0;

		/* Skip empty blocks or decals that are already active this frame */
		if ( !decal->shader ) {
			prevDecal = decal;
			decal = nextDecal;
			continue;
		}
		if ( decal->flags & DECAL_TEMP ) {
			//Only render it for this 1 frame
			if ( decal->frameCount == tr.frameCount ) {
				goto renderDecal;
			}
			goto freeDecal;
			/* Newly created decal, first init the correct time */
		} else if ( decal->endTime <= 0 ) {
			decal->endTime = trScene.refdef->time - decal->endTime;
			goto renderDecal;
			/* Decal that will be cleared next frame */			
		
		} else if ( decal->endTime < trScene.refdef->time ) {
freeDecal:		
			//With smp we make sure the decal wasn't active in the last frame
			if ( !r_smp->integer || (decal->frameCount < (tr.frameCount - 1)) ) {
				if ( prevDecal && !prevDecal->shader ) {
					prevDecal->size += decal->size;
					prevDecal->lastOne = decal->lastOne;
					decal = prevDecal;
					if ( decal == trDecal.listSearch )
						trDecal.listSearch = prevDecal;
				} else {
					decal->shader = 0;
				}
				if ( nextDecal && !nextDecal->shader) {
					if ( nextDecal == trDecal.listSearch )
						trDecal.listSearch = decal;
					decal->size += nextDecal->size;
					decal->lastOne = nextDecal->lastOne;
					if ( !decal->lastOne )
						nextDecal = (srfDecal_t *)(((byte *)decal) + decal->size);
					else
						nextDecal = 0;
				}
			}
		} else {
renderDecal:
			if (R_CullPointAndRadius( decal->cullOrigin, decal->cullRadius ) != CULL_OUT) {
				int mask = decal->surfIndex << QSORT_INDEX_SHIFT;
				R_AddDrawSurf( &decal->type, decal->shader, decal->fogIndex, mask );
				decal->frameCount = tr.frameCount;
			}
		}

		prevDecal = decal;
		decal = nextDecal;
	}
}


void R_DecalReset( void ) {
	/* Go through the entire list setting all to be cleared */
	srfDecal_t *decal = trDecal.list;

	while ( decal ) {
		if ( decal->shader ) {
			decal->endTime = 0;
			decal->flags = DECAL_TEMP;
		}
		if ( !decal->lastOne )
			decal = (srfDecal_t *)(((byte *)decal) + decal->size);
		else
			break;
	}
}


void R_DecalInit( void ) {
	int allocSize;
	r_decalSize = ri.Cvar_Get( "r_decalSize", "512", CVAR_ARCHIVE | CVAR_LATCH );
	memset( &trDecal, 0, sizeof( trDecal ));
	
	allocSize = r_decalSize->integer * 1024;
	if ( allocSize < 64*1024)
		allocSize = 64*1024;
	else if ( allocSize > 2048*1024)
		allocSize = 2048*1024;
	trDecal.allocSize = allocSize;
	trDecal.list = ri.Hunk_Alloc( allocSize, h_low );
	memset( trDecal.list, 0, sizeof( *trDecal.list ));
	trDecal.listSearch = trDecal.list;
	trDecal.list->size = allocSize;
	trDecal.list->lastOne = qtrue;
}

void R_DecalShutdown( void ) {

}
