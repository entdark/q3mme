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
// tr_shade_calc.c

#include "tr_local.h"
#if idppc_altivec
#include <altivec.h>
#endif

static inline float EvalWaveIndex( const waveForm_t *wf, int offset ) {
	float timeFraction = (backEnd.refdef ? backEnd.refdef->timeFraction : 0.0f);
	float index = (float)(tess.shaderTime % wf->interval) + timeFraction;
	index = (index * FUNCTABLE_SIZE) / wf->interval;
	index = ((float)wf->start + index + (float)offset);
	return index;
}
/*
** EvalWaveForm
**
** Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
*/
static float EvalWaveFormAt( const waveForm_t *wf, float index  ) {
	index = fmod(index / FUNCTABLE_SIZE, 1.0);
	if (wf->table == tr.sinTable) {
		return wf->base + wf->amplitude * sin(DEG2RAD(index * 360.0)); 
	} else if (wf->table == tr.triangleTable) {
		if (index < 0.25f) {
			index = index / 0.25f;
		} else if (index < 0.5f) {
			index = 1.0 - ((index - 0.25f) / 0.25f);
		} else if (index < 0.75f) {
			index = -((index - 0.5f) / 0.25f);
		} else if (index <= 1.0f) {
			index = -(1.0f - ((index - 0.75f) / 0.25f));
		}
		return wf->base + wf->amplitude * index; 
	} else if (wf->table == tr.sawToothTable) {
		return wf->base + wf->amplitude * index; 
	} else if (wf->table == tr.inverseSawToothTable) {
		return wf->base + wf->amplitude * (1.0f - index); 
	} else { // square/noise or some unknown table o_o
		return wf->base + wf->amplitude * wf->table[ (int)(index * FUNCTABLE_SIZE) & FUNCTABLE_MASK ]; 
	}
}

static float EvalWaveForm( const waveForm_t *wf ) {
	float index = EvalWaveIndex( wf, 0 );
	return EvalWaveFormAt( wf, index );
}

// not even a table
float NewSinTable (double index) {
	return sin(DEG2RAD(fmod(index, 1.0) * 360.0));
}
float NewCosTable (double index) {
	return cos(DEG2RAD(fmod(index, 1.0) * 360.0));
}


static float EvalWaveFormClamped( const waveForm_t *wf ) {
	float glow  = EvalWaveForm( wf );
	if ( glow < 0 )
		return 0;
	else if ( glow > 1 )
		return 1;
	return glow;
}

/*
** RB_CalcStretchTexCoords
*/
void RB_CalcStretchTexCoords( const waveForm_t *wf,const float *srcTexCoords, float *dstTexCoords ) {
	texModTransform_t tf;
	float p = 1.0f / EvalWaveForm( wf );

	tf.matrix[0][0] = p;
	tf.matrix[1][0] = 0;
	tf.translate[0] = 0.5f - 0.5f * p;

	tf.matrix[0][1] = 0;
	tf.matrix[1][1] = p;
	tf.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tf, srcTexCoords, dstTexCoords );
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
========================
RB_CalcDeformVertexes

========================
*/
void RB_CalcDeformVertexes( const deformStage_t *ds ) {
	int i;
	float	scale;
	const vecSimd_t *inXyz, *normal;
	vecSimd_t *outXyz;

	inXyz = tess.xyz;
	outXyz = tess.xyz;
//	tess.xyz = tess.xyz;
	normal = tess.normal;

	if ( ds->vertexes.wave.interval == 0 ) {
		scale = EvalWaveFormAt( &ds->vertexes.wave, ds->vertexes.wave.start );
		for ( i = 0; i < tess.numVertexes; i++, inXyz++, outXyz++, normal++ ) {
			VectorMA( inXyz[0], scale, normal[0], outXyz[0] );
		}
	} else {
		for ( i = 0; i < tess.numVertexes; i++, inXyz++, outXyz++, normal++ ) {
			int off = Q_ftol(( inXyz[0][0] + inXyz[0][1] + inXyz[0][2] ) * ds->vertexes.spread * FUNCTABLE_SIZE);
			scale = EvalWaveForm( &ds->vertexes.wave );
			VectorMA( inXyz[0], scale, normal[0], outXyz[0] );
		}
	}
}

/*
=========================
RB_CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
void RB_CalcDeformNormals( const deformStage_t *ds ) {
	int i;
	double scale, t;
	const vecSimd_t *xyz, *inNormal;
	vecSimd_t *outNormal;
	double timeFraction = (backEnd.refdef ? backEnd.refdef->timeFraction : 0.0);

	xyz = tess.xyz;
	inNormal = tess.normal;
	outNormal = tess.normal;
//	tess.normal = tess.normal;

	t = (double)tess.shaderTime / (double)ds->normals.interval + timeFraction / (double)ds->normals.interval;

	for ( i = 0; i < tess.numVertexes; i++, xyz++, inNormal++, outNormal++ ) {
		scale = 0.98;
		scale = R_NoiseGet4f( xyz[0][0] * scale, xyz[0][1] * scale, xyz[0][2] * scale, t );
		outNormal[0][0] = inNormal[0][0] + ds->normals.amplitude * scale;

		scale = 0.98;
		scale = R_NoiseGet4f( 100 + xyz[0][0] * scale, xyz[0][1] * scale, xyz[0][2] * scale, t );
		outNormal[0][1] = inNormal[0][1] + ds->normals.amplitude * scale;

		scale = 0.98;
		scale = R_NoiseGet4f( 200 + xyz[0][0] * scale, xyz[0][1] * scale, xyz[0][2] * scale, t );
		outNormal[0][2] = inNormal[0][2] + ds->normals.amplitude * scale;
		VectorNormalizeFast( outNormal[0] );
	}
	
}

/*
========================
RB_CalcBulgeVertexes

========================
*/
void RB_CalcBulgeVertexes( const deformStage_t *ds ) {
	int i;
	const vecSimd_t *inXyz, *normal;
	const vec2_t *texCoords;
	vecSimd_t *outXyz;
	double timeFraction = (backEnd.refdef ? backEnd.refdef->timeFraction : 0.0);
	//CANATODO, fix timing here too
	double now = (double)tess.shaderTime * ds->bulge.speed * 0.001 + timeFraction * ds->bulge.speed * 0.001;

	inXyz = tess.xyz;
	outXyz = tess.xyz;
	normal = tess.normal;
	texCoords = tess.texCoords;

	for ( i = 0; i < tess.numVertexes; i++, inXyz++, outXyz++, texCoords++, normal++ ) {
		double off;
		float scale;
//		off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( texCoords[0][0] * ds->bulge.width + now );
//		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulge.height;
		off = ( texCoords[0][0] * ds->bulge.width + now ) / (M_PI*2);
		scale = NewSinTable(off) * ds->bulge.height;
		VectorMA( inXyz[0], scale, normal[0], outXyz[0] );
	}
}


/*
======================
RB_CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
void RB_CalcMoveVertexes( const deformStage_t *ds ) {
	int			i;
	const vecSimd_t *inXyz;
	vecSimd_t *outXyz;
	float		scale;
	vec3_t		offset;

	scale = EvalWaveForm( &ds->move.wave );

	VectorScale( ds->move.vector, scale, offset );

	inXyz = tess.xyz;
	outXyz = tess.xyz;

	for ( i = 0; i < tess.numVertexes; i++, inXyz++, outXyz++ ) {
		VectorAdd( inXyz[0], offset, outXyz[0] );
	}
}

/*
==================
GlobalVectorToLocal
==================
*/
static void GlobalVectorToLocal( const vec3_t in, vec3_t out ) {
	out[0] = DotProduct( in, backEnd.or.axis[0] );
	out[1] = DotProduct( in, backEnd.or.axis[1] );
	out[2] = DotProduct( in, backEnd.or.axis[2] );
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( void ) {
	int		i;
	int		oldVerts;
	const vecSimd_t *xyz;
	const color4ub_t *colors;
	vec3_t	mid, delta;
	float	radius;
	vec3_t	left, up;
	vec3_t	leftDir, upDir;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count", tess.shader->name );
	}

	oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	if ( backEnd.currentModel ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[1], leftDir );
		GlobalVectorToLocal( backEnd.viewParms.or.axis[2], upDir );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[1], leftDir );
		VectorCopy( backEnd.viewParms.or.axis[2], upDir );
	}

	xyz = tess.xyz;
	colors = tess.colors;
	i = oldVerts / 4;
	for ( i = oldVerts / 4 ; i >0 ; i--, xyz += 4, colors += 4) {
		// find the midpoint
		mid[0] = 0.25f * (xyz[0][0] + xyz[1][0] + xyz[2][0] + xyz[3][0] );
		mid[1] = 0.25f * (xyz[0][1] + xyz[1][1] + xyz[2][1] + xyz[3][1] );
		mid[2] = 0.25f * (xyz[0][2] + xyz[1][2] + xyz[2][2] + xyz[3][2]);

		VectorSubtract( xyz[0], mid, delta );
		radius = VectorLength( delta ) * 0.707f;		// / sqrt(2)

		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd.viewParms.isMirror ) {
			VectorSubtract( vec3_origin, left, left );
		}
		
		// compensate for scale in the axes if necessary
		if ( backEnd.currentModel && backEnd.currentModel->nonNormalizedAxes ) {
			float axisLength = VectorLength( backEnd.currentModel->axis[0] );
			if ( !axisLength ) {
				axisLength = 0;
			} else {
				axisLength = 1.0f / axisLength;
			}
			VectorScale(left, axisLength, left);
			VectorScale(up, axisLength, up);
		}
		RB_AddQuadStamp( mid, left, up, colors[0] );
	}
}


/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( void ) {
	int		i, j; //,k;
	const int *indexes;
	const vecSimd_t *inXyz;
	vecSimd_t *outXyz;
	vec3_t	forward;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.shader->name );
	}

	if ( backEnd.currentModel ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[0], forward );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[0], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	inXyz = tess.xyz;
	outXyz = tess.xyz;
	indexes = tess.indexes;

	i = tess.numVertexes >> 2;

	for ( ; i > 0; i--, inXyz += 4, outXyz += 4, indexes += 6  ) {
		float	lengths[2];
		int		nums[2];
		vec3_t	mid[2];
		vec3_t	major, minor;

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for ( j = 0 ; j < 6 ; j++ ) {
			float	l;
			vec3_t	temp;
			const float	*v1, *v2;

			v1 = inXyz[edgeVerts[j][0]];
			v2 = inXyz[edgeVerts[j][1]];

			VectorSubtract( v1, v2, temp );
			
			l = DotProduct( temp, temp );
			if ( l < lengths[0] ) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			} else if ( l < lengths[1] ) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for ( j = 0 ; j < 2 ; j++ ) {
			const float	*v1, *v2;

			v1 = inXyz[edgeVerts[nums[j]][0]];
			v2 = inXyz[edgeVerts[nums[j]][1]];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract( mid[1], mid[0], major );

		// cross this with the view direction to get minor axis
		CrossProduct( major, forward, minor );
		VectorNormalize( minor );
		
		// re-project the points
		for ( j = 0 ; j < 2 ; j++ ) {
			float	l;
			float	*v1, *v2;
			v1 = outXyz[edgeVerts[nums[j]][0]];
			v2 = outXyz[edgeVerts[nums[j]][1]];

			l = 0.5 * sqrt( lengths[j] );
			
			// we need to see which direction this edge
			// is used to determine direction of projection
#if 0
			for ( k = 0 ; k < 5 ; k++ ) {
				if ( indexes [ k ] == i + edgeVerts[nums[j]][0]
					&& indexes [k + 1 ] == i + edgeVerts[nums[j]][1] ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( mid[j], l, minor, v1 );
				VectorMA( mid[j], -l, minor, v2 );
			} else 
#endif
			{
				VectorMA( mid[j], -l, minor, v1 );
				VectorMA( mid[j], l, minor, v2 );
			}
		}
	}
}


/*
=====================
RB_DeformTessGeometry

=====================
*/
void RB_DeformTessGeometry( void ) {
	int i;
	const deformStage_t	*ds;

	for ( i = 0 ; i < tess.shader->numDeforms ; i++ ) {
		ds = &tess.shader->deforms[ i ];

		switch ( ds->deformation ) {
        case DEFORM_NONE:
            break;
		case DEFORM_NORMALS:
			RB_CalcDeformNormals( ds );
			break;
		case DEFORM_VERTEXES:
			RB_CalcDeformVertexes( ds );
			break;
		case DEFORM_BULGE:
			RB_CalcBulgeVertexes( ds );
			break;
		case DEFORM_MOVE:
			RB_CalcMoveVertexes( ds );
			break;
		case DEFORM_PROJECTION_SHADOW:
			RB_ProjectionShadowDeform();
			break;
		case DEFORM_AUTOSPRITE:
			AutospriteDeform();
			break;
		case DEFORM_AUTOSPRITE2:
			Autosprite2Deform();
			break;
		case DEFORM_TEXT0:
		case DEFORM_TEXT1:
		case DEFORM_TEXT2:
		case DEFORM_TEXT3:
		case DEFORM_TEXT4:
		case DEFORM_TEXT5:
		case DEFORM_TEXT6:
		case DEFORM_TEXT7:
//			DeformText( backEnd.refdef.text[ds->deformation - DEFORM_TEXT0] );
			break;
		}
	}
}

/*
====================================================================

COLORS

====================================================================
*/


/*
** RB_CalcColorFromEntity
*/
void RB_CalcColorFromEntity( unsigned char *dstColors ) {
	int *pColors = ( int * ) dstColors;
	int c = * ( int * ) backEnd.entityColor;
	int	i;
	for ( i = 0; i < tess.numVertexes; i++, pColors++ ) {
		*pColors = c;
	}
}

/*
** RB_CalcColorFromOneMinusEntity
*/
void RB_CalcColorFromOneMinusEntity( unsigned char *dstColors ) {
	int	i;
	int *pColors = ( int * ) dstColors;
	unsigned char invModulate[3];

	invModulate[0] = 255 - backEnd.entityColor[0];
	invModulate[1] = 255 - backEnd.entityColor[1];
	invModulate[2] = 255 - backEnd.entityColor[2];
	invModulate[3] = backEnd.entityColor[3];

	for ( i = 0; i < tess.numVertexes; i++, pColors++ ) {
		*pColors = * ( int * ) invModulate;
	}
}

/*
** RB_CalcAlphaFromEntity
*/
void RB_CalcAlphaFromEntity( unsigned char *dstColors ) {
	int	i;
	byte a = backEnd.entityColor[3];
	dstColors += 3;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		*dstColors = a;
	}
}

/*
** RB_CalcAlphaFromOneMinusEntity
*/
void RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors ) {
	int	i;
	byte a = 255 - backEnd.entityColor[3];
	dstColors += 3;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		*dstColors = a;
	}
}

/*
** RB_CalcWaveColor
*/
void RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors ) {
	int i;
	int v;
	float glow;
	int *colors = ( int * ) dstColors;
	byte	color[4];

	if ( wf->table != tr.noiseTable) {
		glow = EvalWaveForm( wf ) * tr.identityLight;
	} else {
		double timeFraction = (backEnd.refdef ? backEnd.refdef->timeFraction : 0.0);
		//CANATODO, nothing really shader doesn't use a repeating wave form :(
		double noiseTime = (double)tess.shaderTime * ( 1.0 / wf->interval ) + timeFraction * ( 1.0 / wf->interval );
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, noiseTime ) * wf->amplitude;
	}

	v = Q_ftol( 255 * glow );
	if ( v < 0 ) {
		v = 0;
	}
	else if ( v > 255 ) {
		v = 255;
	}
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int *)color;
	
	for ( i = 0; i < tess.numVertexes; i++, colors++ ) {
		*colors = v;
	}
}

/*
** RB_CalcWaveAlpha
*/
void RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors ) {
	float glow = EvalWaveFormClamped( wf );
	int v = 255 * glow;
	int i;
	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 ) {
		dstColors[3] = v;
	}
}

/*
** RB_CalcModulateColorsByFog
*/
void RB_CalcModulateColorsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}

/*
** RB_CalcModulateAlphasByFog
*/
void RB_CalcModulateAlphasByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[3] *= f;
	}
}

/*
** RB_CalcModulateRGBAsByFog
*/
void RB_CalcModulateRGBAsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
		colors[3] *= f;
	}
}


/*
====================================================================

TEX COORDS

====================================================================
*/

/*
========================
RB_CalcFogTexCoords

To do the clipped fog plane really correctly, we should use
projected textures, but I don't trust the drivers and it
doesn't fit our shader data.
========================
*/
void RB_CalcFogTexCoords( float *st ) {
	int			i;
	const float	*v;
	float		s, t;
	float		eyeT;
	qboolean	eyeOutside;
	fog_t		*fog;
	vec3_t		local;
	vec4_t		fogDistanceVector, fogDepthVector = {0, 0, 0, 0};

	fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	VectorSubtract( backEnd.or.origin, backEnd.viewParms.or.origin, local );
	fogDistanceVector[0] = -backEnd.or.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.or.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.or.modelMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.or.axis[0] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] + 
			fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] * backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] + 
			fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] * backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] + 
			fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] * backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.or.origin, fog->surface );

		eyeT = DotProduct( backEnd.or.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		eyeT = 1;	// non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	if ( eyeT < 0 ) {
		eyeOutside = qtrue;
	} else {
		eyeOutside = qfalse;
	}

	fogDistanceVector[3] += 1.0/512;

	// calculate density for each point
	for (i = 0, v = tess.xyz[0] ; i < tess.numVertexes ; i++, v += VEC_SIMD ) {
		// calculate the length in fog
		s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[3];
		t = DotProduct( v, fogDepthVector ) + fogDepthVector[3];

		// partially clipped fogs use the T axis		
		if ( eyeOutside ) {
			if ( t < 1.0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 1.0/32 + 30.0/32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		} else {
			if ( t < 0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 31.0/32;
			}
		}

		st[0] = s;
		st[1] = t;
		st += 2;
	}
}



/*
** RB_CalcEnvironmentTexCoords
*/
void RB_CalcEnvironmentTexCoords( float *st ) {
	int			i;
	const float	*v, *normal;
	vec3_t		viewer, reflected;
	float		d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += VEC_SIMD, normal += VEC_SIMD, st += 2 ) {
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcTurbulentTexCoords
*/
void RB_CalcTurbulentTexCoords( const waveForm_t *wf, const float *srcTexCoords, float *dstTexCoords ) {
	int i;
	float now = EvalWaveIndex( wf, 0 );
	const float	*xyz = tess.xyz[0];
	for ( i = 0; i < tess.numVertexes; i++, srcTexCoords += 2, dstTexCoords += 2, xyz += VEC_SIMD ) {
//		dstTexCoords[0] = srcTexCoords[0] + tr.sinTable[ ( ( int ) ( ( ( xyz[0] + xyz[2] )* 1.0/128 * 0.125) * FUNCTABLE_SIZE ) + now ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
//		dstTexCoords[1] = srcTexCoords[1] + tr.sinTable[ ( ( int ) ( ( xyz[1] * 1.0/128 * 0.125 ) * FUNCTABLE_SIZE ) + now) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		dstTexCoords[0] = srcTexCoords[0] + NewSinTable( ( xyz[0] + xyz[2] )* 1.0/128 * 0.125 + now / FUNCTABLE_SIZE ) * wf->amplitude;
		dstTexCoords[1] = srcTexCoords[1] + NewSinTable( xyz[1] * 1.0/128 * 0.125 + now / FUNCTABLE_SIZE) * wf->amplitude;
	}
}

/*
** RB_CalcScaleTexCoords
*/
void RB_CalcScaleTexCoords( const float scale[2], const float *srcTexCoords, float *dstTexCoords ) {
	int i;
	for ( i = 0; i < tess.numVertexes; i++, srcTexCoords += 2, dstTexCoords += 2 ) {
		dstTexCoords[0] = srcTexCoords[0] * scale[0];
		dstTexCoords[1] = srcTexCoords[1] * scale[1];
	}
}

/*
** RB_CalcScrollTexCoords
*/
void RB_CalcScrollTexCoords( const texModScroll_t *scroll,  const float *srcTexCoords, float *dstTexCoords ) {
	int i;
	float adjustedScrollS, adjustedScrollT;
	float timeFraction = backEnd.refdef ? backEnd.refdef->timeFraction : 0.0f;

	adjustedScrollS = (tess.shaderTime % scroll->interval[0]) * scroll->rescale[0] + timeFraction * scroll->rescale[0];
	adjustedScrollT = (tess.shaderTime % scroll->interval[1]) * scroll->rescale[1] + timeFraction * scroll->rescale[1];

	for ( i = 0; i < tess.numVertexes; i++, srcTexCoords += 2, dstTexCoords += 2 ) {
		dstTexCoords[0] = srcTexCoords[0] + adjustedScrollS;
		dstTexCoords[1] = srcTexCoords[1] + adjustedScrollT;
	}
}

/*
** RB_CalcTransformTexCoords
*/
void RB_CalcTransformTexCoords( const texModTransform_t *tf, const float *srcTexCoords, float *dstTexCoords ) {
	int i;

	for ( i = 0; i < tess.numVertexes; i++, srcTexCoords += 2, dstTexCoords += 2 )
	{
		float s = srcTexCoords[0];
		float t = srcTexCoords[1];

		dstTexCoords[0] = s * tf->matrix[0][0] + t * tf->matrix[1][0] + tf->translate[0];
		dstTexCoords[1] = s * tf->matrix[0][1] + t * tf->matrix[1][1] + tf->translate[1];
	}
}

/*
** RB_CalcRotateTexCoords
*/
void RB_CalcRotateTexCoords( const texModRotate_t *rotate, const float *srcTexCoords, float *dstTexCoords ) {
	float index, sinValue, cosValue;
	texModTransform_t tf;
	float timeFraction = backEnd.refdef ? backEnd.refdef->timeFraction : 0.0f;

	index = (((float)(tess.shaderTime % rotate->interval) + timeFraction) * (float)rotate->scale) / (float)rotate->interval;
	index /= FUNCTABLE_SIZE;

	sinValue = NewSinTable(index);
	cosValue = NewCosTable(index);

	tf.matrix[0][0] = cosValue;
	tf.matrix[1][0] = -sinValue;
	tf.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tf.matrix[0][1] = sinValue;
	tf.matrix[1][1] = cosValue;
	tf.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords( &tf, srcTexCoords, dstTexCoords  );
}





/*
#if id386 && !( (defined __GNUC__ ) && (defined __i386__ ) ) // rb010123

long myftol( float f ) {
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
}

#endif
*/
/*
** RB_CalcSpecularAlpha
**
** Calculates specular coefficient and places it in the alpha channel
*/
vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

void RB_CalcSpecularAlpha( unsigned char *alphas ) {
	int			i;
	const float		*v, *normal;
	vec3_t		viewer,  reflected;
	float		l, d;
	int			b;
	vec3_t		lightDir;
	int			numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += VEC_SIMD, normal += VEC_SIMD, alphas += 4) {
		float ilength;

		VectorSubtract( lightOrigin, v, lightDir );
//		ilength = Q_rsqrt( DotProduct( lightDir, lightDir ) );
		VectorNormalizeFast( lightDir );

		// calculate the specular color
		d = DotProduct (normal, lightDir);
//		d *= ilength;

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
		l = DotProduct (reflected, viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		} else {
			l = l*l;
			l = l*l;
			b = l * 255;
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc
*/
void RB_CalcDiffuseColor( unsigned char *colors ) {
	int				i, j;
	const float		*normal;
	float			incoming;
	const sceneModel_t	*ent;
	int				ambientLightInt;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;
	int				numVertexes;

	ent = backEnd.currentModel;
	if (!ent)
		return;

	ambientLightInt = ent->ambientLightInt;
	VectorCopy( ent->ambientLight, ambientLight );
	VectorCopy( ent->directedLight, directedLight );
	VectorCopy( ent->lightDir, lightDir );
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, normal += VEC_SIMD) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			*(int *)&colors[i*4] = ambientLightInt;
			continue;
		} 
		j = Q_ftol( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+0] = j;

		j = Q_ftol( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+1] = j;

		j = Q_ftol( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+2] = j;

		colors[i*4+3] = 255;
	}
}

