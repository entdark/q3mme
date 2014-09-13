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

/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /MissionPack/code/client/snd_dma.c $
 *
 *****************************************************************************/

#include "client.h"
#include "snd_local.h"
#include "snd_mix.h"

#define		DMA_SNDCHANNELS		128
#define		DMA_LOOPCHANNELS	128

dma_t			dma;

static mixBackground_t	dmaBackground;
static mixLoop_t		dmaLoops[DMA_LOOPCHANNELS];
static mixChannel_t		dmaChannels[DMA_SNDCHANNELS];
static mixEffect_t	dmaEffect;
static qboolean			dmaInit;
static int   			dmaWrite;

cvar_t		*s_khz;
cvar_t		*s_show;
cvar_t		*s_mixahead;
cvar_t		*s_mixPreStep;


/*
==================
S_ClearSoundBuffer

Lock the dma buffer and clear
==================
*/
void S_DMAClearBuffer( void ) {
	int		clear;
		
	if (!dmaInit)
		return;

	/* Clear the active channels and loops */
	Com_Memset( dmaLoops, 0, sizeof( dmaLoops ));
	Com_Memset( dmaChannels, 0, sizeof( dmaChannels ));
	Com_Memset( &dmaEffect, 0, sizeof( &dmaEffect ));

	s_rawend = 0;

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	/* Fill the dma buffer */
	SNDDMA_BeginPainting ();
	if (dma.buffer)
		Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit ();
}

void S_DMAInit(void) {
	dmaInit = SNDDMA_Init();
	dmaWrite = 0;

	s_khz = Cvar_Get ("s_khz", "22", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);

	s_mixPreStep = Cvar_Get ("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
}

void S_DMAShowInfo(void) {
	if (!dmaInit) {
		Com_Printf( "DMA disabled\n" );
		return;
	}
	Com_Printf("%5d stereo\n", dma.channels - 1);
	Com_Printf("%5d samples\n", dma.samples);
	Com_Printf("%5d samplebits\n", dma.samplebits);
	Com_Printf("%5d submission_chunk\n", dma.submission_chunk);
	Com_Printf("%5d speed\n", dma.speed);
	Com_Printf("0x%x dma buffer\n", dma.buffer);
}


//=============================================================================


void S_DMA_Update( float scale ) {
	int				ma, count;
	static int		lastPos;
	int				thisPos;
	int				lastWrite, lastRead;
	int				bufSize, bufDone;
	int				speed;
	int				buf[2048];

	if (!dmaInit)
		return;

	bufSize = dma.samples >> (dma.channels-1);

	// Check for possible buffer underruns

	thisPos = SNDDMA_GetDMAPos() >> (dma.channels - 1);
	lastWrite = (lastPos <= dmaWrite) ? (dmaWrite - lastPos) : (bufSize - lastPos + dmaWrite);
	lastRead = ( lastPos <= thisPos ) ? (thisPos - lastPos) : (bufSize - lastPos + thisPos);
	if (lastRead > lastWrite) {
		bufDone = 0;
		dmaWrite = thisPos;
//		Com_Printf("OMG Buffer underrun\n");
	} else {
		bufDone = lastWrite - lastRead;
	}
//	Com_Printf( "lastRead %d lastWrite %d done %d\n", lastRead, lastWrite, bufDone );
	lastPos = thisPos;

	ma = s_mixahead->value * dma.speed;
	count = lastRead;
	if (bufDone + count < ma) {
		count = ma - bufDone + 1;
	} else if (bufDone + count > bufSize) {
		count = bufSize - bufDone;
	}

	if (count > sizeof(buf) / (2 * sizeof(int))) {
		count = sizeof(buf) / (2 * sizeof(int));
	}
	// mix to an even submission block size
	count = (count + dma.submission_chunk-1) & ~(dma.submission_chunk-1);

	// never mix more than the complete buffer

	speed = (scale * (MIX_SPEED << MIX_SHIFT)) / dma.speed;

	/* Make sure that the speed will always go forward for very small scales */
	if ( speed == 0 && scale )
		speed = 1;

	/* Mix sound or fill with silence depending on speed */
	if ( speed > 0 ) {
		/* mix the background track or init the buffer with silence */
		S_MixBackground( &dmaBackground, speed, count, buf );
		S_MixChannels( dmaChannels, DMA_SNDCHANNELS, speed, count, buf );
		S_MixLoops( dmaLoops, DMA_LOOPCHANNELS, speed, count, buf );
		S_MixEffects(&dmaEffect, speed, count, buf);
	} else {
		Com_Memset( buf, 0, sizeof( buf[0] ) * count * 2);
	}
	/* Lock dma buffer and copy/clip the final data */
	SNDDMA_BeginPainting ();
	S_MixClipOutput( count, buf, (short *)dma.buffer, dmaWrite, bufSize-1 );
	SNDDMA_Submit ();
	dmaWrite += count;
	if (dmaWrite >= bufSize)
		dmaWrite -= bufSize;
}

