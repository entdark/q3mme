/*
===========================================================================
Copyright (C) 2005 Eugene Bujak.

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
// snd_mme.c -- Movie Maker's Edition sound routines


#include "snd_local.h"
#include "snd_mix.h"

#define MME_SNDCHANNELS 128
#define MME_LOOPCHANNELS 128

extern	cvar_t	*mme_saveWav;

typedef struct {
	char			baseName[MAX_OSPATH];
	fileHandle_t	fileHandle;
	long			fileSize;
	float			deltaSamples, sampleRate;
	mixLoop_t		loops[MME_LOOPCHANNELS];
	mixChannel_t	channels[MME_SNDCHANNELS];
	mixEffect_t		effect;
	qboolean		gotFrame;
} mmeWav_t;

static mmeWav_t mmeSound;

/*
=================================================================================

WAV FILE HANDLING FUNCTIONS

=================================================================================
*/

#define WAV_HEADERSIZE 44
/*
===================
S_MME_PrepareWavHeader

Fill in wav header so that we can write sound after the header
===================
*/
static void S_MMEFillWavHeader(void* buffer, long fileSize, int sampleRate) {
	((unsigned long*)buffer)[0] = 0x46464952;	// "RIFF"
	((unsigned long*)buffer)[1] = fileSize-8;		// WAVE chunk length
	((unsigned long*)buffer)[2] = 0x45564157;	// "WAVE"

	((unsigned long*)buffer)[3] = 0x20746D66;	// "fmt "
	((unsigned long*)buffer)[4] = 16;			// fmt chunk size
	((unsigned short*)buffer)[(5*2)] = 1;		// audio format. 1 - PCM uncompressed
	((unsigned short*)buffer)[(5*2)+1] = 2;		// number of channels
	((unsigned long*)buffer)[6] = sampleRate;	// sample rate
	((unsigned long*)buffer)[7] = sampleRate * 2 * 2;	// byte rate
	((unsigned short*)buffer)[(8*2)] = 2 * 2;	// block align
	((unsigned short*)buffer)[(8*2)+1] = 16;	// sample bits

	((unsigned long*)buffer)[9] = 0x61746164;	// "data"
	((unsigned long*)buffer)[10] = fileSize-44;	// data chunk length
}

/*
===================
S_MME_Update

Called from CL_Frame() in cl_main.c when shooting avidemo
===================
*/

void S_MMEClearBuffer( void ) {

}

void S_MMEWavClose(void) {
	byte header[WAV_HEADERSIZE];

	if (!mmeSound.fileHandle)
		return;

	S_MMEFillWavHeader( header, mmeSound.fileSize, MME_SAMPLERATE );
	FS_Seek( mmeSound.fileHandle, 0, FS_SEEK_SET );
	FS_Write( header, sizeof(header), mmeSound.fileHandle );
	FS_FCloseFile( mmeSound.fileHandle );
	Com_Memset( &mmeSound, 0, sizeof(mmeSound) );
}

static byte wavExportBuf[MME_SAMPLERATE] = {0};
static int bytesInBuf = 0;
qboolean S_MMEAviImport(byte *out, int *size) {
	const char *format = Cvar_VariableString("mme_screenShotFormat");
	const int shot = Cvar_VariableIntegerValue("mme_saveShot");
	const int depth = Cvar_VariableIntegerValue("mme_saveDepth");
	const int stencil = Cvar_VariableIntegerValue("mme_saveStencil");
	if (mme_saveWav->integer != 2 || Q_stricmp(format, "avi") || (!shot && !depth && !stencil))
		return qfalse;
	*size = 0;
	if (bytesInBuf >= MME_SAMPLERATE)
		bytesInBuf -= MME_SAMPLERATE;
	if (bytesInBuf <= 0)
		return qtrue;
	Com_Memcpy( out, wavExportBuf, bytesInBuf );
	*size = bytesInBuf;
	bytesInBuf = 0;
	return qtrue;
}

#define MAXUPDATE 4096
void S_MMEUpdate( float scale ) {
	int count, speed;
	int mixTemp[MAXUPDATE*2];
	short mixClip[MAXUPDATE*2];

	if (!mmeSound.fileHandle && mme_saveWav->integer != 2)
		return;
	if (!mmeSound.gotFrame) {
		if (mme_saveWav->integer != 2)
			S_MMEWavClose();
		return;
	}
	count = (int)mmeSound.deltaSamples;
	if (!count)
		return;
	mmeSound.deltaSamples -= count;
	if (count > MAXUPDATE)
		count = MAXUPDATE;

	speed = (scale * (MIX_SPEED << MIX_SHIFT)) / MME_SAMPLERATE;
	if (speed < 0 || (speed == 0 && scale) )
		speed = 1;

	Com_Memset( mixTemp, 0, sizeof(int) * count * 2);
	if ( speed > 0 ) {
		S_MixChannels( mmeSound.channels, MME_SNDCHANNELS, speed, count, mixTemp );
		S_MixLoops( mmeSound.loops, MME_LOOPCHANNELS, speed, count, mixTemp );
		S_MixEffects( &mmeSound.effect, speed, count, mixTemp );
	}	
	S_MixClipOutput( count, mixTemp, mixClip, 0, MAXUPDATE - 1 );
	if (mme_saveWav->integer != 2) {
		FS_Write( mixClip, count*4, mmeSound.fileHandle );
	} else if (mme_saveWav->integer == 2) {
		Com_Memcpy(&wavExportBuf[bytesInBuf], mixClip, count*4 );
		bytesInBuf += count*4;
	}
	mmeSound.fileSize += count * 4;
	mmeSound.gotFrame = qfalse;
}

void S_MMERecord( const char *baseName, float deltaTime ) {
	const char *format = Cvar_VariableString("mme_screenShotFormat");
	const int shot = Cvar_VariableIntegerValue("mme_saveShot");
	const int depth = Cvar_VariableIntegerValue("mme_saveDepth");
	const int stencil = Cvar_VariableIntegerValue("mme_saveStencil");
	if (!mme_saveWav->integer || (mme_saveWav->integer == 2 && (Q_stricmp(format, "avi") || (!shot && !depth && !stencil))))
		return;
	if (Q_stricmp(baseName, mmeSound.baseName) && mme_saveWav->integer != 2) {
		char fileName[MAX_OSPATH];
		if (mmeSound.fileHandle)
			S_MMEWavClose();
		Com_sprintf( fileName, sizeof(fileName), "%s.wav", baseName );
		mmeSound.fileHandle = FS_FOpenFileReadWrite( fileName );
		if (!mmeSound.fileHandle) {
			mmeSound.fileHandle = FS_FOpenFileWrite( fileName );
			if (!mmeSound.fileHandle) 
				return;
		}
		Q_strncpyz( mmeSound.baseName, baseName, sizeof( mmeSound.baseName ));
		mmeSound.deltaSamples = 0;
		mmeSound.sampleRate = MME_SAMPLERATE;
		FS_Seek( mmeSound.fileHandle, 0, FS_SEEK_END );
		mmeSound.fileSize = FS_filelength( mmeSound.fileHandle );
		if ( mmeSound.fileSize < WAV_HEADERSIZE) {
			int left = WAV_HEADERSIZE - mmeSound.fileSize;
			while (left) {
				FS_Write( &left, 1, mmeSound.fileHandle );
				left--;
       		}
			mmeSound.fileSize = WAV_HEADERSIZE;
		}
	} else if (Q_stricmp(baseName, mmeSound.baseName) && mme_saveWav->integer == 2) {
		Q_strncpyz( mmeSound.baseName, baseName, sizeof( mmeSound.baseName ));
		mmeSound.deltaSamples = 0;
		mmeSound.sampleRate = MME_SAMPLERATE;
		mmeSound.fileSize = 0;
		bytesInBuf = 0;
	}
	mmeSound.deltaSamples += deltaTime * mmeSound.sampleRate;
	mmeSound.gotFrame = qtrue;
}

/* This is a seriously crappy hack, but it'll work for now */
void S_MMEMusic( const char *musicName, float time, float length ) {
	if ( !musicName || !musicName[0] ) {
		s_background.playing = qfalse;
		s_background.override = qfalse;
		return;
	}
	Q_strncpyz( s_background.startName, musicName, sizeof( s_background.startName ));
	/* S_FileExists sets correct extension */
	if (!S_FileExists(s_background.startName)) {
		s_background.playing = qfalse;
		s_background.override = qfalse;
	} else {
		s_background.override = qtrue;
		s_background.seekTime = time;
		s_background.length = length;
		s_background.playing = qtrue;
		s_background.reload = qtrue;
	}
}