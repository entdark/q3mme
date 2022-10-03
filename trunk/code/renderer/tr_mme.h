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
#include <mmintrin.h>

#define MME_MAX_WORKSIZE 2048

#define AVI_MAX_FRAMES	20000
#define AVI_MAX_SIZE	((2*1024-10)*1024*1024)
#define AVI_HEADER_SIZE	2048
#define AVI_MAX_FILES	1000

#define BLURMAX 256
#define PASSMAX 256

#define PIPE_COMMAND_Q "\""
#ifndef MACOS_X
#define PIPE_COMMAND_ABSOLUTE_PATH ""
#define PIPE_COMMAND_ABSOLUTE_PATH_Q ""
#define PIPE_COMMAND_ABSOLUTE_PATH_E ""
#else
#define PIPE_COMMAND_ABSOLUTE_PATH "%a"
#define PIPE_COMMAND_ABSOLUTE_PATH_Q "\"%a"
#define PIPE_COMMAND_ABSOLUTE_PATH_E PIPE_COMMAND_Q
#endif
#define PIPE_COMMAND_BASE(s, e)	PIPE_COMMAND_ABSOLUTE_PATH_Q "ffmpeg"	\
								PIPE_COMMAND_ABSOLUTE_PATH_E			\
								" -f avi -i - -threads 0 "				\
								s										\
								" -c:a aac -c:v libx264 -preset ultrafast -y -pix_fmt yuv420p -crf 19 "	\
								PIPE_COMMAND_Q \
								PIPE_COMMAND_ABSOLUTE_PATH "%o."		\
								e										\
								PIPE_COMMAND_Q							\
								" 2> "									\
								PIPE_COMMAND_ABSOLUTE_PATH_Q			\
								"ffmpeglog.txt"							\
								PIPE_COMMAND_ABSOLUTE_PATH_E

#define PIPE_COMMAND_DEFAULT	PIPE_COMMAND_BASE("", "mkv")
#define PIPE_COMMAND_STEREO		PIPE_COMMAND_BASE("-metadata:s:v stereo_mode=top_bottom", "mkv")
#define PIPE_COMMAND_VR180		PIPE_COMMAND_BASE("-vf v360=c1x6:he:in_forder=frblud:in_stereo=tb:out_stereo=sbs -metadata:s:v stereo_mode=left_right", "mkv")
#define PIPE_COMMAND_VR360		PIPE_COMMAND_BASE("-vf v360=c1x6:e:in_forder=frblud", "mp4")

typedef struct mmeAviFile_s {
	char name[MAX_OSPATH];
	fileHandle_t f;
	float fps;
	int	width, height;
	unsigned int frames, aframes, iframes;
	int index[2*AVI_MAX_FRAMES];
	int aindex[2*AVI_MAX_FRAMES];
	int	written, awritten, maxSize;
	int header;
	int format;
	qboolean audio;
    qboolean pipe;
	mmeShotType_t type;
} mmeAviFile_t;

typedef struct {
	char name[MAX_OSPATH];
	char nameOld[MAX_OSPATH];
	int	 counter;
	mmeShotFormat_t format;
	mmeShotType_t type;
	mmeAviFile_t avi;
	byte *stereoTemp;
} mmeShot_t;

typedef struct {
//	int		pixelCount;
	int		totalFrames;
	int		totalIndex;
	int		overlapFrames;
	int		overlapIndex;

	short	MMX[BLURMAX];
	short	SSE[BLURMAX];
	float	Float[BLURMAX];
} mmeBlurControl_t;

typedef struct {
	__m64	*accum;
	__m64	*overlap;
	int		count;
	mmeBlurControl_t *control;
} mmeBlurBlock_t;

void R_MME_GetShot( void* output, mmeShotType_t type, qboolean square );
void R_MME_GetStencil( void *output );
void R_MME_GetDepth( byte *output );
void R_MME_SaveShot( mmeShot_t *shot, int width, int height, float fps, byte *inBuf, qboolean audio, int aSize, byte *aBuf, qboolean stereo );

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf, qboolean audio );
void mmeAviSound( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, const byte *soundBuf, int size );
void aviClose( mmeAviFile_t *aviFile );

void R_MME_BlurAccumAdd( mmeBlurBlock_t *block, const __m64 *add );
void R_MME_BlurOverlapAdd( mmeBlurBlock_t *block, int index );
void R_MME_BlurAccumShift( mmeBlurBlock_t *block  );
void blurCreate( mmeBlurControl_t* control, const char* type, int frames );
void R_MME_JitterTable(float *jitarr, int num);

void MME_AccumClearSSE( void* Q_RESTRICT w, const void* Q_RESTRICT r, short int mul, int count );
void MME_AccumAddSSE( void* Q_RESTRICT w, const void* Q_RESTRICT r, short int mul, int count );
void MME_AccumShiftSSE( const void *r, void *w, int count );

float R_MME_FocusScale(float focus);
void R_MME_ClampDof(float *focus, float *radius);

extern cvar_t	*mme_combineStereoShots;
extern cvar_t	*mme_pipeCommand;

extern cvar_t	*mme_aviFormat;

extern cvar_t	*mme_blurJitter;
extern cvar_t	*mme_blurStrength;
extern cvar_t	*mme_dofFrames;
extern cvar_t	*mme_dofRadius;

ID_INLINE byte * R_MME_BlurOverlapBuf( mmeBlurBlock_t *block ) {
	mmeBlurControl_t* control = block->control;
	int index = control->overlapIndex % control->overlapFrames;
	return (byte *)( block->overlap + block->count * index );
}
