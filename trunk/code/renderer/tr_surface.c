/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later versio

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_surf.c
#include "tr_local.h"
#if idppc_altivec
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

static ID_INLINE void copySimd_t( const vecSimd_t *src, vecSimd_t *dst, int count ) {
	if ( count & 1 ) {
		VectorCopy( src[0], dst[0] );
		src++; dst++;
	}
	for ( count>>=1; count > 0; count--, src+=2 , dst+=2 ) {
		double c0, c1, c2, c3;
		double *in = (double *) src;
		double *out = (double *) dst;

		c0 = in[0];
		c1 = in[1];
		c2 = in[2];
//		c3 = in[3];
		out[0] = c0;
		out[1] = c1;
		out[2] = c2;
//		out[3] = c3;
	}
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( const vec3_t origin, const vec3_t left, const vec3_t up, const color4ub_t color) {
	int			ndx;
	int			*indexes;
	vecSimd_t		*xyz;
	vec2_t		*texCoords;
	color4ub_t	*colors;

	RB_CheckOverflow( 4, 6 );

	ndx = tess.numVertexes;
	indexes = tess.indexes + tess.numIndexes;

	indexes[0] = ndx+0;
	indexes[1] = ndx+1;
	indexes[2] = ndx+3;
	indexes[3] = ndx+3;
	indexes[4] = ndx+1;
	indexes[5] = ndx+2;

	xyz = tess.xyz + ndx;
	xyz[0][0] = origin[0] + left[0] + up[0];
	xyz[0][1] = origin[1] + left[1] + up[1];
	xyz[0][2] = origin[2] + left[2] + up[2];
	xyz[1][0] = origin[0] - left[0] + up[0];
	xyz[1][1] = origin[1] - left[1] + up[1];
	xyz[1][2] = origin[2] - left[2] + up[2];
	xyz[2][0] = origin[0] - left[0] - up[0];
	xyz[2][1] = origin[1] - left[1] - up[1];
	xyz[2][2] = origin[2] - left[2] - up[2];
	xyz[3][0] = origin[0] + left[0] - up[0];
	xyz[3][1] = origin[1] + left[1] - up[1];
	xyz[3][2] = origin[2] + left[2] - up[2];

	// constant normal all the way around

	if (tess.needMask & SHADER_NEED_NORMAL) {
		vecSimd_t *normal = tess.normal + ndx;
		normal[0][0] = normal[1][0] = normal[2][0] = normal[3][0] = -backEnd.viewParms.or.axis[0][0];
		normal[0][1] = normal[1][1] = normal[2][1] = normal[3][1] = -backEnd.viewParms.or.axis[0][1];
		normal[0][2] = normal[1][2] = normal[2][2] = normal[3][2] = -backEnd.viewParms.or.axis[0][2];
	}
	
	texCoords = tess.texCoords + ndx;
	// standard square texture coordinates
	texCoords[0][0] = 0;
	texCoords[0][1] = 0;
	texCoords[1][0] = 1;
	texCoords[1][1] = 0;
	texCoords[2][0] = 1;
	texCoords[2][1] = 1;
	texCoords[3][0] = 0;
	texCoords[3][1] = 1;

	colors = tess.colors + ndx;
	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	Byte4Copy( color[0], colors[0]);
	Byte4Copy( color[0], colors[1]);
	Byte4Copy( color[0], colors[2]);
	Byte4Copy( color[0], colors[3]);

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

static ID_INLINE void LineUpVector( const vec3_t start, const vec3_t end, vec3_t up ) {
	vec3_t v1, v2;
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}


/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( const srfSprite_t *sprite ) {
	vec3_t		left, up;
	float		radius;

	RB_CheckShaderTime( backEnd.refdef->time - sprite->shaderTime );
	// calculate the xyz locations for the four corners
	radius = sprite->radius;
	if ( sprite->rotation == 0 ) {
		VectorScale( backEnd.viewParms.or.axis[1], radius, left );
		VectorScale( backEnd.viewParms.or.axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;
		
		ang = M_PI * sprite->rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.or.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.or.axis[2], left );

		VectorScale( backEnd.viewParms.or.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.or.axis[1], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( sprite->origin, left, up, sprite->color );
	if (tess.needMask & SHADER_NEED_ENTITY ) {
		/* Maybe just hack the shader to no longer use entity? */
		Byte4Copy( sprite->color, backEnd.entityColor );
		RB_EndSurface();
	}
}


/*
=============
RB_SurfacePolychain
=============
*/
void RB_SurfacePolychain( const srfPoly_t *p ) {
	int		i;
	int		numv;
	const vecSimd_t	*xyz;
	const vec2_t	*st;
	const color4ub_t *color;


	RB_CheckShaderTime( backEnd.refdef->time );
	RB_CheckOverflow( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	xyz = ((vecSimd_t *)( p + 1));
	st = ((vec2_t*)( xyz + p->numVerts));
	color = (color4ub_t*)( st + p->numVerts );
	for ( i = p->numVerts; i>0; i--, xyz++, st++, color++ ) {
		VectorCopy( xyz[0], tess.xyz[numv] );
		tess.texCoords[numv][0] = st[0][0];
		tess.texCoords[numv][1] = st[0][1];
		Byte4Copy( color[0], tess.colors[numv] );
		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes ;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}
	tess.numVertexes = numv;
}


/*
=============
RB_SurfaceTriangles
=============
*/
void RB_SurfaceTriangles( const srfTriangles_t *surf ) {
	int i;

	RB_CheckShaderTime( backEnd.refdef->time );
	RB_CheckOverflow( surf->numVertexes, surf->numIndexes );
	tess.dlightBits |= surf->dlightBits[ backEnd.smpFrame ];
	for ( i = 0; i < surf->numVertexes ; i++ ) {
		VectorCopy( surf->xyz[i], tess.xyz[ tess.numVertexes + i] );
		VectorCopy( surf->normal[i], tess.normal[ tess.numVertexes + i ] );
		tess.texCoords[tess.numVertexes + i][0] = surf->texCoords[i][0];
		tess.texCoords[tess.numVertexes + i][1] = surf->texCoords[i][1];
		tess.lightMap[tess.numVertexes + i][0] = surf->lightMap[i][0];
		tess.lightMap[tess.numVertexes + i][1] = surf->lightMap[i][1];
		Byte4Copy( surf->colors[i], tess.colors[tess.numVertexes + i]);
	}
	for ( i = 0; i < surf->numIndexes; i++ ) 
		tess.indexes[ i + tess.numIndexes ] = tess.numVertexes + surf->indexes[i];

	tess.numVertexes += surf->numVertexes;
	tess.numIndexes += surf->numIndexes;
}

/*
==============
RB_SurfaceBeam
==============
*/
void RB_SurfaceBeam( const srfBeam_t *beam ) 
{
	float	length, width;
	vec3_t	direction, line, up;
	float	angle = 0;

	RB_CheckShaderTime( backEnd.refdef->time - beam->shaderTime );
	VectorSubtract( beam->end, beam->start, direction );
	length = VectorNormalize( direction );
	if (!length )
		return;

	LineUpVector( beam->start, beam->end, line );
	VectorCopy( line, up );
	width = beam->width * 0.5f;
	while ( angle <= 180 ) {
		vecSimd_t *xyz;
		int	*indexes;
		vec2_t *texCoords;
		color4ub_t *colors;

		RB_CheckOverflow( 4, 6 );

		xyz = tess.xyz + tess.numVertexes;

		VectorMA( beam->start, -width, up, xyz[0] );
		VectorMA( beam->start, width, up, xyz[1] );
		VectorMA( beam->end, width, up, xyz[2] );
		VectorMA( beam->end, -width, up, xyz[3] );

#if 0
		VectorSet( xyz[0], -100, -100, 100 );
		VectorSet( xyz[1], -100, 100, 100 );
		VectorSet( xyz[2],  100, 100, 100 );
		VectorSet( xyz[3],  100, -100, 100 );
#endif

		texCoords = tess.texCoords + tess.numVertexes;
		texCoords[0][0] = 0;
		texCoords[0][1] = 0;
		
		texCoords[1][0] = 0;
		texCoords[1][1] = 1;

		texCoords[2][0] = 1;
		texCoords[2][1] = 1;

		texCoords[3][0] = 1;
		texCoords[3][1] = 0;

		colors = tess.colors + tess.numVertexes;
		
		Byte4Copy( beam->color, colors[0] );
		Byte4Copy( beam->color, colors[1] );
		Byte4Copy( beam->color, colors[2] );
		Byte4Copy( beam->color, colors[3] );
		
		indexes = tess.indexes + tess.numIndexes;
		indexes[0] = tess.numVertexes+0;
		indexes[1] = tess.numVertexes+1;
		indexes[2] = tess.numVertexes+2;
		indexes[3] = tess.numVertexes+0;
		indexes[4] = tess.numVertexes+2;
		indexes[5] = tess.numVertexes+3;
		
		tess.numIndexes += 6;
		tess.numVertexes += 4;

		if ( !beam->rotateAngle )
			break;
		angle += beam->rotateAngle;
		RotatePointAroundVector( up, direction, line, angle );
	}
	if (tess.needMask & SHADER_NEED_ENTITY ) {
		/* Maybe just hack the shader to no longer use entity? */
		Byte4Copy( beam->color, backEnd.entityColor );
		RB_EndSurface();
	}
}

/*
==============
RB_SurfaceRings
==============
*/
void RB_SurfaceRings( const srfRings_t *rings ) {
	float	dist, length, stepSize;
	vec3_t	dir, up, right;

	RB_CheckShaderTime( backEnd.refdef->time - rings ->shaderTime );
	VectorSubtract( rings ->end, rings ->start, dir );
	length = VectorNormalize( dir );
	if (!length )
		return;
	stepSize = rings->stepSize;
	if ( stepSize <= 1 )
		stepSize = 1;

	if ( r_railRingsAlign->integer ) {
		VectorCopy( backEnd.refdef->viewaxis[1], right );
		VectorCopy( backEnd.refdef->viewaxis[2], up );
	} else {
		LineUpVector( rings->start, rings->end, up );
		MakeNormalVectors( dir, right, up );
	}

	VectorScale( up, rings->radius, up );
	VectorScale( right, rings->radius, right );

	for ( dist = 0; dist < length; dist += stepSize ) {
		vec3_t origin;

		VectorMA( rings->start, dist, dir, origin );
		RB_AddQuadStamp( origin, right, up, rings->color );
	}

	if (tess.needMask & SHADER_NEED_ENTITY ) {
		/* Maybe just hack the shader to no longer use entity? */
		Byte4Copy( rings->color, backEnd.entityColor );
		RB_EndSurface();
	}
}

/*
==============
RB_SurfaceDecal
==============
*/
void RB_SurfaceDecal( const srfDecal_t *decal ) {
	int		n;
	const byte *fragments;
	const vec3_t	*xyz;
	const vec2_t	*st;
	color4ub_t color;

	RB_CheckShaderTime( backEnd.refdef->time );
	Byte4Copy( decal->color, color );
	if (!(decal->flags & DECAL_TEMP)) {
		float leftTime = (decal->endTime - backEnd.refdef->time) - backEnd.refdef->timeFraction; 
		if ( decal->flags & DECAL_ENERGY) {
			if ( leftTime > 7000) {

			} else if ( leftTime > (7000 - 2048)) {
				float fade = leftTime - (7000 - 2048);
				color[0] = (color[0] * fade) / 2048.0f;
				color[1] = (color[1] * fade) / 2048.0f;
				color[2] = (color[2] * fade) / 2048.0f;
			} else {
				if ( leftTime < 1024) {
					color[3] = (color[3] * leftTime) / 1024.0f;
				}
				color[0] = 0;
				color[1] = 0;
				color[2] = 0;
			}
		} else if ( decal->flags & DECAL_ALPHA ) {
			if ( leftTime < 1024) {
				color[3] = (color[3] * leftTime) / 1024.0f;
			}
		} else {
			if ( leftTime < 1024) {
				color[0] = (color[0] * leftTime) / 1024.0f;
				color[1] = (color[1] * leftTime) / 1024.0f;
				color[2] = (color[2] * leftTime) / 1024.0f;
			}
		}
	}
	xyz = decal->xyz;
	st = (vec2_t *)(xyz + decal->numVerts);
	fragments = (byte *)(st + decal->numVerts);

	for ( n = 0; n < decal->numFragments; n++) {
		int i;
		int numVerts;
		numVerts = fragments[n];
		/* overkill index size but 4* is easy */
		RB_CheckOverflow( numVerts, 4*numVerts );
		/* Generate the indexes */
		for ( i = 0; i < numVerts-2; i++ ) {
			tess.indexes[tess.numIndexes + 0] = tess.numVertexes ;
			tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
			tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
			tess.numIndexes += 3;
		}
		i = tess.numVertexes;
		tess.numVertexes += numVerts;
		/* Copy the vertex data */
		for (;numVerts>0;numVerts--, xyz++, st++, i++) {
			VectorCopy( xyz[0], tess.xyz[i] );
			tess.texCoords[i][0] = st[0][0];
			tess.texCoords[i][1] = st[0][1];
			Byte4Copy( color, tess.colors[i] );
		}
	}
}

/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(float *normals, unsigned int count)
{
//    assert(count);
        
#if idppc
    {
        register float half = 0.5;
        register float one  = 1.0;
        float *components = (float *)normals;
        
        // Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
        // runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
        // refinement step to get a little more precision.  This seems to yeild results
        // that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
        // (That is, for the given input range of about 0.6 to 2.0).
        do {
            float x, y, z;
            float B, y0, y1;
            
            x = components[0];
            y = components[1];
            z = components[2];
            components += 4;
            B = x*x + y*y + z*z;

#ifdef __GNUC__            
            asm("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
			y0 = __frsqrte(B);
#endif
            y1 = y0 + half*y0*(one - B*y0*y0);

            x = x * y1;
            y = y * y1;
            components[-4] = x;
            z = z * y1;
            components[-3] = y;
            components[-2] = z;
        } while(count--);
    }
#else // No assembly version for this architecture, or C_ONLY defined
	// given the input, it's safe to call VectorNormalizeFast
    while (count--) {
        VectorNormalizeFast( normals );
        normals += 4;
    }
#endif

}



/*
** LerpMeshVertexes
*/
static void LerpMeshVertexes (md3Surface_t *surf, float backlerp) 
{
	const short	*oldXyz, *newXyz;
	float 	*outXyz, *outNormal;
	int		vertNum;
	int		lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentModel->frame * surf->numVerts * 4);
	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4,
			outXyz += VEC_SIMD, outNormal += VEC_SIMD) 
		{	
			const short *newNormal = newXyz + 3;

			outXyz[0] = newXyz[0] * MD3_XYZ_SCALE;
			outXyz[1] = newXyz[1] * MD3_XYZ_SCALE;
			outXyz[2] = newXyz[2] * MD3_XYZ_SCALE;

			lat = ( newNormal[0] >> 8 ) & 0xff;
			lng = ( newNormal[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
	} else {
		int		latScale, lngScale;
		int		normalScale;
		float	oldXyzScale, newXyzScale;

		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentModel->oldframe * surf->numVerts * 4);
		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
		normalScale = (1.0 - backlerp) * 255;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4,
			outXyz += VEC_SIMD, outNormal += VEC_SIMD) 
		{
			const short *newNormal, *oldNormal;
			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			oldNormal = oldXyz + 3;
			newNormal = newXyz + 3;
			/* Interpolate the angles */
			lat = (FUNCTABLE_SIZE/256) * (( newNormal[0] >> 8 ) & 0xff);
			latScale = (FUNCTABLE_SIZE/256) * (( oldNormal[0] >> 8 ) & 0xff) - lat;

			if ( latScale < -FUNCTABLE_SIZE/2 )
				latScale += FUNCTABLE_SIZE;
			else if ( latScale > FUNCTABLE_SIZE/2)
				latScale -= FUNCTABLE_SIZE;
			lat = lat + ((latScale * normalScale) >> 8);

			lng = (FUNCTABLE_SIZE/256) * (( newNormal[0] >> 0 ) & 0xff);
			lngScale = (FUNCTABLE_SIZE/256) * (( oldNormal[0] >> 0 ) & 0xff) - lng;

			if ( lngScale < -FUNCTABLE_SIZE/2 )
				lngScale += FUNCTABLE_SIZE;
			else if ( lngScale > FUNCTABLE_SIZE/2)
				lngScale -= FUNCTABLE_SIZE;

			lng = lng + ((lngScale * normalScale) >> 8);

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng&FUNCTABLE_MASK];
			outNormal[1] = tr.sinTable[lat&FUNCTABLE_MASK] * tr.sinTable[lng&FUNCTABLE_MASK];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
   	}
}

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh( md3Surface_t *surface ) {
	int				j;
	float			backlerp;
	int				*triangles;
	vec2_t			*texCoords;
	float			*meshCoords;
	int				*indexes;
	int				numVerts;
	int				numIndexes;

	RB_CheckShaderTime( backEnd.refdef->time - backEnd.currentModel->shaderTime );
	Byte4Copy( backEnd.currentModel->shaderRGBA, backEnd.entityColor );

	if (  backEnd.currentModel->oldframe == backEnd.currentModel->frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentModel->backlerp;
	}

	numIndexes = surface->numTriangles * 3;

	RB_CheckOverflow( surface->numVerts, numIndexes );

	LerpMeshVertexes( surface, backlerp );

	triangles = (int *) ((byte *)surface + surface->ofsTriangles);

	indexes = tess.indexes + tess.numIndexes;
	for (j = 0 ; j < numIndexes ; j++) {
		indexes[j] = tess.numVertexes + triangles[j];
	}
	tess.numIndexes += numIndexes;

	meshCoords = (float *) ((byte *)surface + surface->ofsSt);
	texCoords = tess.texCoords + tess.numVertexes;

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		texCoords[j][0] = meshCoords[j*2+0];
		texCoords[j][1] = meshCoords[j*2+1];
	}
	tess.numVertexes += numVerts;
}


/*
==============
RB_SurfaceFace
==============
*/
void RB_SurfaceFace( const srfFace_t *surf ) {
	int i;

	RB_CheckShaderTime( backEnd.refdef->time );
	RB_CheckOverflow( surf->numVertexes, surf->numIndexes );

	tess.dlightBits |= surf->dlightBits[ backEnd.smpFrame ];

	memcpy( tess.texCoords[ tess.numVertexes], surf->texCoords, surf->numVertexes * sizeof( vec2_t )); 
	memcpy( tess.lightMap[ tess.numVertexes], surf->lightMap, surf->numVertexes * sizeof( vec2_t )); 
	copySimd_t( surf->xyz, tess.xyz + tess.numVertexes, surf->numVertexes );
	if ( tess.needMask & SHADER_NEED_COLOR ) {
		memcpy( tess.colors[ tess.numVertexes ], surf->colors, surf->numVertexes * sizeof( color4ub_t ));
	}
	if ( tess.needMask & SHADER_NEED_NORMAL ) {
		float n0, n1, n2;
		n0 = surf->plane.normal[0];
		n1 = surf->plane.normal[1];
		n2 = surf->plane.normal[2];
		for ( i = 0; i < surf->numVertexes; i++ ) {
			tess.normal[ tess.numVertexes + i][0] = n0;
			tess.normal[ tess.numVertexes + i][1] = n1;
			tess.normal[ tess.numVertexes + i][2] = n2;
		}
	}
	for ( i = 0;i < surf->numIndexes; i+=3 ) {
		tess.indexes[tess.numIndexes + i + 0] = tess.numVertexes + surf->indexes[i + 0];
		tess.indexes[tess.numIndexes + i + 1] = tess.numVertexes + surf->indexes[i + 1];
		tess.indexes[tess.numIndexes + i + 2] = tess.numVertexes + surf->indexes[i + 2];
	}
	tess.numIndexes += surf->numIndexes;
	tess.numVertexes += surf->numVertexes;
}

static float	LodErrorForVolume( vec3_t local, float radius ) {
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.or.axis[0][0] + local[1] * backEnd.or.axis[1][0] + 
		local[2] * backEnd.or.axis[2][0] + backEnd.or.origin[0];
	world[1] = local[0] * backEnd.or.axis[0][1] + local[1] * backEnd.or.axis[1][1] + 
		local[2] * backEnd.or.axis[2][1] + backEnd.or.origin[1];
	world[2] = local[0] * backEnd.or.axis[0][2] + local[1] * backEnd.or.axis[1][2] + 
		local[2] * backEnd.or.axis[2][2] + backEnd.or.origin[2];

	VectorSubtract( world, backEnd.viewParms.or.origin, world );
	d = DotProduct( world, backEnd.viewParms.or.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
void RB_SurfaceGrid( srfGridMesh_t *cv ) {
	
	int		i, j;
	vecSimd_t	*xyz;
	vecSimd_t	*normal;
	vec2_t		*texCoords;
	vec2_t		*lightMap;
	color4ub_t	*colors;
	drawVert_t	*dv;
	int			*indexes;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		dlightBits;

	dlightBits = cv->dlightBits[ backEnd.smpFrame ];


	// determine the allowable discrepance
	lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < cv->width-1 ; i++ ) {
		if ( cv->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = cv->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < cv->height-1 ; i++ ) {
		if ( cv->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = cv->height-1;
	lodHeight++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	rows = 0;
	tess.dlightBits |= dlightBits;

	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				tess.dlightBits = dlightBits;
			} else {
				break;
			}
		} while ( 1 );

		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		xyz = tess.xyz + tess.numVertexes;
		normal = tess.normal + tess.numVertexes;
		texCoords = tess.texCoords + tess.numVertexes;
		lightMap = tess.lightMap + tess.numVertexes;
		colors = tess.colors + tess.numVertexes;

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = cv->verts + heightTable[ used + i ] * cv->width
					+ widthTable[ j ];

				xyz[0][0] = dv->xyz[0];
				xyz[0][1] = dv->xyz[1];
				xyz[0][2] = dv->xyz[2];
				texCoords[0][0] = dv->st[0];
				texCoords[0][1] = dv->st[1];
				lightMap[0][0] = dv->lightmap[0];
				lightMap[0][1] = dv->lightmap[1];
				normal[0][0] = dv->normal[0];
				normal[0][1] = dv->normal[1];
				normal[0][2] = dv->normal[2];
				Byte4Copy( dv->color, colors[0] );
				xyz++;
				normal++;
				texCoords++;
				lightMap++;
				colors++;
			}
		}

		indexes = tess.indexes + tess.numIndexes;
		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++, indexes += 6) {
					int		v1, v2, v3, v4;
			
					// vertex order to be reckognized as tristrips
					v1 = tess.numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					indexes[0] = v2;
					indexes[1] = v3;
					indexes[2] = v1;
					
					indexes[3] = v1;
					indexes[4] = v3;
					indexes[5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
void RB_SurfaceAxis( void ) {
	GL_Bind( tr.whiteImage );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
}

//===========================================================================


void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

#if 0

void RB_SurfaceFlare( srfFlare_t *surf ) {
	vec3_t		left, up;
	float		radius;
	byte		color[4];
	vec3_t		dir;
	vec3_t		origin;
	float		d;

	// calculate the xyz locations for the four corners
	radius = 30;
	VectorScale( backEnd.viewParms.or.axis[1], radius, left );
	VectorScale( backEnd.viewParms.or.axis[2], radius, up );
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

	color[0] = color[1] = color[2] = color[3] = 255;

	VectorMA( surf->origin, 3, surf->normal, origin );
	VectorSubtract( origin, backEnd.viewParms.or.origin, dir );
	VectorNormalize( dir );
	VectorMA( origin, r_ignore->value, dir, origin );

	d = -DotProduct( dir, surf->normal );
	if ( d < 0 ) {
		return;
	}
#if 0
	color[0] *= d;
	color[1] *= d;
	color[2] *= d;
#endif

	RB_AddQuadStamp( origin, left, up, color );
}

#else

void RB_SurfaceFlare( srfFlare_t *surf ) {
	if (r_flares->integer) {
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal);
	}
#if 0
	vec3_t		left, up;
	byte		color[4];

	color[0] = surf->color[0] * 255;
	color[1] = surf->color[1] * 255;
	color[2] = surf->color[2] * 255;
	color[3] = 255;

	VectorClear( left );
	VectorClear( up );

	left[0] = r_ignore->value;

	up[1] = r_ignore->value;
	
	RB_AddQuadStampExt( surf->origin, left, up, color, 0, 0, 1, 1 );
#endif
}

#endif

void RB_SurfaceSkip( void *surf ) {
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( const void *) = {
	(void(*)(const void*))RB_SurfaceBad,			// SF_BAD, 
	(void(*)(const void*))RB_SurfaceSkip,			// SF_SKIP, 
	(void(*)(const void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(const void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(const void*))RB_SurfaceTriangles,	// SF_TRIANGLES,
	(void(*)(const void*))RB_SurfacePolychain,	// SF_POLY,
	(void(*)(const void*))RB_SurfaceMesh,			// SF_MD3,
	(void(*)(const void*))RB_SurfaceAnim,			// SF_MD4,
#ifdef RAVENMD4
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
#endif
	(void(*)(const void*))RB_SurfaceSprite,		// SF_SPRITE,
	(void(*)(const void*))RB_SurfaceBeam,		// SF_BEAM, 
	(void(*)(const void*))RB_SurfaceRings,		// SF_RINGS, 
	(void(*)(const void*))RB_SurfaceDecal,		// SF_DECAL,
	(void(*)(const void*))RB_SurfaceFlare,		// SF_FLARE,
};
