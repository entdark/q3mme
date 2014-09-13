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

#include "tr_local.h"
#include "tr_mme.h"
#include <mmintrin.h>

static char *workAlloc = 0;
static char *workAlign = 0;
static int workSize, workUsed;
static qboolean allocFailed = qfalse;

static struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t shot, depth, stencil;
	float	jitter[BLURMAX][2];
} blurData;

static struct {
	qboolean		take;
	float			fps;
	float			dofFocus;
	mmeShot_t		main, stencil, depth;
	float			jitter[BLURMAX][2];
} shotData;

//Data to contain the blurring factors
static struct {
	mmeBlurControl_t control;
	mmeBlurBlock_t dof;
	float	jitter[BLURMAX][2];
} passData;

static struct {
	int pixelCount;
} mainData;

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

cvar_t	*mme_dofFrames;
cvar_t	*mme_dofRadius;

cvar_t	*mme_cpuSSE2;

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

static void R_MME_GetShot( void* output ) {
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, output ); 
}

static void R_MME_GetStencil( void *output ) {
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, output ); 
}

static void R_MME_GetDepth( byte *output ) {
	float focusStart, focusEnd, focusMul;
	float zBase, zAdd, zRange;
	int i, pixelCount;
	byte *temp;

	if ( mme_depthRange->value <= 0 )
		return;
	
	pixelCount = glConfig.vidWidth * glConfig.vidHeight;

	focusStart = mme_depthFocus->value - mme_depthRange->value;
	focusEnd = mme_depthFocus->value + mme_depthRange->value;
	focusMul = 255.0f / (2 * mme_depthRange->value);

	zRange = backEnd.sceneZfar - r_znear->value;
	zBase = ( backEnd.sceneZfar + r_znear->value ) / zRange;
	zAdd =  ( 2 * backEnd.sceneZfar * r_znear->value ) / zRange;

	temp = ri.Hunk_AllocateTempMemory( pixelCount * sizeof( float ) );
	qglDepthRange( 0.0f, 1.0f );
	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_DEPTH_COMPONENT, GL_FLOAT, temp ); 
	/* Could probably speed this up a bit with SSE but frack it for now */
	for ( i=0 ; i < pixelCount; i++ ) {
		/* Read from the 0 - 1 depth */
		float zVal = ((float *)temp)[i];
		int outVal;
		/* Back to the original -1 to 1 range */
		zVal = zVal * 2.0f - 1.0f;
		/* Back to the original z values */
		zVal = zAdd / ( zBase - zVal );
		/* Clip and scale the range that's been selected */
		if (zVal <= focusStart)
			outVal = 0;
		else if (zVal >= focusEnd)
			outVal = 255;
		else 
			outVal = (zVal - focusStart) * focusMul;
		output[i] = outVal;
	}
	ri.Hunk_FreeTempMemory( temp );
}

void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf ) {
	mmeShotFormat_t format;
	char *extension;
	char *outBuf;
	int outSize;
	char fileName[MAX_OSPATH];

	format = shot->format;
	switch (format) {
	case mmeShotFormatJPG:
		extension = "jpg";
		break;
	case mmeShotFormatTGA:
		/* Seems hardly any program can handle grayscale tga, switching to png */
		if (shot->type == mmeShotTypeGray) {
			format = mmeShotFormatPNG;
			extension = "png";
		} else {
			extension = "tga";
		}
		break;
	case mmeShotFormatPNG:
		extension = "png";
		break;
	case mmeShotFormatAVI:
		mmeAviShot( &shot->avi, shot->name, shot->type, width, height, fps, inBuf );
		return;
	}

	if (shot->counter < 0) {
		int counter = 0;
		while ( counter < 1000000000) {
			Com_sprintf( fileName, sizeof(fileName), "%s.%010d.%s", shot->name, counter, extension);
			if (!FS_FileExists( fileName ))
				break;
			if ( mme_saveOverwrite->integer ) 
				FS_FileErase( fileName );
			counter++;
		}
		if ( mme_saveOverwrite->integer ) {
			shot->counter = 0;
		} else {
			shot->counter = counter;
		}
	} 

	Com_sprintf( fileName, sizeof(fileName), "%s.%010d.%s", shot->name, shot->counter, extension );
	shot->counter++;

	outSize = width * height * 4 + 2048;
	outBuf = ri.Hunk_AllocateTempMemory( outSize );
	switch ( format ) {
	case mmeShotFormatJPG:
		outSize = SaveJPG( mme_jpegQuality->integer, width, height, shot->type, inBuf, outBuf, outSize );
		break;
	case mmeShotFormatTGA:
		outSize = SaveTGA( mme_tgaCompression->integer, width, height, shot->type, inBuf, outBuf, outSize );
		break;
	case mmeShotFormatPNG:
		outSize = SavePNG( mme_pngCompression->integer, width, height, shot->type, inBuf, outBuf, outSize );
		break;
	default:
		outSize = 0;
	}
	if (outSize)
		ri.FS_WriteFile( fileName, outBuf, outSize );
	ri.Hunk_FreeTempMemory( outBuf );
}

static void blurCreate( mmeBlurControl_t* control, const char* type, int frames ) {
	float*  blurFloat = control->Float;
	float	blurMax, strength;
	float	blurHalf = 0.5f * ( frames - 1 );
	float	bestStrength;
	float	floatTotal;
	int		passes, bestTotal;
	int		i;
	
	if (blurHalf <= 0)
		return;

	if ( !Q_stricmp( type, "gaussian")) {
		for (i = 0; i < frames ; i++) {
			double xVal = ((i - blurHalf ) / blurHalf) * 3;
			double expVal = exp( - (xVal * xVal) / 2);
			double sqrtVal = 1.0f / sqrt( 2 * M_PI);
			blurFloat[i] = sqrtVal * expVal;
		}
	} else if (!Q_stricmp( type, "triangle")) {
		for (i = 0; i < frames; i++) {
			if ( i <= blurHalf )
				blurFloat[i] = 1 + i;
			else
				blurFloat[i] = 1 + ( frames - 1 - i);
		}
	} else {
		for (i = 0; i < frames; i++) {
			blurFloat[i] = 1;
		}
	}

	floatTotal = 0;
	blurMax = 0;
	for (i = 0; i < frames; i++) {
		if ( blurFloat[i] > blurMax )
			blurMax = blurFloat[i];
		floatTotal += blurFloat[i];
	}

	floatTotal = 1 / floatTotal;
	for (i = 0; i < frames; i++) 
		blurFloat[i] *= floatTotal;

	bestStrength = 0;
	bestTotal = 0;
	strength = 128;

	/* Check for best 256 match for MMX */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < frames; i++) 
			total += strength * blurFloat[i];
		if (total > 256) {
			strength *= (256.0 / total);
		} else if (total <= 256) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (256.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < frames; i++) {
		control->MMX[i] = bestStrength * blurFloat[i];
	}

	bestStrength = 0;
	bestTotal = 0;
	strength = 128;
	/* Check for best 32768 match for MMX */
	for (passes = 32;passes >0;passes--) {
		int total = 0;
		for (i = 0; i < frames; i++) 
			total += strength * blurFloat[i];
		if ( total > 32768 ) {
			strength *= (32768.0 / total);
		} else if (total <= 32767 ) {
			if ( total > bestTotal) {
				bestTotal = total;
				bestStrength = strength;
			}
			strength *= (32768.0 / total); 
		} else {
			bestTotal = total;
			bestStrength = strength;
			break;
		}
	}
	for (i = 0; i < frames; i++) {
		control->SSE[i] = bestStrength * blurFloat[i];
	}

	control->totalIndex = 0;
	control->totalIndex = frames;
	control->overlapFrames = 0;
	control->overlapIndex = 0;

	_mm_empty();
}

void MME_AccumClearMMX( void* w, const void* r, short mul, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;
	int i; 
	__m64 readVal, zeroVal, work0, work1, multiply;
	 multiply = _mm_set1_pi16( mul );
	 zeroVal = _mm_setzero_si64();
	 for (i = count; i>0 ; i--) {
		 readVal = *reader++;
		 work0 = _mm_unpacklo_pi8( readVal, zeroVal );
		 work1 = _mm_unpackhi_pi8( readVal, zeroVal );
		 work0 = _mm_mullo_pi16( work0, multiply );
		 work1 = _mm_mullo_pi16( work1, multiply );
		 writer[0] = work0;
		 writer[1] = work1;
		 writer += 2;
	 }
	 _mm_empty();
}

void MME_AccumAddMMX( void *w, const void* r, short mul, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;
	int i;
	__m64 zeroVal, multiply;
	 multiply = _mm_set1_pi16( mul );
	 zeroVal = _mm_setzero_si64();
	 /* Add 2 pixels in a loop */
	 for (i = count ; i>0 ; i--) {
		 __m64 readVal = *reader++;
		 __m64 work0 = _mm_mullo_pi16( multiply, _mm_unpacklo_pi8( readVal, zeroVal ) );
		 __m64 work1 = _mm_mullo_pi16( multiply, _mm_unpackhi_pi8( readVal, zeroVal ) );
		 writer[0] = _mm_add_pi16( writer[0], work0 );
		 writer[1] = _mm_add_pi16( writer[1], work1 );
		 writer += 2;
	 }
	 _mm_empty();
}


void MME_AccumShiftMMX( const void  *r, void *w, int count ) {
	const __m64 * reader = (const __m64 *) r;
	__m64 *writer = (__m64 *) w;

	int i;
	__m64 work0, work1, work2, work3;
	/* Handle 2 at once */
	for (i = count/2;i>0;i--) {
		work0 = _mm_srli_pi16 (reader[0], 8);
		work1 = _mm_srli_pi16 (reader[1], 8);
		work2 = _mm_srli_pi16 (reader[2], 8);
		work3 = _mm_srli_pi16 (reader[3], 8);
		reader += 4;
		writer[0] = _mm_packs_pu16( work0, work1 );
		writer[1] = _mm_packs_pu16( work2, work3 );
		writer += 2;
	}
	_mm_empty();
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

static void R_MME_CheckCvars( void ) {
	int pixelCount, blurTotal, passTotal;
	mmeBlurControl_t* blurControl = &blurData.control;
	mmeBlurControl_t* passControl = &passData.control;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;

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

	blurTotal = mme_blurFrames->integer + mme_blurOverlap->integer ;
	passTotal = mme_dofFrames->integer;

	if ( (mme_blurType->modified || passTotal != passControl->totalFrames ||  blurTotal != blurControl->totalFrames || pixelCount != mainData.pixelCount || blurControl->overlapFrames != mme_blurOverlap->integer) && !allocFailed ) {
		workUsed = 0;
		
		mainData.pixelCount = pixelCount;

		blurCreate( blurControl, mme_blurType->string, blurTotal );
		blurControl->totalFrames = blurTotal;
		blurControl->totalIndex = 0;
		blurControl->overlapFrames = mme_blurOverlap->integer; 
		blurControl->overlapIndex = 0;

		R_MME_MakeBlurBlock( &blurData.shot, pixelCount * 3, blurControl );
		R_MME_MakeBlurBlock( &blurData.stencil, pixelCount * 1, blurControl );
		R_MME_MakeBlurBlock( &blurData.depth, pixelCount * 1, blurControl );

		R_MME_JitterTable( blurData.jitter[0], blurTotal );

		//Multi pass data
		blurCreate( passControl, "median", passTotal );
		passControl->totalFrames = passTotal;
		passControl->totalIndex = 0;
		passControl->overlapFrames = 0;
		passControl->overlapIndex = 0;
		R_MME_MakeBlurBlock( &passData.dof, pixelCount * 3, passControl );
		R_MME_JitterTable( passData.jitter[0], passTotal );
	}
	mme_blurOverlap->modified = qfalse;
	mme_blurType->modified = qfalse;
	mme_blurFrames->modified = qfalse;
	mme_dofFrames->modified = qfalse;
}

qboolean R_MME_JitterOrigin( float *x, float *y ) {
	mmeBlurControl_t* passControl = &passData.control;
	*x = 0;
	*y = 0;
	if ( !shotData.take )
		return qfalse;
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		*x = mme_dofRadius->value * passData.jitter[i][0];
		*y = -mme_dofRadius->value * passData.jitter[i][1];
//		*x = 0;
//		*y = 0;
		return qtrue;
	} 
	return qfalse;
}

void R_MME_JitterView( float *pixels, float *eyes ) {
	mmeBlurControl_t* blurControl = &blurData.control;
	mmeBlurControl_t* passControl = &passData.control;
	if ( !shotData.take )
		return;
	if ( blurControl->totalFrames ) {
		int i = blurControl->totalIndex;
		pixels[0] = mme_blurJitter->value * blurData.jitter[i][0];
		pixels[1] = mme_blurJitter->value * blurData.jitter[i][1];
	}
	if ( passControl->totalFrames ) {
		int i = passControl->totalIndex;
		float scale;	//			= r_znear->value / shotData.dofFocus;
		float focus;
//		return;

		focus = shotData.dofFocus;
		if ( focus < 10 ) 
			focus = mme_depthFocus->value;
		if ( focus < 10 )
			focus = 10;
		scale = r_znear->value / focus;
		scale *= mme_dofRadius->value;
		eyes[0] = scale * passData.jitter[i][0];
		eyes[1] = scale * passData.jitter[i][1];
	}

}

static void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const __m64 *add ) {
	mmeBlurControl_t* control = block->control;
	int index = control->totalIndex;
	if ( mme_cpuSSE2->integer ) {
		if ( index == 0) {
			MME_AccumClearSSE( block->accum, add, control->SSE[ index ], block->count );
		} else {
			MME_AccumAddSSE( block->accum, add, control->SSE[ index ], block->count );
		}
	} else {
		if ( index == 0) {
			MME_AccumClearMMX( block->accum, add, control->MMX[ index ], block->count );
		} else {
			MME_AccumAddMMX( block->accum, add, control->MMX[ index ], block->count );
		}
	}
}

static void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index ) {
	mmeBlurControl_t* control = block->control;
	index = ( index + control->overlapIndex ) % control->overlapFrames;
	R_MME_BlurAccumAdd( block, block->overlap + block->count * index );
}

static ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block ) {
	mmeBlurControl_t* control = block->control;
	int index = control->overlapIndex % control->overlapFrames;
	return (byte *)( block->overlap + block->count * index );
}

static void R_MME_BlurAccumShift( mmeBlurBlock_t *block  ) {
	if ( mme_cpuSSE2->integer ) {
		MME_AccumShiftSSE( block->accum, block->accum, block->count );
	} else {
		MME_AccumShiftMMX( block->accum, block->accum, block->count );
	}
}


int R_MME_MultiPassBegin( void ) {
	return 0;
}

int R_MME_MultiPassNext( ) {
	mmeBlurControl_t* control = &passData.control;
	byte* outAlloc;
	__m64 *outAlign;
	int index;
	if ( !shotData.take )
		return 0;
	if ( !control->totalFrames )
		return 0;

	index = control->totalIndex;
	outAlloc = ri.Hunk_AllocateTempMemory( mainData.pixelCount * 3 + 16);
	outAlign = (__m64 *)((((int)(outAlloc)) + 15) & ~15);

	GLimp_EndFrame();
	R_MME_GetShot( outAlign );
	R_MME_BlurAccumAdd( &passData.dof, outAlign );

	ri.Hunk_FreeTempMemory( outAlloc );
	if ( ++(control->totalIndex) < control->totalFrames ) {
		return 1;
	}
	control->totalIndex = 0;
	R_MME_BlurAccumShift( &passData.dof );
	return 0;
}

static void R_MME_MultiShot( byte * target ) {
	if ( !passData.control.totalFrames ) {
		R_MME_GetShot( target );
	} else {
		Com_Memcpy( target, passData.dof.accum, mainData.pixelCount * 3 );
	}
}


void R_MME_TakeShot( void ) {
	int pixelCount;
	qboolean doGamma;
	qboolean doShot;
	mmeBlurControl_t* blurControl = &blurData.control;

	if ( !shotData.take || allocFailed )
		return;
	shotData.take = qfalse;

	pixelCount = glConfig.vidHeight * glConfig.vidWidth;

	doGamma = ( mme_screenShotGamma->integer || (tr.overbrightBits > 0) ) && (glConfig.deviceSupportsGamma );
	R_MME_CheckCvars();
	//Special early version using the framebuffer
	if ( mme_saveShot->integer && blurControl->totalFrames > 0 &&
		R_FrameBuffer_Blur( blurControl->Float[ blurControl->totalIndex ], blurControl->totalIndex, blurControl->totalFrames ) ) {
		byte *shotBuf;
		if ( ++(blurControl->totalIndex) < blurControl->totalFrames ) 
			return;
		blurControl->totalIndex = 0;
		shotBuf = ri.Hunk_AllocateTempMemory( pixelCount * 3 );
		R_MME_MultiShot( shotBuf );
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );

		R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, shotBuf );
		ri.Hunk_FreeTempMemory( shotBuf );
		return;
	}

	/* Test if we need to do blurred shots */
	if ( blurControl->totalFrames > 0 ) {
		mmeBlurBlock_t *blurShot = &blurData.shot;
		mmeBlurBlock_t *blurDepth = &blurData.depth;
		mmeBlurBlock_t *blurStencil = &blurData.stencil;

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
				R_MME_MultiShot( shotBuf ); 
				if ( doGamma && mme_blurGamma->integer ) {
					R_GammaCorrect( shotBuf, glConfig.vidWidth * glConfig.vidHeight * 3 );
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
				R_MME_MultiShot( (byte*)outAlign );
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

			fps = shotData.fps / ( blurControl->totalFrames );
		
			if ( mme_saveShot->integer == 1 ) {
				R_MME_BlurAccumShift( blurShot );
				if (doGamma && !mme_blurGamma->integer)
					R_GammaCorrect( (byte *)blurShot->accum, pixelCount * 3);
			}
			if ( mme_saveDepth->integer == 1 )
				R_MME_BlurAccumShift( blurDepth );
			if ( mme_saveStencil->integer == 1 )
				R_MME_BlurAccumShift( blurStencil );
		
			// Big test for an rgba shot
			if ( mme_saveShot->integer == 1 && shotData.main.type == mmeShotTypeRGBA ) 
			{
				int i;
				byte *alphaShot = ri.Hunk_AllocateTempMemory( pixelCount * 4);
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
				R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, fps, alphaShot );
				ri.Hunk_FreeTempMemory( alphaShot );
			} else {
				if ( mme_saveShot->integer == 1 )
					R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurShot->accum ));
				if ( mme_saveDepth->integer == 1 )
					R_MME_SaveShot( &shotData.depth, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurDepth->accum ));
				if ( mme_saveStencil->integer == 1 )
					R_MME_SaveShot( &shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, fps, (byte *)( blurStencil->accum) );
			}
			doShot = qtrue;
		} else {
			doShot = qfalse;
		}
	} 
	if ( mme_saveShot->integer > 1 || (!blurControl->totalFrames && mme_saveShot->integer )) {
		byte *shotBuf = ri.Hunk_AllocateTempMemory( pixelCount * 5 );
		R_MME_MultiShot( shotBuf );
		
		if ( doGamma ) 
			R_GammaCorrect( shotBuf, pixelCount * 3 );

		if ( shotData.main.type == mmeShotTypeRGBA ) {
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
		R_MME_SaveShot( &shotData.main, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, shotBuf );
		ri.Hunk_FreeTempMemory( shotBuf );
	}

	if ( shotData.main.type == mmeShotTypeRGB ) {
		if ( mme_saveStencil->integer > 1 || ( !blurControl->totalFrames && mme_saveStencil->integer) ) {
			byte *stencilShot = ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetStencil( stencilShot );
			R_MME_SaveShot( &shotData.stencil, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, stencilShot );
			ri.Hunk_FreeTempMemory( stencilShot );
		}
		if ( mme_saveDepth->integer > 1 || ( !blurControl->totalFrames && mme_saveDepth->integer) ) {
			byte *depthShot = ri.Hunk_AllocateTempMemory( pixelCount * 1);
			R_MME_GetDepth( depthShot );
			R_MME_SaveShot( &shotData.depth, glConfig.vidWidth, glConfig.vidHeight, shotData.fps, depthShot );
			ri.Hunk_FreeTempMemory( depthShot );
		}
	}
}

const void *R_MME_CaptureShotCmd( const void *data ) {
	const captureCommand_t *cmd = (const captureCommand_t *)data;

	if (!cmd->name[0])
		return (const void *)(cmd + 1);

	shotData.take = qtrue;
	shotData.fps = cmd->fps;
	shotData.dofFocus = cmd->focus;
	if (strcmp( cmd->name, shotData.main.name) || mme_screenShotFormat->modified || mme_screenShotAlpha->modified ) {
		/* Also reset the the other data */
		blurData.control.totalIndex = 0;
		if ( workAlign )
			Com_Memset( workAlign, 0, workUsed );
		Com_sprintf( shotData.main.name, sizeof( shotData.main.name ), "%s", cmd->name );
		Com_sprintf( shotData.depth.name, sizeof( shotData.depth.name ), "%s.depth", cmd->name );
		Com_sprintf( shotData.stencil.name, sizeof( shotData.stencil.name ), "%s.stencil", cmd->name );
		
		mme_screenShotFormat->modified = qfalse;
		mme_screenShotAlpha->modified = qfalse;

		if (!Q_stricmp(mme_screenShotFormat->string, "jpg")) {
			shotData.main.format = mmeShotFormatJPG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "tga")) {
			shotData.main.format = mmeShotFormatTGA;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "png")) {
			shotData.main.format = mmeShotFormatPNG;
		} else if (!Q_stricmp(mme_screenShotFormat->string, "avi")) {
			shotData.main.format = mmeShotFormatAVI;
		} else {
			shotData.main.format = mmeShotFormatTGA;
		}
		
//		if (shotData.main.format != mmeShotFormatAVI) {
			shotData.depth.format = mmeShotFormatPNG;
			shotData.stencil.format = mmeShotFormatPNG;
//		} else {
//			shotData.depth.format = mmeShotFormatAVI;
//			shotData.stencil.format = mmeShotFormatAVI;
//		}

		shotData.main.type = mmeShotTypeRGB;
		if ( mme_screenShotAlpha->integer ) {
			if ( shotData.main.format == mmeShotFormatPNG )
				shotData.main.type = mmeShotTypeRGBA;
			else if ( shotData.main.format == mmeShotFormatTGA )
				shotData.main.type = mmeShotTypeRGBA;
		}
		shotData.main.counter = -1;
		shotData.depth.type = mmeShotTypeGray;
		shotData.depth.counter = -1;
		shotData.stencil.type = mmeShotTypeGray;
		shotData.stencil.counter = -1;	
	}
	return (const void *)(cmd + 1);	
}

void R_MME_Capture( const char *shotName, float fps, float focus ) {
	captureCommand_t *cmd;
	
	if ( !tr.registered || !fps ) {
		return;
	}
	cmd = R_GetCommandBuffer( RC_CAPTURE, sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->fps = fps;
	cmd->focus = focus;
	Q_strncpyz( cmd->name, shotName, sizeof( cmd->name ));
}

void R_MME_BlurInfo( int* total, int *index ) {
	*total = mme_blurFrames->integer;
	*index = blurData.control.totalIndex;
	if (*index )
		*index -= blurData.control.overlapFrames;
}

void R_MME_Shutdown(void) {
	aviClose( &shotData.main.avi );
	aviClose( &shotData.depth.avi );
	aviClose( &shotData.stencil.avi );
}

void R_MME_Init(void) {

	// MME cvars
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
	mme_blurType = ri.Cvar_Get ( "mme_blurType", "0", CVAR_ARCHIVE );
	mme_blurGamma = ri.Cvar_Get ( "mme_blurGamma", "0", CVAR_ARCHIVE );
	mme_blurJitter = ri.Cvar_Get ( "mme_blurJitter", "1", CVAR_ARCHIVE );

	mme_dofFrames = ri.Cvar_Get ( "mme_dofFrames", "0", CVAR_ARCHIVE );
	mme_dofRadius = ri.Cvar_Get ( "mme_dofRadius", "2", CVAR_ARCHIVE );

	mme_cpuSSE2 = ri.Cvar_Get ( "mme_cpuSSE2", "0", CVAR_ARCHIVE );
	
	mme_depthRange = ri.Cvar_Get ( "mme_depthRange", "0", CVAR_ARCHIVE );
	mme_depthFocus = ri.Cvar_Get ( "mme_depthFocus", "0", CVAR_ARCHIVE );
	mme_saveOverwrite = ri.Cvar_Get ( "mme_saveOverwrite", "0", CVAR_ARCHIVE );
	mme_saveStencil = ri.Cvar_Get ( "mme_saveStencil", "0", CVAR_ARCHIVE );
	mme_saveDepth = ri.Cvar_Get ( "mme_saveDepth", "0", CVAR_ARCHIVE );
	mme_saveShot = ri.Cvar_Get ( "mme_saveShot", "1", CVAR_ARCHIVE );
	mme_workMegs = ri.Cvar_Get ( "mme_workMegs", "128", CVAR_LATCH | CVAR_ARCHIVE );

	mme_worldShader->modified = qtrue;

	Com_Memset( &shotData, 0, sizeof(shotData));
	//CANATODO, not exactly the best way to do this probably, but it works
	if (!workAlloc) {
		workSize = mme_workMegs->integer;
		if (workSize < 64)
			workSize = 64;
		if (workSize > 512)
			workSize = 512;
		workSize *= 1024 * 1024;
		workAlloc = calloc( workSize + 16, 1 );
		if (!workAlloc) {
			ri.Printf(PRINT_ALL, "Failed to allocate %d bytes for mme work buffer\n", workSize );
			return;
		}
		workAlign = (char *)(((int)workAlloc + 15) & ~15);
	}
}

