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
// tr_shade.c

#include "tr_local.h"
#if idppc_altivec
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
================
R_ArrayElementDiscrete

This is just for OpenGL conformance testing, it should never be the fastest
================
*/
static void APIENTRY R_ArrayElementDiscrete( GLint index ) {
	qglColor4ubv( tess.colors[ index ] );
	if ( glState.currenttmu ) {
		qglMultiTexCoord2fARB( 0, tess.stage.texCoords[0][index][0], tess.stage.texCoords[0][index][1] );
		qglMultiTexCoord2fARB( 1, tess.stage.texCoords[1][index][0], tess.stage.texCoords[1][index][1] );
	} else {
		qglTexCoord2fv( tess.stage.texCoords[0][index] );
	}
	qglVertex3fv( tess.xyz[ index ] );
}

/*
===================
R_DrawStripElements

===================
*/
static int		c_vertexes;		// for seeing how long our average strips are
static int		c_begins;
static void R_DrawStripElements( int numIndexes, const glIndex_t *indexes, void ( APIENTRY *element )(GLint) ) {
	int i;
	int last[3] = { -1, -1, -1 };
	qboolean even;

	c_begins++;

	if ( numIndexes <= 0 ) {
		return;
	}

	qglBegin( GL_TRIANGLE_STRIP );

	// prime the strip
	element( indexes[0] );
	element( indexes[1] );
	element( indexes[2] );
	c_vertexes += 3;

	last[0] = indexes[0];
	last[1] = indexes[1];
	last[2] = indexes[2];

	even = qfalse;

	for ( i = 3; i < numIndexes; i += 3 )
	{
		// odd numbered triangle in potential strip
		if ( !even )
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( indexes[i+0] == last[2] ) && ( indexes[i+1] == last[1] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;
				assert( indexes[i+2] < tess.numVertexes );
				even = qtrue;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );

				c_vertexes += 3;

				even = qfalse;
			}
		}
		else
		{
			// check previous triangle to see if we're continuing a strip
			if ( ( last[2] == indexes[i+1] ) && ( last[0] == indexes[i+0] ) )
			{
				element( indexes[i+2] );
				c_vertexes++;

				even = qfalse;
			}
			// otherwise we're done with this strip so finish it and start
			// a new one
			else
			{
				qglEnd();

				qglBegin( GL_TRIANGLE_STRIP );
				c_begins++;

				element( indexes[i+0] );
				element( indexes[i+1] );
				element( indexes[i+2] );
				c_vertexes += 3;

				even = qfalse;
			}
		}

		// cache the last three vertices
		last[0] = indexes[i+0];
		last[1] = indexes[i+1];
		last[2] = indexes[i+2];
	}

	qglEnd();
}

static void R_DrawFlatElements( int numIndexes, const glIndex_t *indexes ) {
	int tris;

	for( tris = 0; tris < numIndexes; tris += 3 ) {
		int i;
		const color4ub_t* color0 = &tess.stage.colors[ indexes[ tris + 0 ] ];
		const color4ub_t* color1 = &tess.stage.colors[ indexes[ tris + 1 ] ];
		const color4ub_t* color2 = &tess.stage.colors[ indexes[ tris + 2 ] ];

		color4ub_t colorFinal;
		colorFinal[0] = ( (*color0)[0] + (*color1)[0] + (*color2)[0] ) / 3;
		colorFinal[1] = ( (*color0)[1] + (*color1)[1] + (*color2)[1] ) / 3;
		colorFinal[2] = ( (*color0)[2] + (*color1)[2] + (*color2)[2] ) / 3;
		colorFinal[3] = ( (*color0)[3] + (*color1)[3] + (*color2)[3] ) / 3;

		qglBegin( GL_TRIANGLES );
		
		for ( i = 0; i < 3; i++ ) {
			int index = indexes[ tris + i ];
			qglColor4ubv( colorFinal );
			if ( glState.currenttmu ) {
				qglMultiTexCoord2fARB( 0, tess.stage.texCoords[0][index][0], tess.stage.texCoords[0][index][1] );
				qglMultiTexCoord2fARB( 1, tess.stage.texCoords[1][index][0], tess.stage.texCoords[1][index][1] );
			} else {
				qglTexCoord2fv( tess.stage.texCoords[0][index] );
			}
			qglVertex3fv( tess.xyz[ index ] );
		}
		qglEnd();
	}
}



/*
==================
R_DrawElements

Optionally performs our own glDrawElements that looks for strip conditions
instead of using the single glDrawElements call that may be inefficient
without compiled vertex arrays.
==================
*/
static void R_DrawElements( int numIndexes, const glIndex_t *indexes ) {
	int		primitives;

	primitives = r_primitives->integer;

	// default is to use triangles if compiled vertex arrays are present
	if ( primitives == 0 ) {
		if ( qglLockArraysEXT ) {
			primitives = 2;
		} else {
			primitives = 1;
		}
	}


	if ( primitives == 2 ) {
		qglDrawElements( GL_TRIANGLES, 
						numIndexes,
						GL_INDEX_TYPE,
						indexes );
		return;
	}

	if ( primitives == 1 ) {
		R_DrawStripElements( numIndexes,  indexes, qglArrayElement );
		return;
	}
	
	if ( primitives == 3 ) {
		R_DrawStripElements( numIndexes,  indexes, R_ArrayElementDiscrete );
		return;
	}

	if ( primitives == 4 ) {
		R_DrawFlatElements( numIndexes,  indexes );
		return;
	}

	// anything else will cause no drawing
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;

static qboolean	setArraysOnce;

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage( const textureBundle_t *bundle ) {
	int		index;

	if ( bundle->isVideoMap ) {
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_Bind( bundle->image[0] );
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
//	index = myftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
//	index >>= FUNCTABLE_SIZE2;

	//Canabis, anyone use shader time offsets?
	if ( tess.shaderTime < 0 ) {
		index = 0;	// may happen with shader time offsets
	} else {
		index = tess.shaderTime / bundle->animationInterval;
		index %= bundle->numImageAnimations;
	}
	GL_Bind( bundle->image[ index ] );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris ( void ) {
	GL_Bind( tr.whiteImage );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );
	
	if (!( r_showtris->integer & 0x2) ) {
		qglDisableClientState (GL_COLOR_ARRAY);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		qglColor3f (1,1,1);
	}
	
	if ( !(r_showtris->integer & 0x4 )) {
		qglDepthRange( 0, 0 );
	}

	qglVertexPointer (3, GL_FLOAT, 4 * VEC_SIMD, tess.xyz );	// padded for SIMD

	if (qglLockArraysEXT) {
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );

	if (qglUnlockArraysEXT) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}
	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals ( void ) {
	int		i;
	vec3_t	temp;
	const float *xyz, *normal;

	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);
	qglDepthRange( 0, 0 );	// never occluded
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

	xyz = tess.xyz[0];
	normal = tess.normal[0];
	qglBegin (GL_LINES);
	for (i = 0 ; i < tess.numVertexes ; i++, normal+=VEC_SIMD, xyz+=VEC_SIMD ) {
		qglVertex3fv( xyz );
		VectorMA (xyz , 2, normal, temp);
		qglVertex3fv (temp);
	}
	qglEnd ();

	qglDepthRange( 0, 1 );
}


static void	TraceShader( void ) {
	int i;
	float *origin, *dir;
	vec3_t u, v, n, w0, w, I;
	float r, a, b, s, t;
	float uu, uv, vv, wu, wv, D;

	origin = backEnd.viewParms.or.origin;
	dir = backEnd.viewParms.or.axis[0];

	for ( i = 0; i < tess.numIndexes;i+=3 ) {
		const float *p0, *p1, *p2;

		p0 = tess.xyz[ tess.indexes[i+0]];
		p1 = tess.xyz[ tess.indexes[i+1]];
		p2 = tess.xyz[ tess.indexes[i+2]];
		VectorSubtract( p1, p0, u );
		VectorSubtract( p2, p0, v );
		
		CrossProduct( u, v, n );
		VectorSubtract( origin, p0, w0 );
		b = DotProduct( n, dir );
		if ( fabs(b) < 0.01 ) {		//Triangle is parallel to the ray
			continue;
		}
		a = -DotProduct( n, w0 );
		r = a/b;
		if ( r < 0 )		//Ray going away from triangle
			continue;
		//Create intersection point of the plane
		VectorMA( origin, r, dir, I );
		/* See if the intersect point lies within the triangle */
		VectorSubtract( I, p0, w );
		uu = DotProduct( u, u );
		uv = DotProduct( u, v );
		vv = DotProduct( v, v );
		wu = DotProduct( w, u );
		wv = DotProduct( w, v );
		D = uv * uv - uu*vv;
		s = (uv * wv - vv * wu) / D;
		if (s < 0.0f || s > 1.0f)        // I is outside T
	        continue;
		t = (uv * wu - uu * wv) / D;
		if (t < 0.0 || (s + t) > 1.0)  // I is outside T
			continue;
		t = Distance( origin, I );
		Com_Printf( "Trace impact on %s at %f\n", tess.shader->name, t );
		return;
	}
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;
	tess.needMask = state->needMask;
}

/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void DrawMultitextured( const shaderStage_t *pStage ) {
	GL_State( pStage->stateBits );

	// this is an ugly hack to work around a GeForce driver
	// bug with multitexture and clip planes
//	if ( backEnd.viewParms.isPortal ) {
//		qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
//	}

	//
	// base
	//
	GL_SelectTexture( 0 );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.stage.texCoords[0] );
	R_BindAnimatedImage( &pStage->bundle[0] );

	//
	// lightmap/secondary pass
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( tess.shader->multitextureEnv );
	}

	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.stage.texCoords[1] );

	R_BindAnimatedImage( &pStage->bundle[1] );

	R_DrawElements( tess.numIndexes, tess.indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	//qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
	qglDisable( GL_TEXTURE_2D );

	GL_SelectTexture( 0 );
}



/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
static void ProjectDlightTexture( void ) {
	int		i, l;
	vec3_t	origin;
	float	*texCoords;
	byte	*colors;
	byte	clipBits[SHADER_MAX_VERTEXES];
	float	texCoordsArray[SHADER_MAX_VERTEXES][2];
	byte	colorArray[SHADER_MAX_VERTEXES][4];
	unsigned	hitIndexes[SHADER_MAX_INDEXES];
	int		numIndexes;
	float	scale;
	float	radius;
	vec3_t	floatColor;
	float	modulate;
	int		inputIndexes;
	const dlight_t	*dl;


//	if ( !backEnd.refdef.num_dlights ) {
//		return;
//	}

	inputIndexes = tess.numIndexes/3;
	dl = backEnd.refdef->dlights;
	for ( l = 0 ; l < backEnd.refdef->numDlights ; l++, dl++) {
		const float *xyz;
		const int *indexes;
		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}
		texCoords = texCoordsArray[0];
		colors = colorArray[0];
		VectorCopy( dl->transformed, origin );
		radius = dl->radius;
		scale = 1.0f / radius;

		floatColor[0] = dl->color[0] * 255.0f;
		floatColor[1] = dl->color[1] * 255.0f;
		floatColor[2] = dl->color[2] * 255.0f;
		xyz = tess.xyz[0];
		for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4, xyz += VEC_SIMD ) {
			vec3_t	dist;
			int		clip;

			backEnd.pc.c_dlightVertexes++;

			VectorSubtract( origin, xyz, dist );
			texCoords[0] = 0.5f + dist[0] * scale;
			texCoords[1] = 0.5f + dist[1] * scale;

			clip = 0;
			if ( texCoords[0] < 0.0f ) {
				clip |= 1;
			} else if ( texCoords[0] > 1.0f ) {
				clip |= 2;
			}
			if ( texCoords[1] < 0.0f ) {
				clip |= 4;
			} else if ( texCoords[1] > 1.0f ) {
				clip |= 8;
			}
			// modulate the strength based on the height and color
			if ( dist[2] > radius ) {
				clip |= 16;
				modulate = 0.0f;
			} else if ( dist[2] < -radius ) {
				clip |= 32;
				modulate = 0.0f;
			} else {
				dist[2] = fabs(dist[2]);
				if ( dist[2] < radius * 0.5f ) {
					modulate = 1.0f;
				} else {
					modulate = 2.0f * (radius - dist[2]) * scale;
				}
			}
			clipBits[i] = clip;

			colors[0] = Q_ftol(floatColor[0] * modulate);
			colors[1] = Q_ftol(floatColor[1] * modulate);
			colors[2] = Q_ftol(floatColor[2] * modulate);
			colors[3] = 255;
		}

		// build a list of triangles that need light
		numIndexes = 0;
		indexes = tess.indexes;
		for ( i = inputIndexes ; i >0 ; i--, indexes += 3) {
			int		a, b, c;

			a = indexes[0];
			b = indexes[1];
			c = indexes[2];
			if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
				continue;	// not lighted
			}
			hitIndexes[numIndexes] = a;
			hitIndexes[numIndexes+1] = b;
			hitIndexes[numIndexes+2] = c;
			numIndexes += 3;
		}

		if ( !numIndexes ) {
			continue;
		}

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

		qglEnableClientState( GL_COLOR_ARRAY );
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

		GL_Bind( tr.dlightImage );
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		if ( dl->additive ) {
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
		else {
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
		R_DrawElements( numIndexes, hitIndexes );
		backEnd.pc.c_totalIndexes += numIndexes;
		backEnd.pc.c_dlightIndexes += numIndexes;
	}
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
	fog_t		*fog;
	int			i;

	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.storage.stageColors );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.storage.stageCoords );

	fog = tr.world->fogs + tess.fogNum;

	for ( i = 0; i < tess.numVertexes; i++ ) {
		Byte4Copy( fog->colorInt, tess.storage.stageColors[i] );
	}

	RB_CalcFogTexCoords( tess.storage.stageCoords[0][0] );

	GL_Bind( tr.fogImage );

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );
}

/*
===============
ComputeColors
===============
*/
static void ComputeColors( const shaderStage_t *pStage )
{
	int		i;

	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			tess.stage.colors = tr.colorIdentity;
			break;
		case CGEN_FULL_IDENTITY:
			tess.stage.colors = tess.storage.stageColors;
			for ( i = 0; i < tess.numVertexes; i++ ) {
				*(int *)tess.storage.stageColors[i] = 0xffffffff;
			}
			break;
		default:
		case CGEN_IDENTITY_LIGHTING:
			tess.stage.colors = tr.colorIdentityLight;
			break;
		case CGEN_FULL_IDENTITY_LIGHTING:
			tess.stage.colors = tess.storage.stageColors;
			for ( i = 0; i < tess.numVertexes; i++ ) {
				Byte4Copy( tr.identityLightColor, tess.storage.stageColors[i] );
			}
			break;
		case CGEN_LIGHTING_DIFFUSE:
			tess.stage.colors = tess.storage.stageColors;
			RB_CalcDiffuseColor( tess.storage.stageColors[0] );
			break;
		case CGEN_EXACT_VERTEX:
			tess.stage.colors = tess.colors;
			break;
		case CGEN_FULL_EXACT_VERTEX:
			tess.stage.colors = tess.storage.stageColors;
			for ( i = 0; i < tess.numVertexes; i++ ) {
				Byte4Copy( tess.colors[i], tess.storage.stageColors[i] );
			}
			break;
		case CGEN_CONST:
			tess.stage.colors = tess.storage.stageColors;
			for ( i = 0; i < tess.numVertexes; i++ ) {
				Byte4Copy( pStage->constantColor, tess.storage.stageColors[i] );
			}
			break;
		case CGEN_VERTEX:
			tess.stage.colors = tess.storage.stageColors;
			{
				const byte *inColor = tess.colors[0];
				byte *outColor = tess.storage.stageColors[0];
				int lightScale = tr.identityLightByte;

				for ( i = 0; i < tess.numVertexes; i++, outColor +=4, inColor+=4  ) {
					outColor[0] = (inColor[0] * lightScale) >> 8;
					outColor[1] = (inColor[1] * lightScale) >> 8;
					outColor[2] = (inColor[2] * lightScale) >> 8;
					outColor[3] = inColor[3];
				}
			}
			break;
		case CGEN_ONE_MINUS_VERTEX:
			tess.stage.colors = tess.storage.stageColors;
			if ( tr.identityLightByte == 255 )
			{
				const byte *inColor = tess.colors[0];
				byte *outColor = tess.storage.stageColors[0];
				for ( i = 0; i < tess.numVertexes; i++, outColor +=4, inColor+=4  ) {
					outColor[0] = 255 - inColor[0];
					outColor[1] = 255 - inColor[1];
					outColor[2] = 255 - inColor[2];
				}
			} else {
				const byte *inColor = tess.colors[0];
				byte *outColor = tess.storage.stageColors[0];
				int lightScale = tr.identityLightByte;

				for ( i = 0; i < tess.numVertexes; i++, outColor +=4, inColor+=4  ) {
					outColor[0] = (( 255 - inColor[0] ) * lightScale) >> 8;
					outColor[1] = (( 255 - inColor[1] ) * lightScale) >> 8;
					outColor[2] = (( 255 - inColor[2] ) * lightScale) >> 8;
				}
			}
			break;
		case CGEN_FOG:
			{
				const fog_t	*fog;
				fog = tr.world->fogs + tess.fogNum;
				tess.stage.colors = tess.storage.stageColors;

				for ( i = 0; i < tess.numVertexes; i++ ) {
					Byte4Copy( fog->colorInt, tess.storage.stageColors[i] );
				}
			}
			break;
		case CGEN_WAVEFORM:
			tess.stage.colors = tess.storage.stageColors;
			RB_CalcWaveColor( &pStage->rgbWave, tess.storage.stageColors[0] );
			break;
		case CGEN_ENTITY:
			tess.stage.colors = tess.storage.stageColors;
			RB_CalcColorFromEntity( tess.storage.stageColors[0] );
			break;
		case CGEN_ONE_MINUS_ENTITY:
			tess.stage.colors = tess.storage.stageColors;
			RB_CalcColorFromOneMinusEntity( tess.storage.stageColors[0] );
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
	case AGEN_SKIP:
		break;
	case AGEN_IDENTITY:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			tess.storage.stageColors[i][3] = 0xff;
		}
		break;
	case AGEN_CONST:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			tess.storage.stageColors[i][3] = pStage->constantColor[3];
		}
		break;
	case AGEN_WAVEFORM:
		RB_CalcWaveAlpha( &pStage->alphaWave, tess.storage.stageColors[0] );
		break;
	case AGEN_LIGHTING_SPECULAR:
		RB_CalcSpecularAlpha( tess.storage.stageColors[0] );
		break;
	case AGEN_ENTITY:
		RB_CalcAlphaFromEntity( tess.storage.stageColors[0] );
		break;
	case AGEN_ONE_MINUS_ENTITY:
		RB_CalcAlphaFromOneMinusEntity( tess.storage.stageColors[0] );
		break;
    case AGEN_VERTEX:
		for ( i = 0; i < tess.numVertexes; i++ ) {
			tess.storage.stageColors[i][3] = tess.colors[i][3];
		}
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ ) {
			tess.storage.stageColors[i][3] = 255 - tess.colors[i][3];
        }
        break;
	case AGEN_PORTAL:
		{
			unsigned char alpha;
			const vecSimd_t *xyz;

			xyz = tess.xyz;
			for ( i = 0; i < tess.numVertexes; i++, xyz++ )
			{
				float len;
				vec3_t v;

				VectorSubtract( xyz[0], backEnd.viewParms.or.origin, v );
				len = VectorLength( v );

				len /= tess.shader->portalRange;

				if ( len < 0 )
				{
					alpha = 0;
				}
				else if ( len > 1 )
				{
					alpha = 0xff;
				}
				else
				{
					alpha = len * 0xff;
				}

				tess.storage.stageColors[i][3] = alpha;
			}
		}
		break;
	}

	//
	// fog adjustment for colors to fade out as fog increases
	//
	//CANATODO, another check to refill the color table....
	if ( tess.fogNum ) {
		switch ( pStage->adjustColorsForFog ) {
		case ACFF_MODULATE_RGB:
			RB_CalcModulateColorsByFog( tess.storage.stageColors[0] );
			break;
		case ACFF_MODULATE_ALPHA:
			RB_CalcModulateAlphasByFog( tess.storage.stageColors[0] );
			break;
		case ACFF_MODULATE_RGBA:
			RB_CalcModulateRGBAsByFog( tess.storage.stageColors[0] );
			break;
		case ACFF_NONE:
			return;
		}
	}
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords( const shaderStage_t *pStage ) {
	int		i;
	int		b;

	for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
		int tm;

		//
		// generate the texture coordinates
		//
		switch ( pStage->bundle[b].tcGen ) {
		case TCGEN_IDENTITY:
			tess.stage.texCoords[b] = tess.storage.stageCoords[b];
			Com_Memset( tess.storage.stageCoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
			break;
		case TCGEN_TEXTURE:
			tess.stage.texCoords[b] = tess.texCoords;
			break;
		case TCGEN_LIGHTMAP:
			tess.stage.texCoords[b] = tess.lightMap;
			break;
		case TCGEN_VECTOR:
			tess.stage.texCoords[b] = tess.storage.stageCoords[b];
			for ( i = 0 ; i < tess.numVertexes ; i++ ) {
				//canatodo, prestore the xyz for crappy compiler
				tess.storage.stageCoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
				tess.storage.stageCoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
			}
			break;
		case TCGEN_FOG:
			tess.stage.texCoords[b] = tess.storage.stageCoords[b];
			RB_CalcFogTexCoords( tess.storage.stageCoords[b][0] );
			break;
		case TCGEN_ENVIRONMENT_MAPPED:
			tess.stage.texCoords[b] = tess.storage.stageCoords[b];
			RB_CalcEnvironmentTexCoords( tess.storage.stageCoords[b][0] );
			break;
		case TCGEN_BAD:
			return;
		}

		//
		// alter texture coordinates
		//
		for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) {
			switch ( pStage->bundle[b].texMods[tm].type )
			{
			case TMOD_NONE:
				tm = TR_MAX_TEXMODS;		// break out of for loop
				break;

			case TMOD_TURBULENT:
				RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].data.wave,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;

			case TMOD_ENTITY_TRANSLATE:
//				RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
//									 ( float * ) tess.svars.texcoords[b] );
				break;

			case TMOD_SCROLL:
				RB_CalcScrollTexCoords( &pStage->bundle[b].texMods[tm].data.scroll,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;

			case TMOD_SCALE:
				RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].data.scale,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;
			
			case TMOD_STRETCH:
				RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].data.wave,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;

			case TMOD_TRANSFORM:
				RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm].data.transform,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;

			case TMOD_ROTATE:
				RB_CalcRotateTexCoords( &pStage->bundle[b].texMods[tm].data.rotate,
					tess.stage.texCoords[b][0], tess.storage.stageCoords[b][0] );
				tess.stage.texCoords[b] = tess.storage.stageCoords[b];
				break;

			default:
				ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
				break;
			}
		}
	}
}

/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric( void )
{
	int stage;
	const shaderStage_t **stageList = tess.shader->stages;

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
	
		const shaderStage_t *pStage = stageList[stage];
		if ( !pStage )
		{
			break;
		}
		tess.stage.colors = tess.colors;
		tess.stage.texCoords[0] = tess.texCoords;
		tess.stage.texCoords[1] = tess.lightMap;
		ComputeColors( pStage );
		ComputeTexCoords( pStage );

		if ( !setArraysOnce )
		{
			qglEnableClientState( GL_COLOR_ARRAY );
			qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.stage.colors );
		}

		//
		// do multitexture
		//
		if ( pStage->bundle[1].image[0] != 0 )
		{
			DrawMultitextured( pStage );
		}
		else
		{
			if ( !setArraysOnce )
			{
				qglTexCoordPointer( 2, GL_FLOAT, 0, tess.stage.texCoords[0] );
			}

			R_BindAnimatedImage( &pStage->bundle[0] );

			GL_State( pStage->stateBits );

			//
			// draw
			//
			R_DrawElements( tess.numIndexes, tess.indexes );
		}
		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
		{
			break;
		}
	}
}


/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	const shader_t *shader = tess.shader;
	RB_DeformTessGeometry();

	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( shader->cullType );

	// set polygon offset if necessary
	if ( shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// if there is only a single pass then we can enable color
	// and texture arrays before we compile, otherwise we need
	// to avoid compiling those arrays since they will change
	// during multipass rendering
	//
	if ( 1 || shader->numStages > 1 || shader->multitextureEnv )
	{
		setArraysOnce = qfalse;
		qglDisableClientState (GL_COLOR_ARRAY);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		setArraysOnce = qtrue;

		qglEnableClientState( GL_COLOR_ARRAY);
		qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.colors );

		qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer( 2, GL_FLOAT, 0, tess.texCoords );
	}

	//
	// lock XYZ
	//
	qglVertexPointer (3, GL_FLOAT, 4*VEC_SIMD, tess.xyz );
	if (qglLockArraysEXT)
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	//
	// enable color and texcoord arrays after the lock if necessary
	//
	if ( !setArraysOnce )
	{
		qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
		qglEnableClientState( GL_COLOR_ARRAY );
	}

	//
	// call shader function
	//
	RB_IterateStagesGeneric( );

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && shader->sort <= SS_OPAQUE
		&& !(shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && shader->fogPass ) {
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}

	//
	// reset polygon offset
	//
	if ( shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
	const shader_t	*shader;
	shader = tess.shader;

	//
	// compute colors
	//
	RB_CalcDiffuseColor( tess.storage.stageColors[0] );
	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( shader->cullType );

	//
	// set arrays and lock
	//
	qglEnableClientState( GL_COLOR_ARRAY);
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

//	memset( tess.stage.colors, 0xff, tess.numVertexes * 4 );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.storage.stageColors[0] );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.texCoords );
	qglVertexPointer (3, GL_FLOAT, 4*VEC_SIMD, tess.xyz );

	if ( qglLockArraysEXT )
	{
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	//
	// call special shade routine
	//
	R_BindAnimatedImage( &shader->stages[0]->bundle[0] );
	GL_State( shader->stages[0]->stateBits );
	R_DrawElements( tess.numIndexes, tess.indexes );

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && shader->fogPass ) {
		RB_FogPass();
	}

	// 
	// unlock arrays
	//
	if (qglUnlockArraysEXT) 
	{
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}
}

//#define	REPLACE_MODE

void RB_StageIteratorLightmappedMultitexture( void ) {
	const shader_t *shader;

	shader = tess.shader;

	//
	// log this call
	//
	if ( r_logFile->integer ) {
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	GL_Cull( shader->cullType );

	//
	// set color, pointers, and lock
	//
	GL_State( GLS_DEFAULT );
	qglVertexPointer( 3, GL_FLOAT, 4*VEC_SIMD, tess.xyz );

#ifdef REPLACE_MODE
	qglDisableClientState( GL_COLOR_ARRAY );
	qglColor3f( 1, 1, 1 );
	qglShadeModel( GL_FLAT );
#else
	qglEnableClientState( GL_COLOR_ARRAY );
	qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tr.colorIdentity );
#endif

	//
	// select base stage
	//
	GL_SelectTexture( 0 );

	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	R_BindAnimatedImage( &shader->stages[0]->bundle[0] );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.texCoords );

	//
	// configure second stage
	//
	GL_SelectTexture( 1 );
	qglEnable( GL_TEXTURE_2D );
	if ( r_lightmap->integer ) {
		GL_TexEnv( GL_REPLACE );
	} else {
		GL_TexEnv( GL_MODULATE );
	}
	R_BindAnimatedImage( &shader->stages[0]->bundle[1] );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	qglTexCoordPointer( 2, GL_FLOAT, 0, tess.lightMap );

	//
	// lock arrays
	//
	if ( qglLockArraysEXT ) {
		qglLockArraysEXT(0, tess.numVertexes);
		GLimp_LogComment( "glLockArraysEXT\n" );
	}

	R_DrawElements( tess.numIndexes, tess.indexes );

	//
	// disable texturing on TEXTURE1, then select TEXTURE0
	//
	qglDisable( GL_TEXTURE_2D );
	qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

	GL_SelectTexture( 0 );
#ifdef REPLACE_MODE
	GL_TexEnv( GL_MODULATE );
	qglShadeModel( GL_SMOOTH );
#endif

	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
		ProjectDlightTexture();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// unlock arrays
	//
	if ( qglUnlockArraysEXT ) {
		qglUnlockArraysEXT();
		GLimp_LogComment( "glUnlockArraysEXT\n" );
	}
}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	const shader_t *shader;

	if (tess.numIndexes == 0) {
		return;
	}
	shader = tess.shader;

	if ( tess.numVertexes >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", tess.numVertexes, SHADER_MAX_VERTEXES );
	}
	if ( tess.numIndexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", tess.numIndexes, SHADER_MAX_INDEXES );
	}

	if ( shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		tess.numIndexes = 0;
		tess.numVertexes = 0;
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		tess.numIndexes = 0;
		tess.numVertexes = 0;
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * shader->numStages;

	//
	// call off to shader specific tess end function
	//
	shader->optimalStageIteratorFunc();
	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris();
	}
	if ( r_shownormals->integer ) {
		DrawNormals();
	}
	if ( backEnd.traceShader ) {
		TraceShader();
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	GLimp_LogComment( "----------\n" );
}

