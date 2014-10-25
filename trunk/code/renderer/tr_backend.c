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

backEndData_t	*backEndData[SMP_FRAMES];
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
	int texnum;

	if ( !image ) {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		image->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture (GL_TEXTURE_2D, texnum);
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit )
{
	if ( glState.currenttmu == unit )
	{
		return;
	}

	if ( unit == 0 )
	{
		qglActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE0_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE0_ARB )\n" );
	}
	else if ( unit == 1 )
	{
		qglActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE1_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE1_ARB )\n" );
	} else {
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}


/*
** GL_BindMultitexture
*/
void GL_BindMultitexture( image_t *image0, GLuint env0, image_t *image1, GLuint env1 ) {
	int		texnum0, texnum1;

	texnum0 = image0->texnum;
	texnum1 = image1->texnum;

	if ( glState.currenttextures[1] != texnum1 ) {
		GL_SelectTexture( 1 );
		image1->frameUsed = tr.frameCount;
		glState.currenttextures[1] = texnum1;
		qglBindTexture( GL_TEXTURE_2D, texnum1 );
	}
	if ( glState.currenttextures[0] != texnum0 ) {
		GL_SelectTexture( 0 );
		image0->frameUsed = tr.frameCount;
		glState.currenttextures[0] = texnum0;
		qglBindTexture( GL_TEXTURE_2D, texnum0 );
	}
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qglEnable( GL_CULL_FACE );

		if ( cullType == CT_BACK_SIDED )
		{
			if ( backEnd.viewParms.isMirror )
			{
				qglCullFace( GL_FRONT );
			}
			else
			{
				qglCullFace( GL_BACK );
			}
		}
		else
		{
			if ( backEnd.viewParms.isMirror )
			{
				qglCullFace( GL_BACK );
			}
			else
			{
				qglCullFace( GL_FRONT );
			}
		}
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env )
{
	if ( env == glState.texEnv[glState.currenttmu] )
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
	case GL_MODULATE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case GL_REPLACE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		break;
	case GL_DECAL:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		break;
	case GL_ADD:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
		break;
	default:
		ri.Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed\n", env );
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned long stateBits )
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		GLenum srcFactor, dstFactor;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
		{
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;		// to get warning to shut up
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits\n" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;		// to get warning to shut up
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits\n" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		}
		else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		switch ( stateBits & GLS_ATEST_BITS )
		{
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	glState.glStateBits = stateBits;
}



/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef->time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}

static void SetFinalProjection( void ) {
	float	xmin, xmax, ymin, ymax;
	float	width, height, depth;
	float	zNear, zFar, zProj, stereoSep;
	float	dx, dy;
	vec2_t	pixelJitter, eyeJitter;
	
	//
	// set up projection matrix
	//
	zNear	= r_znear->value;
	zFar	= backEnd.viewParms.zFar;
	
	zProj	= r_zproj->value;
	stereoSep = r_stereoSeparation->value / 100.0f;

	ymax = zNear * tan( backEnd.viewParms.fovY * M_PI / 360.0f );
	ymin = -ymax;

	xmax = zNear * tan( backEnd.viewParms.fovX * M_PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;
	depth = zFar - zNear;

	pixelJitter[0] = pixelJitter[1] = 0;
	eyeJitter[0] = eyeJitter[1] = 0;
	/* Jitter the view */
	if ( stereoSep <= 0.0f) {
		R_MME_JitterView( pixelJitter, eyeJitter );
	} else if ( stereoSep > 0.0f) {
		R_MME_JitterViewStereo( pixelJitter, eyeJitter );
	}

	dx = ( pixelJitter[0]*width ) / backEnd.viewParms.viewportWidth;
	dy = ( pixelJitter[1]*height ) / backEnd.viewParms.viewportHeight;
	dx += eyeJitter[0];
	dy += eyeJitter[1];

	xmin += dx; xmax += dx;
	ymin += dy; ymax += dy;

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
	qglGetFloatv(GL_PROJECTION_MATRIX, backEnd.viewParms.projectionMatrix );
	qglPopMatrix();

	backEnd.viewParms.projectionMatrix[0] = 2 * zNear / width;
	backEnd.viewParms.projectionMatrix[4] = 0;
	backEnd.viewParms.projectionMatrix[8] = ( xmax + xmin + 2 * stereoSep ) / width;	// normally 0
	backEnd.viewParms.projectionMatrix[12] = 2 * zProj * stereoSep / width;

	backEnd.viewParms.projectionMatrix[1] = 0;
	backEnd.viewParms.projectionMatrix[5] = 2 * zNear / height;
	backEnd.viewParms.projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	backEnd.viewParms.projectionMatrix[13] = 0;

	backEnd.viewParms.projectionMatrix[2] = 0;
	backEnd.viewParms.projectionMatrix[6] = 0;
	backEnd.viewParms.projectionMatrix[10] = -( zFar + zNear ) / depth;
	backEnd.viewParms.projectionMatrix[14] = -2 * zFar * zNear / depth;

	backEnd.viewParms.projectionMatrix[3] = 0;
	backEnd.viewParms.projectionMatrix[7] = 0;
	backEnd.viewParms.projectionMatrix[11] = -1;
	backEnd.viewParms.projectionMatrix[15] = 0;
}


static void SetViewportAndScissor( void ) {
	qglMatrixMode(GL_PROJECTION);

	SetFinalProjection();
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );

	qglMatrixMode(GL_MODELVIEW);
	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT | GLS_DEPTHTEST_DISABLE );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 || mme_saveStencil->integer )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if ( !( backEnd.refdef->rdflags & RDF_NOWORLDMODEL )) {
		if (mme_skykey->string[0] != '0') {
			vec3_t skyColor;
			clearBits |= GL_COLOR_BUFFER_BIT;
			Q_parseColor( mme_skykey->string, defaultColors, skyColor );
			qglClearColor( skyColor[0], skyColor[1], skyColor[2], 1.0f );
		} else if (r_fastsky->integer ) {
#ifdef _DEBUG
			qglClearColor( 0.8f, 0.7f, 0.4f, 1.0f );	// FIXME: get color of sky
#else
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );	// FIXME: get color of sky
#endif	
			clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
		}
	}

	qglClear( clearBits );

	if ( backEnd.refdef->rdflags & RDF_HYPERSPACE ) {
		RB_Hyperspace();
		return;
	} else {
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;		// force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
		float	plane[4];
		double	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.or.origin) - plane[3];

		qglLoadMatrixf( s_flipMatrix );
		qglClipPlane (GL_CLIP_PLANE0, plane2);
		qglEnable (GL_CLIP_PLANE0);
	} else {
		qglDisable (GL_CLIP_PLANE0);
	}
}


#define	MAC_EVENT_PUMP_MSEC		5

/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader;
	int				fogNum;
	qboolean		depthRange, oldDepthRange;
	int				i;
	drawSurf_t		*drawSurf;
	int				oldSort;
	const surfaceType_t	*surface;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	// Prepare initial values that will trigger the needed settings
	backEnd.currentModel = (sceneModel_t*)-1;
	oldDepthRange = qfalse;
	oldSort = -1;
	depthRange = qfalse;

	// Clear for endsurface first run
	tess.numIndexes = 0;

	backEnd.pc.c_surfaces += numDrawSurfs;
	if (!(backEnd.refdef->rdflags & RDF_NOWORLDMODEL)) {
		backEnd.sceneZfar = backEnd.viewParms.zFar;
	}

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		/* Rendering a model? */
		if ( drawSurf->sort & QSORT_HAVEMODEL_MASK ) {
			const drawModel_t *drawModel;
			unsigned int index;

			RB_EndSurface();

			oldSort = drawSurf->sort;
			drawModel = (drawModel_t*) drawSurf->data;
			backEnd.currentModel = drawModel->model;
			index = R_SortToIndex( oldSort );
			surface = drawModel->surface[index];
			// set up the transformation matrix
			R_RotateForEntity( backEnd.currentModel, &backEnd.viewParms, &backEnd.or );
			R_TransformDlights( backEnd.refdef->numDlights, backEnd.refdef->dlights, &backEnd.or );
			//Load the temporary matrix to show the model
			qglLoadMatrixf( backEnd.or.modelMatrix );

			depthRange = 0;
			//figure this stuff out now and store it
			if ( backEnd.currentModel->renderfx & RF_NODEPTH ) {
				depthRange = 2;
			} else if ( backEnd.currentModel->renderfx & RF_DEPTHHACK ) {
				depthRange = 1;
			}
			if ( oldDepthRange != depthRange ) {
				oldDepthRange = depthRange;
				switch ( depthRange ) {
				default:
				case 0:
					qglDepthRange (0, 1);	
					break;
				case 1:
					qglDepthRange (0, .3);	
					break;
				case 2:
					qglDepthRange (0, 0);
					break;
				}
			}
			shader = R_SortToShader( oldSort );
			fogNum = R_SortToFog( oldSort );
			RB_BeginSurface( shader, fogNum );
			if ( mme_saveStencil->integer == 1) {
				if  ( backEnd.currentModel->renderfx & RF_STENCIL) {
					if (!backEnd.doingStencil) {
						qglStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
						backEnd.doingStencil = qtrue;
					}
				} else {
					if (backEnd.doingStencil) {
						qglStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );
						backEnd.doingStencil = qfalse;
					}
				}
			}
			rb_surfaceTable[ *surface ]( surface );
			continue;
		} 
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			rb_surfaceTable[ *((byte *)drawSurf->data) ]( drawSurf->data );
			continue;
		}
		RB_EndSurface();
		oldSort = drawSurf->sort;
		shader = R_SortToShader( oldSort );
		fogNum = R_SortToFog( oldSort );
		RB_BeginSurface( shader, fogNum );
		if ( backEnd.currentModel ) {
			if (backEnd.doingStencil) {
				qglStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );
				backEnd.doingStencil = qfalse;
			}
			backEnd.currentModel = 0;
			backEnd.or = backEnd.viewParms.world;
			R_TransformDlights( backEnd.refdef->numDlights, backEnd.refdef->dlights, &backEnd.or );
			qglLoadMatrixf( backEnd.or.modelMatrix );
			if ( oldDepthRange ) {
				depthRange = 0;
				oldDepthRange = 0;
				qglDepthRange (0, 1);
			}
		}
		rb_surfaceTable[ *((byte *)drawSurf->data) ]( drawSurf->data );
		if ( mme_saveStencil->integer == 1) {
			if  (backEnd.currentModel && backEnd.currentModel->renderfx & RF_STENCIL) {
				if (!backEnd.doingStencil) {
					qglStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
					backEnd.doingStencil = qtrue;
				}
			} else {
				if (backEnd.doingStencil) {
					qglStencilOp( GL_KEEP, GL_KEEP, GL_ZERO );
					backEnd.doingStencil = qfalse;
				}
			}
		}
		// add the triangles for this surface
	}

//	if ( backEnd.currentModel && tess.numVertexes ) 
//		Com_Printf( "Shader batch for a model?\n" );
	// draw the contents of the last shader batch
	RB_EndSurface();

	// go back to the world modelview matrix
	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange (0, 1);
	}

#if 0
	RB_DrawSun();
#endif
	// darken down any stencil shadows
	RB_ShadowFinish();		

	// add light flares on lights that aren't obscured
	RB_RenderFlares();
}


/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	if ( tess.numIndexes | tess.numVertexes )
		Com_Printf(" Going 2d with verts?" );
	backEnd.projection2D = qtrue;
	backEnd.currentModel = 0;
	
	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	tess.shaderTime = ri.Milliseconds();
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;

	if ( !tr.registered ) {
		return;
	}
	R_SyncRenderThread();

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = end = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	qglBegin (GL_QUADS);
	qglTexCoord2f ( 0.5f / cols,  0.5f / rows );
	qglVertex2f (x, y);
	qglTexCoord2f ( ( cols - 0.5f ) / cols ,  0.5f / rows );
	qglVertex2f (x+w, y);
	qglTexCoord2f ( ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
	qglVertex2f (x+w, y+h);
	qglTexCoord2f ( 0.5f / cols, ( rows - 0.5f ) / rows );
	qglVertex2f (x, y+h);
	qglEnd ();
}

void RE_UploadCinematic (int w2, int h2, int cols, int rows, const byte *data, int client, qboolean dirty) {

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
//		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
//		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );	

		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
//		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RB_SetColor

=============
*/
static void RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;
}

/*
=============
RB_StretchPic
=============
*/
static void RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int *indexes;
	vec2_t *texCoords;
	vecSimd_t *xyz;
	color4ub_t *colors;

	cmd = (const stretchPicCommand_t *)data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		RB_EndSurface();
		RB_BeginSurface( shader, 0 );
	}

	RB_CheckOverflow( 4, 6 );

	indexes = tess.indexes + tess.numIndexes;
	indexes[0] = tess.numVertexes + 3;
	indexes[1] = tess.numVertexes + 0;
	indexes[2] = tess.numVertexes + 2;
	indexes[3] = tess.numVertexes + 2;
	indexes[4] = tess.numVertexes + 0;
	indexes[5] = tess.numVertexes + 1;

	colors = tess.colors + tess.numVertexes;
	Byte4Copy( backEnd.color2D, colors[0] );
	Byte4Copy( backEnd.color2D, colors[1] );
	Byte4Copy( backEnd.color2D, colors[2] );
	Byte4Copy( backEnd.color2D, colors[3] );

	xyz = tess.xyz + tess.numVertexes;
	VectorSet( xyz[0], cmd->x, cmd->y, 0 );
	VectorSet( xyz[1], cmd->x + cmd->w, cmd->y, 0 );
	VectorSet( xyz[2], cmd->x + cmd->w, cmd->y + cmd->h, 0 );
	VectorSet( xyz[3], cmd->x, cmd->y + cmd->h, 0 );

	texCoords = tess.texCoords + tess.numVertexes;

	texCoords[0][0] = cmd->s1;
	texCoords[0][1] = cmd->t1;
	texCoords[1][0] = cmd->s2;
	texCoords[1][1] = cmd->t1;
	texCoords[2][0] = cmd->s2;
	texCoords[2][1] = cmd->t2;
	texCoords[3][0] = cmd->s1;
	texCoords[3][1] = cmd->t2;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

}


/*
=============
RB_DrawSurfs

=============
*/
static void RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;
	//Jitter the camera origin
	if ( !backEnd.viewParms.isPortal && !(backEnd.refdef->rdflags & RDF_NOWORLDMODEL) ) {
		int i;
		float x, y;
		if ( (r_stereoSeparation->value <= 0 && R_MME_JitterOrigin( &x, &y ))
			|| (r_stereoSeparation->value > 0 && R_MME_JitterOriginStereo( &x, &y ))) {
			orientationr_t* or = &backEnd.viewParms.or;
			orientationr_t* world = &backEnd.viewParms.world;

//			VectorScale( or->axis[0], 0.5, or->axis[0] );
//			VectorScale( or->axis[1], 0.3, or->axis[1] );
//			VectorScale( or->axis[2], 0.8, or->axis[2] );
			VectorMA( or->origin, x, or->axis[1], or->origin );
			VectorMA( or->origin, y, or->axis[2], or->origin );
//			or->origin[2] += 4000;
//			or->origin[2] += 0.1 * x;
			R_RotateForWorld( or, world );
			for ( i = 0; i < 16; i++ ) {
				int r = (rand() & 0xffff ) - 0x4000;
				//world->modelMatrix[i] *= (0.9 + r * 0.0001);
				//or->modelMatrix[i] *= (0.9 + r * 0.0001);
			}
		} else { 	
			for ( i = 0; i < 16; i++ ) {
//				int r = (rand() & 0xffff ) - 0x4000;
//				backEnd.viewParms.world.modelMatrix[i] *= (0.9 + r * 0.0001);
			}
		}
	}
	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
}


/*
=============
RB_DrawBuffer

=============
*/
static void RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer( cmd->buffer );

	R_FrameBuffer_StartFrame();

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	start = ri.Milliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );
		qglBegin (GL_QUADS);
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}


/*
=============
RB_SwapBuffers

=============
*/
static int RB_SwapBuffers( const void *data ) {

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	backEnd.projection2D = qfalse;
	
	tr.capturingDofOrStereo = qfalse;
	tr.latestDofOrStereoFrame = qfalse;

	/* Take and merge DOF frames */
	if ( r_stereoSeparation->value <= 0.0f && !tr.finishStereo) {
		if ( R_MME_MultiPassNext() ) {
			return 1;
		}
	} else if ( r_stereoSeparation->value > 0.0f) {
		if ( R_MME_MultiPassNextStereo() ) {
			return 1;
		}
	}
	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}

	if ( !glState.finishCalled ) {
		qglFinish();
	}
	/* Allow MME to take a screenshot */
	if ( r_stereoSeparation->value < 0.0f && tr.finishStereo) {
		tr.capturingDofOrStereo = qtrue;
		tr.latestDofOrStereoFrame = qtrue;
		Cvar_SetValue("r_stereoSeparation", -r_stereoSeparation->value);
		return 1;
	} else if ( r_stereoSeparation->value <= 0.0f) {
		if ( R_MME_TakeShot( ) && r_stereoSeparation->value != 0.0f) {
			tr.capturingDofOrStereo = qtrue;
			tr.latestDofOrStereoFrame = qfalse;
			Cvar_SetValue("r_stereoSeparation", -r_stereoSeparation->value);
			tr.finishStereo = qtrue;
			return 1;
		}
	} else if ( r_stereoSeparation->value > 0.0f) {
		if ( tr.finishStereo) {
			R_MME_TakeShotStereo( );
			R_MME_DoNotTake( );
			Cvar_SetValue("r_stereoSeparation", -r_stereoSeparation->value);
			tr.finishStereo = qfalse;
		}
	}

	R_FrameBuffer_EndFrame();

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	return 0;
}

/*
====================
RB_ExecuteRenderCommands

This function will be called synchronously if running without
smp extensions, or asynchronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( const void *oldData ) {
	int		t1, t2;
	commandHeader_t *header;
	const void* data;

	t1 = ri.Milliseconds ();

	if ( !r_smp->integer || oldData == backEndData[0]->data ) {
		backEnd.smpFrame = 0;
	} else {
		backEnd.smpFrame = 1;
	}
again:
	data = oldData;
	header = (commandHeader_t *)data;
	backEnd.doneBloom = qfalse;
	while ( 1 ) {
		int cmd = header->commandId;
		data = header + 1;
		header = (commandHeader_t *)(((char *)header) + header->size);
		switch ( cmd ) {
		case RC_SET_COLOR:
			RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			R_BloomScreen();
			RB_StretchPic( data );
			break;
		case RC_DRAW_SURFS:
			backEnd.doneSurfaces = qtrue;
			RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			R_BloomScreen();
			if ( RB_SwapBuffers( data ) )
				goto again;
			break;
		case RC_SCREENSHOT:
			RB_ScreenShotCmd( data );
			break;
		case RC_CAPTURE:
			R_MME_CaptureShotCmd( data );
			break;
		case RC_CAPTURE_STEREO:
			R_MME_CaptureShotCmdStereo( data );
			break;
		case RC_ALLOC:
			break;
		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			t2 = ri.Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}
}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void ) {
	const void	*data;

	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if ( !data ) {
			return;	// all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = qfalse;
	}
}

