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

#include "tr_mme.h"
#include "../client/snd_public.h"

static char *workAlloc = 0;
static char *workAlign = 0;
static int workSize, workUsed;
static qboolean allocFailed = qfalse;

typedef struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t shot, depth, stencil;
	float	jitter[BLURMAX][2];
} blurData_t;

typedef struct {
	qboolean		take;
	float			fps;
	mmeShot_t		main, stencil, depth;
	float			jitter[BLURMAX][2];
} shotData_t;

//Data to contain the blurring factors
typedef struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t dof;
	float	focus, radius;
	float	jitter[BLURMAX][2];
} passData_t;

typedef struct {
	int		index;
	int		side;
	void	*data;
} cubeData_t;

typedef struct {
	blurData_t blurData;
	shotData_t shotData;
	passData_t passData;
	cubeData_t cubeData;
} workData_t;

typedef struct {
	workData_t mainWork, stereoWork;
	int pixelCount;
} mainData_t;

static mainData_t mainData;

// MME cvars
cvar_t	*mme_aviFormat;
cvar_t	*mme_screenShotFormat;
cvar_t	*mme_screenShotGamma;
cvar_t	*mme_screenShotAlpha;
cvar_t	*mme_jpegQuality;
cvar_t	*mme_jpegDownsampleChroma;
cvar_t	*mme_jpegOptimizeHuffman;
cvar_t	*mme_tgaCompression;
cvar_t	*mme_pngCompression;
cvar_t	*mme_skykey;
cvar_t	*mme_worldShader;
cvar_t	*mme_pip;
cvar_t	*mme_blurFrames;
cvar_t	*mme_blurFrames;
cvar_t	*mme_blurType;
cvar_t	*mme_blurOverlap;
cvar_t	*mme_blurGamma;
cvar_t	*mme_blurJitter;
cvar_t	*mme_blurStrength;

cvar_t	*mme_dofFrames;
cvar_t	*mme_dofRadius;

cvar_t	*mme_cpuSSE2;
cvar_t	*mme_pbo;

cvar_t	*mme_renderWidth;
cvar_t	*mme_renderHeight;
cvar_t	*mme_workMegs;
cvar_t	*mme_depthFocus;
cvar_t	*mme_depthRange;
cvar_t	*mme_captureName;
cvar_t	*mme_saveOverwrite;
cvar_t	*mme_saveShot;
cvar_t	*mme_saveStencil;
cvar_t	*mme_saveDepth;
cvar_t	*mme_saveCubemap;

cvar_t  *mme_pipeCommand;
cvar_t  *mme_combineStereoShots;

static ID_INLINE workData_t *work( qboolean stereo ) {
	return !stereo ? &mainData.mainWork : &mainData.stereoWork;
}

static void R_MME_MakeCubeData( cubeData_t *cubeData, int size ) {
	memset( cubeData, 0, sizeof( *cubeData ) );
	size = (size + 15) & ~15;

	if ( mme_saveCubemap->integer ) {
		cubeData->data = (workAlign + workUsed);
		workUsed += size;
		if ( workUsed > workSize ) {
			ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
		}
	}
}

static void R_MME_MakeBlurBlock( mmeBlurBlock_t *block, int size, mmeBlurControl_t* control ) {
	memset( block, 0, sizeof( *block ) );
	size = (size + 15) & ~15;
	block->count = size / sizeof ( __m64 );
	block->control = control;

	if ( control->totalFrames ) {
		//Allow for floating point buffer with sse
		block->accum = (__m64 *)(workAlign + workUsed);
		workUsed += size * 4;
		if ( workUsed > workSize ) {
			ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
		}
	} 
	if ( control->overlapFrames ) {
		block->overlap = (__m64 *)(workAlign + workUsed);
		workUsed += control->overlapFrames * size;
		if ( workUsed > workSize ) {
			ri.Error( ERR_FATAL, "Failed to allocate %d bytes from the mme_workMegs buffer\n", workUsed );
		}
	}
}

static void R_MME_CreateBlur( qboolean stereo ) {
	int width, height;
	int pixelCount, glPixelCount, blurTotal, passTotal, side;
	workData_t *w = work(stereo);
	mmeBlurControl_t* blurControl = &w->blurData.control;
	mmeBlurControl_t* passControl = &w->passData.control;

	if ( !mme_saveCubemap->integer ) {
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
		side = 0;
	} else {
		side = height = width = min( glConfig.vidHeight, glConfig.vidWidth );
		if ( abs( mme_saveCubemap->integer ) == 2 ) {
			width *= 6;
		} else {
			height *= 6;
		}
	}
	glPixelCount = glConfig.vidHeight * glConfig.vidWidth;
	pixelCount = height * width;

	blurTotal = mme_blurFrames->integer + mme_blurOverlap->integer ;
	passTotal = mme_dofFrames->integer;

	if ( (mme_saveCubemap->modified || mme_blurType->modified || passTotal != passControl->totalFrames ||  blurTotal != blurControl->totalFrames || pixelCount != mainData.pixelCount || blurControl->overlapFrames != mme_blurOverlap->integer) && !allocFailed ) {
		if (!stereo)
			workUsed = 0;
		
		mainData.pixelCount = pixelCount;

		blurCreate( blurControl, mme_blurType->string, blurTotal );
		blurControl->totalFrames = blurTotal;
		blurControl->totalIndex = 0;
		blurControl->overlapFrames = mme_blurOverlap->integer; 
		blurControl->overlapIndex = 0;

		R_MME_MakeBlurBlock( &w->blurData.shot, pixelCount * 3, blurControl );
		R_MME_MakeBlurBlock( &w->blurData.stencil, glPixelCount * 1, blurControl );
		R_MME_MakeBlurBlock( &w->blurData.depth, glPixelCount * 1, blurControl );

		R_MME_JitterTable( w->blurData.jitter[0], blurTotal );

		//Multi pass data
		blurCreate( passControl, "median", passTotal );
		passControl->totalFrames = passTotal;
		passControl->totalIndex = 0;
		passControl->overlapFrames = 0;
		passControl->overlapIndex = 0;
		R_MME_MakeBlurBlock( &w->passData.dof, glPixelCount * 3, passControl );
		R_MME_JitterTable( w->passData.jitter[0], passTotal );

		R_MME_MakeCubeData( &w->cubeData, pixelCount * 3 );
		w->cubeData.side = side;
	}
}

static void R_MME_CheckCvars( void ) {
	if (mme_blurFrames->integer > BLURMAX) {
		ri.Cvar_Set( "mme_blurFrames", va( "%d", BLURMAX) );
	} else if (mme_blurFrames->integer < 0) {
		ri.Cvar_Set( "mme_blurFrames", "0" );
	}

	if (mme_blurOverlap->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_blurOverlap", va( "%d", BLURMAX) );
	} else if (mme_blurOverlap->integer < 0 ) {
		ri.Cvar_Set( "mme_blurOverlap", "0");
	}
	
	if (mme_dofFrames->integer > BLURMAX ) {
		ri.Cvar_Set( "mme_dofFrames", va( "%d", BLURMAX) );
	} else if (mme_dofFrames->integer < 0 ) {
		ri.Cvar_Set( "mme_dofFrames", "0");
	}

	R_MME_CreateBlur( qfalse );
	R_MME_CreateBlur( qtrue );

	mme_saveCubemap->modified = qfalse;
	mme_blurOverlap->modified = qfalse;
	mme_blurType->modified = qfalse;
	mme_blurFrames->modified = qfalse;
	mme_dofFrames->modified = qfalse;
}

/* each loop LEFT shotData.take becomes true, but we don't want it when taking RIGHT (stereo) screenshot,
because we may want pause, and it will continue taking LEFT screenshot (and that's wrong) */
void R_MME_DoNotTake( void ) {
	work(qfalse)->shotData.take = qfalse;
}

/* Some multicapturing logic is done before the call of RC_CAPTURE
but we need to set takeShot to qtrue for that logic to be processed */
void R_MME_PrepareMultiCapture( const void *data ) {
	commandHeader_t *header = (commandHeader_t *)data;
	if ( !mme_saveCubemap->integer && !mme_dofFrames->integer && !r_stereoSeparation->value )
		return;
	while ( 1 ) {
		int cmd = header->commandId;
		data = header + 1;
		header = (commandHeader_t *)(((char *)header) + header->size);
		switch ( cmd ) {
		case RC_CAPTURE:
			R_MME_CaptureShotCmd( data );
			return;
		case RC_END_OF_LIST:
			return;
		default:
			break;
		}
	}
}

qboolean R_MME_JitterOrigin( float *x, float *y, qboolean stereo ) {
	workData_t *w = work(stereo);
	mmeBlurControl_t* passControl = &w->passData.control;
	*x = 0;
	*y = 0;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return qfalse;
	}
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = w->passData.focus;
		float radius = w->passData.radius;
		R_MME_ClampDof(&focus, &radius);
		scale = radius * R_MME_FocusScale(focus);
		*x = scale * w->passData.jitter[i][0];
		*y = -scale * w->passData.jitter[i][1];
		return qtrue;
	} 
	return qfalse;
}

void R_MME_JitterView( float *pixels, float *eyes, qboolean stereo ) {
	workData_t *w = work(stereo);
	mmeBlurControl_t* blurControl = &w->blurData.control;
	mmeBlurControl_t* passControl = &w->passData.control;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return;
	}
	if ( blurControl->totalFrames ) {
		int i = blurControl->totalIndex;
		pixels[0] = mme_blurJitter->value * w->blurData.jitter[i][0];
		pixels[1] = mme_blurJitter->value * w->blurData.jitter[i][1];
	}
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;
		float focus = w->passData.focus;
		float radius = w->passData.radius;
		R_MME_ClampDof(&focus, &radius);
		scale = r_znear->value / focus;
		scale *= radius * R_MME_FocusScale(focus);;
		eyes[0] = scale * w->passData.jitter[i][0];
		eyes[1] = scale * w->passData.jitter[i][1];
	}

}

qboolean R_MME_CubemapActive( qboolean stereo ) {
	workData_t *w = work(stereo);
	if ( !mme_saveCubemap->integer )
		return qfalse;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return qfalse;
	}
	return qtrue;
}

qboolean R_MME_CubemapIndex( int *index, qboolean stereo ) {
//	*index = mme_saveCubemap->integer;
//	return qtrue;
	workData_t *w = work(stereo);
	if ( !mme_saveCubemap->integer )
		return qfalse;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return qfalse;
	}
	*index = w->cubeData.index;
	return qtrue;
}

int R_MME_CubemapNext( qboolean stereo ) {
	workData_t *w = work(stereo);
	cubeData_t* cubeData = &w->cubeData;
	byte* outAlloc;
	byte *outAlign;
	int index;
	if ( !mme_saveCubemap->integer )
		return 0;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return 0;
	}

	index = cubeData->index;
	R_MME_GetShot( (byte *)cubeData->data + ( index * cubeData->side * cubeData->side * 3 ), w->shotData.main.type, qtrue );
	
	tr.capturingMultiPass = qtrue;
	if ( ++(cubeData->index) < 6 ) {
		int nextIndex = cubeData->index;
		if ( ++(nextIndex) >= 6 && ( ( r_stereoSeparation->value == 0.0f && !stereo ) || stereo ) )
			tr.latestMultiPassFrame = qtrue;
		return 1;
	}
	cubeData->index = 0;
	return 0;
}

int R_MME_MultiPassNext( qboolean stereo ) {
	workData_t *w = work(stereo);
	mmeBlurControl_t* control = &w->passData.control;
	byte* outAlloc;
	byte *outAlign;
	int index;
	if ( !w->shotData.take || ( tr.finishStereo && !stereo ) ) {
		if ( !stereo )
			w->shotData.take = qfalse;
		return 0;
	}
	if ( !control->totalFrames )
		return 0;

	index = control->totalIndex;
	outAlloc = (byte *)ri.Hunk_AllocateTempMemory( mainData.pixelCount * 3 + 16);
	outAlign = (byte *)((((intptr_t)(outAlloc)) + 15) & ~15);

//	GLimp_EndFrame();
	R_MME_GetShot( outAlign, w->shotData.main.type, qfalse );
	R_MME_BlurAccumAdd( &w->passData.dof, outAlign );
	
	tr.capturingMultiPass = qtrue;

	ri.Hunk_FreeTempMemory( outAlloc );
	if ( ++(control->totalIndex) < control->totalFrames ) {
		int nextIndex = control->totalIndex;
		if ( ++(nextIndex) >= control->totalFrames && ( ( r_stereoSeparation->value == 0.0f && !stereo ) || stereo ) )
			tr.latestMultiPassFrame = qtrue;
		return 1;
	}
	control->totalIndex = 0;
	R_MME_BlurAccumShift( &w->passData.dof );
	return 0;
}

static void R_MME_MultiShot( byte * target, qboolean stereo ) {
	workData_t *w = work(stereo);
	if ( mme_saveCubemap->integer ) {
		int channels = 3, step, size, i;
		int index;
		cubeData_t *cubeData = &w->cubeData;
		step = cubeData->side * channels;
		size = cubeData->side * cubeData->side * channels;
		if ( abs( mme_saveCubemap->integer ) == 2 ) {
			for ( i = 0; i < size; i += step) {
				for ( index = 0; index < 6; index++ ) {
					Com_Memcpy( target + i*6 + index*step, (byte *)cubeData->data + ( index * size ) + i, step );
				}
			}
		} else {
			Com_Memcpy( target, (byte *)cubeData->data, size*6 );
		}
	} else if ( !w->passData.control.totalFrames ) {
		R_MME_GetShot( target, w->shotData.main.type, qfalse );
	} else {
		Com_Memcpy( target, w->passData.dof.accum, mainData.pixelCount * 3 );
	}
}

qboolean R_MME_TakeShot( qboolean stereo ) {
	int width, height;
	int pixelCount, glPixelCount;
	byte inSound[MME_SAMPLERATE] = {0};
	int sizeSound = 0;
	qboolean audio = qfalse, audioTaken = qfalse;
	qboolean doGamma;
	qboolean takingStereo = r_stereoSeparation->value != 0.0f;
	qboolean workStereo;
	workData_t *w = work(stereo);
	mmeBlurControl_t* blurControl = &w->blurData.control;

	if ( !w->shotData.take || allocFailed || ( tr.finishStereo && !stereo ) )
		return qfalse;
	w->shotData.take = qfalse;

	if ( takingStereo )
		audioTaken = (qboolean)!stereo;
	if ( takingStereo && mme_combineStereoShots->integer )
		workStereo = qfalse;
	else
		workStereo = stereo;

	if ( !mme_saveCubemap->integer ) {
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	} else {
		height = width = w->cubeData.side;
		if ( abs( mme_saveCubemap->integer ) == 2 ) {
			width *= 6;
		} else {
			height *= 6;
		}
	}
	glPixelCount = glConfig.vidHeight * glConfig.vidWidth;
	pixelCount = height * width;

	doGamma = ( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && (glConfig.deviceSupportsGamma );
	R_MME_CheckCvars();
	//Special early version using the framebuffer
	if ( mme_saveShot->integer && blurControl->totalFrames > 0 &&
		R_FrameBuffer_Blur( blurControl->Float[ blurControl->totalIndex ], blurControl->totalIndex, blurControl->totalFrames ) ) {
		byte *shotBuf;
		float fps;
		if ( ++(blurControl->totalIndex) < blurControl->totalFrames ) 
			return qtrue;
		blurControl->totalIndex = 0;
		shotBuf = ri.Hunk_AllocateTempMemory( pixelCount * 3 );
		R_MME_MultiShot( shotBuf, stereo );
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );

		fps = w->shotData.fps / ( blurControl->totalFrames );
		if (!audioTaken)
			audio = ri.S_MMEAviImport(inSound, &sizeSound);
		R_MME_SaveShot( &work(workStereo)->shotData.main, width, height, fps, shotBuf, audio, sizeSound, inSound, stereo );
		ri.Hunk_FreeTempMemory( shotBuf );
		return qtrue;
	}

	if (w->shotData.fps < 0.0f) {
		byte *shotBuf = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 5 );
		R_MME_MultiShot( shotBuf, stereo );
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );
		R_MME_SaveShot( &work(workStereo)->shotData.main, width, height, w->shotData.fps, shotBuf, qfalse, -1, inSound, stereo );
		ri.Hunk_FreeTempMemory( shotBuf );
		Com_sprintf( w->shotData.main.name, sizeof( w->shotData.main.name ), "%s", w->shotData.main.nameOld );
		return qtrue;
	}
	/* Test if we need to do blurred shots */
	if ( blurControl->totalFrames > 0 ) {
		mmeBlurBlock_t *blurShot = &w->blurData.shot;
		mmeBlurBlock_t *blurDepth = &w->blurData.depth;
		mmeBlurBlock_t *blurStencil = &w->blurData.stencil;

		/* Test if we blur with overlapping frames */
		if ( blurControl->overlapFrames ) {
			/* First frame in a sequence, fill the buffer with the last frames */
			if (blurControl->totalIndex == 0) {
				int i;
				for ( i = 0; i < blurControl->overlapFrames; i++ ) {
					if ( mme_saveShot->integer ) {
						R_MME_BlurOverlapAdd( blurShot, i );
					}
					if ( mme_saveDepth->integer ) {
						R_MME_BlurOverlapAdd( blurDepth, i );
					}
					if ( mme_saveStencil->integer ) {
						R_MME_BlurOverlapAdd( blurStencil, i );
					}
					blurControl->totalIndex++;
				}
			}
			if ( mme_saveShot->integer == 1 ) {
				byte* shotBuf = R_MME_BlurOverlapBuf( blurShot );
				R_MME_MultiShot( shotBuf, stereo ); 
				if ( doGamma && mme_blurGamma->integer ) {
					R_GammaCorrect( shotBuf, width * height * 3 );
				}
				R_MME_BlurOverlapAdd( blurShot, 0 );
			}
			if ( mme_saveDepth->integer == 1 ) {
				R_MME_GetDepth( R_MME_BlurOverlapBuf( blurDepth ) ); 
				R_MME_BlurOverlapAdd( blurDepth, 0 );
			}
			if ( mme_saveStencil->integer == 1 ) {
				R_MME_GetStencil( R_MME_BlurOverlapBuf( blurStencil ) ); 
				R_MME_BlurOverlapAdd( blurStencil, 0 );
			}
			blurControl->overlapIndex++;
			blurControl->totalIndex++;
		} else {
			byte *outAlloc;
			__m64 *outAlign;
			outAlloc = ri.Hunk_AllocateTempMemory( pixelCount * 3 + 16);
			outAlign = (__m64 *)((((int)(outAlloc)) + 15) & ~15);

			if ( mme_saveShot->integer == 1 ) {
				R_MME_MultiShot( (byte*)outAlign, stereo );
				if ( doGamma && mme_blurGamma->integer ) {
					R_GammaCorrect( (byte *) outAlign, pixelCount * 3 );
				}
				R_MME_BlurAccumAdd( blurShot, outAlign );
			}

			if ( mme_saveDepth->integer == 1 ) {
				R_MME_GetDepth( (byte *)outAlign );
				R_MME_BlurAccumAdd( blurDepth, outAlign );
			}

			if ( mme_saveStencil->integer == 1 ) {
				R_MME_GetStencil( (byte *)outAlign );
				R_MME_BlurAccumAdd( blurStencil, outAlign );
			}
			ri.Hunk_FreeTempMemory( outAlloc );
			blurControl->totalIndex++;
		}

		if ( blurControl->totalIndex >= blurControl->totalFrames ) {
			float fps;
			blurControl->totalIndex = 0;

			fps = w->shotData.fps / ( blurControl->totalFrames );
		
			if ( mme_saveShot->integer == 1 ) {
				R_MME_BlurAccumShift( blurShot );
				if (doGamma && !mme_blurGamma->integer)
					R_GammaCorrect( (byte *)blurShot->accum, pixelCount * 3);
			}
			if ( mme_saveDepth->integer == 1 )
				R_MME_BlurAccumShift( blurDepth );
			if ( mme_saveStencil->integer == 1 )
				R_MME_BlurAccumShift( blurStencil );
		
			if (!audioTaken)
				audio = ri.S_MMEAviImport(inSound, &sizeSound);
			audioTaken = qtrue;
			// Big test for an rgba shot
			if ( mme_saveShot->integer == 1 && w->shotData.main.type == mmeShotTypeRGBA ) {
				int i, j;
				byte *alphaShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 4);
				byte *rgbData = (byte *)(blurShot->accum );
				if ( mme_saveDepth->integer == 1 ) {
					byte *depthData = (byte *)( blurDepth->accum );
					for ( i = 0;i < pixelCount; i++ ) {
						alphaShot[i*4+0] = rgbData[i*3+0];
						alphaShot[i*4+1] = rgbData[i*3+1];
						alphaShot[i*4+2] = rgbData[i*3+2];
						alphaShot[i*4+3] = depthData[i];
					}
				} else if ( mme_saveStencil->integer == 1) {
					byte *stencilData = (byte *)( blurStencil->accum );
					for ( i = 0;i < pixelCount; i++ ) {
						alphaShot[i*4+0] = rgbData[i*3+0];
						alphaShot[i*4+1] = rgbData[i*3+1];
						alphaShot[i*4+2] = rgbData[i*3+2];
						alphaShot[i*4+3] = stencilData[i];
					}
				}
				R_MME_SaveShot( &work(workStereo)->shotData.main, width, height, fps, alphaShot, audio, sizeSound, inSound, stereo );
				ri.Hunk_FreeTempMemory( alphaShot );
			} else {
				if ( mme_saveShot->integer == 1 )
					R_MME_SaveShot( &work(workStereo)->shotData.main, width, height, fps, (byte *)( blurShot->accum ), audio, sizeSound, inSound, stereo );
				if ( mme_saveDepth->integer == 1 )
					R_MME_SaveShot( &work(workStereo)->shotData.depth, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurDepth->accum ), audio, sizeSound, inSound, stereo );
				if ( mme_saveStencil->integer == 1 )
					R_MME_SaveShot( &work(workStereo)->shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurStencil->accum), audio, sizeSound, inSound, stereo );
			}
		}
	} 
	if ( mme_saveShot->integer > 1 || (!blurControl->totalFrames && mme_saveShot->integer )) {
		byte *shotBuf = (byte *)ri.Hunk_AllocateTempMemory(pixelCount * 5 );
		R_MME_MultiShot( shotBuf, stereo );
		
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );

		if ( w->shotData.main.type == mmeShotTypeRGBA ) {
			int i;
			byte *alphaBuf = shotBuf + pixelCount * 4;
			if ( mme_saveDepth->integer > 1 || (!blurControl->totalFrames && mme_saveDepth->integer )) {
				R_MME_GetDepth( alphaBuf );
			} else if ( mme_saveStencil->integer > 1 || (!blurControl->totalFrames && mme_saveStencil->integer )) {
				R_MME_GetStencil( alphaBuf );
			}
			for ( i = pixelCount - 1 ; i >= 0; i-- ) {
				shotBuf[i * 4 + 0] = shotBuf[i*3 + 0];
				shotBuf[i * 4 + 1] = shotBuf[i*3 + 1];
				shotBuf[i * 4 + 2] = shotBuf[i*3 + 2];
				shotBuf[i * 4 + 3] = alphaBuf[i];
			}
		}
		if (!audioTaken)
			audio = ri.S_MMEAviImport(inSound, &sizeSound);
		audioTaken = qtrue;
		R_MME_SaveShot( &work(workStereo)->shotData.main, width, height, w->shotData.fps, shotBuf, audio, sizeSound, inSound, stereo );
		ri.Hunk_FreeTempMemory( shotBuf );
	}

	if ( w->shotData.main.type == mmeShotTypeRGB || w->shotData.main.type == mmeShotTypeBGR ) {
		if ( mme_saveStencil->integer > 1 || ( !blurControl->totalFrames && mme_saveStencil->integer) ) {
			byte *stencilShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetStencil( stencilShot );
			if (!audioTaken && ((mme_saveStencil->integer > 1 && mme_saveShot->integer > 1)
				|| (mme_saveStencil->integer == 1 && mme_saveShot->integer == 1)))
				audio = ri.S_MMEAviImport(inSound, &sizeSound);
			R_MME_SaveShot( &w->shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, w->shotData.fps, stencilShot, audio, sizeSound, inSound, stereo );
			ri.Hunk_FreeTempMemory( stencilShot );
		}
		if ( mme_saveDepth->integer > 1 || ( !blurControl->totalFrames && mme_saveDepth->integer) ) {
			byte *depthShot = (byte *)ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetDepth( depthShot );
			if (!audioTaken && ((mme_saveDepth->integer > 1 && mme_saveShot->integer > 1)
				|| (mme_saveDepth->integer == 1 && mme_saveShot->integer == 1)))
				audio = ri.S_MMEAviImport(inSound, &sizeSound);
			R_MME_SaveShot( &w->shotData.depth, glConfig.vidWidth, glConfig.vidHeight, w->shotData.fps, depthShot, audio, sizeSound, inSound, stereo );
			ri.Hunk_FreeTempMemory( depthShot );
		}
	}
	return qtrue;
}

static void R_MME_PrepareCapture( const captureCommand_t *cmd, qboolean stereo ) {
	workData_t *w = work(stereo);
	/* Can happen with multicapture */
	if (w->shotData.take)
		return;
	w->shotData.take = qtrue;
	w->shotData.fps = cmd->fps;
	w->passData.focus = cmd->focus;
	w->passData.radius = cmd->radius;
	if (w->shotData.fps < 0.0f) { //called from screenshot command for special dof screenshot
		Com_sprintf( w->shotData.main.name, sizeof( w->shotData.main.name ), "%s", cmd->name );
		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			w->shotData.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			w->shotData.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			w->shotData.main.format = mmeShotFormatPNG;
		} else {
			w->shotData.main.format = mmeShotFormatPNG;
		}
		w->shotData.main.type = mmeShotTypeRGB;
	} else if (strcmp( cmd->name, w->shotData.main.name) || mme_screenShotFormat->modified || mme_screenShotAlpha->modified || mme_saveCubemap->modified || mme_aviFormat->modified ) {
		/* Also reset the the other data */
		w->blurData.control.totalIndex = 0;
		if ( workAlign )
			Com_Memset( workAlign, 0, workUsed );
		Com_sprintf( w->shotData.main.name, sizeof( w->shotData.main.name ), "%s", cmd->name );
		Com_sprintf( w->shotData.main.nameOld, sizeof( w->shotData.main.nameOld ), "%s", cmd->name );
		Com_sprintf( w->shotData.depth.name, sizeof( w->shotData.depth.name ), "%s.depth", cmd->name );
		Com_sprintf( w->shotData.stencil.name, sizeof( w->shotData.stencil.name ), "%s.stencil", cmd->name );
		
		/* Reset on the second pass */
		if ( stereo ) {
			mme_screenShotFormat->modified = qfalse;
			mme_screenShotAlpha->modified = qfalse;
			mme_aviFormat->modified = qfalse;
		}

		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			w->shotData.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			w->shotData.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			w->shotData.main.format = mmeShotFormatPNG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "avi")) {
			w->shotData.main.format = mmeShotFormatAVI;
        } else if (!Q_stricmp(mme_screenShotFormat->string, "pipe")) {
			w->shotData.main.format = mmeShotFormatPIPE;
		} else {
			w->shotData.main.format = mmeShotFormatTGA;
		}
		
		//grayscale works fine only with compressed avi :(
		if ((w->shotData.main.format != mmeShotFormatAVI && w->shotData.main.format != mmeShotFormatPIPE)) {
			w->shotData.depth.format = mmeShotFormatPNG;
			w->shotData.stencil.format = mmeShotFormatPNG;
		} else if (w->shotData.main.format == mmeShotFormatAVI) {
			w->shotData.depth.format = mmeShotFormatAVI;
			w->shotData.stencil.format = mmeShotFormatAVI;
		} else if (w->shotData.main.format == mmeShotFormatPIPE) {
			w->shotData.depth.format = mmeShotFormatPIPE;
			w->shotData.stencil.format = mmeShotFormatPIPE;
		}

		if ((w->shotData.main.format == mmeShotFormatAVI && !mme_aviFormat->integer) || w->shotData.main.format == mmeShotFormatPIPE) {
			w->shotData.main.type = mmeShotTypeBGR;
		} else {
			w->shotData.main.type = mmeShotTypeRGB;
		}
		if ( mme_screenShotAlpha->integer && !mme_saveCubemap->integer ) {
			if ( w->shotData.main.format == mmeShotFormatPNG )
				w->shotData.main.type = mmeShotTypeRGBA;
			else if ( w->shotData.main.format == mmeShotFormatTGA )
				w->shotData.main.type = mmeShotTypeRGBA;
		}

		w->shotData.main.counter = -1;
		w->shotData.main.stereoTemp = NULL;
		w->shotData.depth.type = mmeShotTypeGray;
		w->shotData.depth.counter = -1;
		w->shotData.depth.stereoTemp = NULL;
		w->shotData.stencil.type = mmeShotTypeGray;
		w->shotData.stencil.counter = -1;
		w->shotData.stencil.stereoTemp = NULL;
	}
}

const void *R_MME_CaptureShotCmd( const void *data ) {
	const captureCommand_t *cmd = (const captureCommand_t *)data;

	if (!cmd->name[0])
		return (const void *)(cmd + 1);
	R_MME_PrepareCapture( cmd, qfalse );
	R_MME_PrepareCapture( cmd, qtrue );
	R_MME_CheckCvars();
	return (const void *)(cmd + 1);	
}

void R_MME_Capture( const char *shotName, float fps, float focus, float radius ) {
	captureCommand_t *cmd;
	
	if ( !tr.registered || !fps ) {
		return;
	}
	cmd = R_GetCommandBuffer( RC_CAPTURE, sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	if (mme_dofFrames->integer > 0 || r_stereoSeparation->value != 0.0f || mme_saveCubemap->integer > 0)
		tr.capturingMultiPass = qtrue;
	cmd->fps = fps;
	cmd->focus = focus;
	cmd->radius = radius;
	Q_strncpyz( cmd->name, shotName, sizeof( cmd->name ));
}

void R_MME_BlurInfo( int* total, int *index ) {
	*total = mme_blurFrames->integer;
	*index = work(qfalse)->blurData.control.totalIndex;
	if (*index )
		*index -= work(qfalse)->blurData.control.overlapFrames;
}

void R_MME_Shutdown(void) {
	aviClose( &work(qfalse)->shotData.main.avi );
	aviClose( &work(qfalse)->shotData.depth.avi );
	aviClose( &work(qfalse)->shotData.stencil.avi );
	aviClose( &work(qtrue)->shotData.main.avi );
	aviClose( &work(qtrue)->shotData.depth.avi );
	aviClose( &work(qtrue)->shotData.stencil.avi );
}

void R_MME_Init(void) {

	// MME cvars
	mme_combineStereoShots = ri.Cvar_Get ("mme_combineStereoShots", "1", CVAR_ARCHIVE);
	mme_pipeCommand = ri.Cvar_Get ("mme_pipeCommand", "auto", CVAR_ARCHIVE);
	mme_aviFormat = ri.Cvar_Get ("mme_aviFormat", "0", CVAR_ARCHIVE);
	mme_jpegQuality = ri.Cvar_Get ("mme_jpegQuality", "90", CVAR_ARCHIVE);
	mme_jpegDownsampleChroma = ri.Cvar_Get ("mme_jpegDownsampleChroma", "0", CVAR_ARCHIVE);
	mme_jpegOptimizeHuffman = ri.Cvar_Get ("mme_jpegOptimizeHuffman", "1", CVAR_ARCHIVE);
	mme_screenShotFormat = ri.Cvar_Get ("mme_screenShotFormat", "tga", CVAR_ARCHIVE);
	mme_screenShotGamma = ri.Cvar_Get ("mme_screenShotGamma", "0", CVAR_ARCHIVE);
	mme_screenShotAlpha = ri.Cvar_Get ("mme_screenShotAlpha", "0", CVAR_ARCHIVE);
	mme_tgaCompression = ri.Cvar_Get ("mme_tgaCompression", "1", CVAR_ARCHIVE);
	mme_pngCompression = ri.Cvar_Get("mme_pngCompression", "5", CVAR_ARCHIVE);
	mme_skykey = ri.Cvar_Get( "mme_skykey", "0", CVAR_ARCHIVE );
	mme_pip = ri.Cvar_Get( "mme_pip", "0", CVAR_CHEAT );
	mme_worldShader = ri.Cvar_Get( "mme_worldShader", "0", CVAR_CHEAT );
	mme_renderWidth = ri.Cvar_Get( "mme_renderWidth", "0", CVAR_LATCH | CVAR_ARCHIVE );
	mme_renderHeight = ri.Cvar_Get( "mme_renderHeight", "0", CVAR_LATCH | CVAR_ARCHIVE );

	mme_blurFrames = ri.Cvar_Get ( "mme_blurFrames", "0", CVAR_ARCHIVE );
	mme_blurOverlap = ri.Cvar_Get ("mme_blurOverlap", "0", CVAR_ARCHIVE );
	mme_blurType = ri.Cvar_Get ( "mme_blurType", "gaussian", CVAR_ARCHIVE );
	mme_blurGamma = ri.Cvar_Get ( "mme_blurGamma", "0", CVAR_ARCHIVE );
	mme_blurJitter = ri.Cvar_Get ( "mme_blurJitter", "1", CVAR_ARCHIVE );
	mme_blurStrength = ri.Cvar_Get ( "mme_blurStrength", "0", CVAR_ARCHIVE );

	mme_dofFrames = ri.Cvar_Get ( "mme_dofFrames", "0", CVAR_ARCHIVE );
	mme_dofRadius = ri.Cvar_Get ( "mme_dofRadius", "2", CVAR_ARCHIVE );

	mme_cpuSSE2 = ri.Cvar_Get ( "mme_cpuSSE2", "1", CVAR_ARCHIVE );
	mme_pbo = ri.Cvar_Get ( "mme_pbo", "1", CVAR_ARCHIVE );
	
	mme_depthRange = ri.Cvar_Get ( "mme_depthRange", "0", CVAR_ARCHIVE );
	mme_depthFocus = ri.Cvar_Get ( "mme_depthFocus", "0", CVAR_ARCHIVE );
	mme_saveOverwrite = ri.Cvar_Get ( "mme_saveOverwrite", "0", CVAR_ARCHIVE );
	mme_saveStencil = ri.Cvar_Get ( "mme_saveStencil", "0", CVAR_ARCHIVE );
	mme_saveDepth = ri.Cvar_Get ( "mme_saveDepth", "0", CVAR_ARCHIVE );
	mme_saveShot = ri.Cvar_Get ( "mme_saveShot", "1", CVAR_ARCHIVE );
	mme_saveCubemap = ri.Cvar_Get ( "mme_saveCubemap", "0", CVAR_ARCHIVE );
	mme_workMegs = ri.Cvar_Get ( "mme_workMegs", "128", CVAR_LATCH | CVAR_ARCHIVE );

	mme_worldShader->modified = qtrue;
	mme_saveCubemap->modified = qtrue;

	Com_Memset( &mainData, 0, sizeof(mainData));
	//CANATODO, not exactly the best way to do this probably, but it works
	if (!workAlloc) {
		workSize = mme_workMegs->integer;
		if (workSize < 64)
			workSize = 64;
		else if (workSize > MME_MAX_WORKSIZE)
			workSize = MME_MAX_WORKSIZE;
		workSize *= 1024 * 1024;
		workAlloc = calloc( workSize + 16, 1 );
		if (!workAlloc) {
			ri.Printf(PRINT_ALL, "Failed to allocate %d bytes for mme work buffer\n", workSize );
			return;
		}
		workAlign = (char *)(((int)workAlloc + 15) & ~15);
	}
	R_MME_CheckCvars();
}

