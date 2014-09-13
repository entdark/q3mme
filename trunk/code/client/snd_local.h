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
// snd_local.h -- private sound definitions


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define		SFX_SOUNDS		2048

typedef struct sfxEntry_s {
	struct		sfxEntry_s *next;
	char		name[MAX_QPATH];
} sfxEntry_t;

typedef struct {
	float		seekTime;
	float		length;
	char		startName[MAX_QPATH];
	char		loopName[MAX_QPATH];
	qboolean	playing;
	qboolean	reload;
	qboolean	override;
} backgroundSound_t;

typedef struct {
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
} portable_samplepair_t;

typedef struct {
	short		*samples;
	int			length;
} sndMixData_t;

typedef struct {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;

typedef struct {
	vec3_t		origin;
	vec3_t		velocity;
	sfxHandle_t	handle;
	int			lastFrame;
	int			volume;
} entitySound_t;

typedef struct {
	const void	*parent;
	vec3_t		origin;
	vec3_t		velocity;
	sfxHandle_t	handle;
	int			volume;
} loopQueue_t;

typedef struct {
	sfxHandle_t	handle;
	vec3_t		origin;
	short		entNum;
	char		entChan;
	char		hasOrigin;
} channelQueue_t;

struct openSound_s;
typedef int (*openSoundRead_t)( struct openSound_s *open, qboolean stereo, int size, short *data );
/* Maybe some kind of error return sometime? */
typedef int (*openSoundSeek_t)( struct openSound_s *open, int samples );
typedef void (*openSoundClose_t)( struct openSound_s *open );

typedef struct openSound_s {
	int					rate;
	int					totalSamples, doneSamples;
	char				buf[16*1024];
	int					bufUsed, bufPos;
	fileHandle_t		fileHandle;
	int					fileSize, filePos;

	openSoundRead_t		read;
	openSoundSeek_t		seek;
	openSoundClose_t	close;

	char				data[0];
} openSound_t;

/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

extern dma_t dma;
// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init(void);

// gets the current DMA position
int		SNDDMA_GetDMAPos(void);

// shutdown the DMA xfer.
void	SNDDMA_Shutdown(void);

void	SNDDMA_BeginPainting (void);

void	SNDDMA_Submit(void);

//====================================================================

#define	MAX_CHANNELS			96
#define	MAX_SNDQUEUE			512
#define	MAX_LOOPQUEUE			512

extern	sfxEntry_t		sfxEntries[SFX_SOUNDS];
extern	backgroundSound_t s_background;
extern	channelQueue_t	s_channelQueue[MAX_SNDQUEUE];
extern	int				s_channelQueueCount;
extern	entitySound_t	s_entitySounds[MAX_GENTITIES];
extern	loopQueue_t		s_loopQueue[MAX_LOOPQUEUE];
extern	int				s_loopQueueCount;
extern	int				s_rawend;


#define	MAX_RAW_SAMPLES	16384
extern	portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];


extern cvar_t	*s_volume;
extern cvar_t	*s_nosound;
extern cvar_t	*s_khz;
extern cvar_t	*s_show;
extern cvar_t	*s_doppler;
extern cvar_t	*s_dopplerSpeed;
extern cvar_t	*s_dopplerFactor;
extern cvar_t	*s_mixahead;
extern cvar_t	*s_mixPreStep;

extern cvar_t	*s_attenuate;

extern cvar_t	*s_testsound;
extern cvar_t	*s_separation;
extern cvar_t	*s_musicVolume;

extern int		s_listenNumber;
extern vec3_t	s_listenOrigin;
extern vec3_t	s_listenVelocity;
extern vec3_t	s_listenAxis[3];

extern qboolean	s_underWater;

qboolean S_FileExists(char *fileName);

void	*S_Malloc( int size );
openSound_t *S_SoundOpen( const char *fileName );
int S_SoundRead( openSound_t *open, qboolean stereo, int size, short *data );
int S_SoundSeek( openSound_t *open, int samples );
void S_SoundClose( openSound_t *open );

