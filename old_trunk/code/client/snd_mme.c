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
#include "client.h"

#define SND_SAMPLERATE 22050
#define SND_CHANNELS 2

int		wavopenfailed = 0;
fileHandle_t	wavFilehandle;
unsigned long numwrittensamples;

/*
=================================================================================

WAV FILE HANDLING FUNCTIONS

=================================================================================
*/

/*
===================
S_MME_PrepareWavHeader

Fill in wav header so that we can write sound after the header
===================
*/
static void S_MME_PrepareWavHeader(void* buffer) {
	unsigned long* bufferlong = buffer;
	((unsigned long*)bufferlong)[0] = 0x46464952;	// "RIFF"
	((unsigned long*)bufferlong)[1] = LONG_MAX;		// WAVE chunk length
	((unsigned long*)bufferlong)[2] = 0x45564157;	// "WAVE"

	((unsigned long*)bufferlong)[3] = 0x20746D66;	// "fmt "
	((unsigned long*)bufferlong)[4] = 16;			// fmt chunk size
	((unsigned short*)bufferlong)[(5*2)] = 1;		// audio format. 1 - PCM uncompressed
	((unsigned short*)bufferlong)[(5*2)+1] = SND_CHANNELS;	// number of channels
	((unsigned long*)bufferlong)[6] = SND_SAMPLERATE;	// sample rate
	((unsigned long*)bufferlong)[7] = SND_SAMPLERATE * 2 * SND_CHANNELS;	// byte rate
	((unsigned short*)bufferlong)[(8*2)] = SND_CHANNELS * 2;	// block align
	((unsigned short*)bufferlong)[(8*2)+1] = 16; // sample bits

	((unsigned long*)bufferlong)[9] = 0x61746164;	// "data"
	((unsigned long*)bufferlong)[10] = LONG_MAX-36;		// data chunk length
}


/*
===================
S_MME_WavFilename

Generate a filename for wav file, used by S_MME_WavOpen()
===================
*/
void S_MME_WavFilename(long lastNumber, char* fileName, char* basename) {
	if (basename == NULL) basename = "wav";
	if (!strlen(basename)) basename = "wav";
	if ( lastNumber < 0 || lastNumber > 9999 ) lastNumber = 9999;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/%s/%s %04d.wav", basename, basename, lastNumber);
}


/*
===================
S_MME_WavOpen

Open wav file for writing, if demoname is specified, creates file with that name

Returns true if successful
===================
*/
qboolean S_MME_WavOpen(char* filename) {
	char checkname[MAX_OSPATH];
	char *basename;

	static long lastNumber = -1;

	fileHandle_t f;

	byte wavheader[44];

	if ((filename != NULL) && (strlen(filename))) {
		// explicit filename
		basename = filename;
	} else {
		basename = "wavfile";
	}
	
	// scan for free filename
	if (lastNumber == -1) lastNumber = 1;
	
	for ( ; lastNumber <= 9999; lastNumber++) {
		S_MME_WavFilename(lastNumber, checkname, basename);
		if (!FS_FileExists(checkname)) {
			// found free wavfile
			break;
		}
	}
	
	if (( lastNumber >= 9999 ) || ( lastNumber < 0 )) {
		Com_Printf ("WavOpen: Couldn't create a file\n"); 
		return 0;
	}
	
	lastNumber++;

	f = FS_FOpenFileWrite(checkname);
	if (!f) {
		Com_Printf( "Failed to open %s\n", checkname );
		return qfalse;
	}

	S_MME_PrepareWavHeader(&wavheader);
	FS_Write(&wavheader, sizeof(wavheader), f);
	
	wavFilehandle = f;
	numwrittensamples = 0;
	wavopenfailed = 0;

	return qtrue;
}


/*
===================
S_MME_WavClose

Update wav header and close the file
===================
*/
void S_MME_WavClose(void) {
	unsigned long longval;
	if (!wavFilehandle) return;

	longval = (numwrittensamples*SND_CHANNELS*2) + 36;
	FS_Seek(wavFilehandle, 4, FS_SEEK_SET);
	FS_Write(&longval, sizeof(longval), wavFilehandle);

	longval -= 36;
	FS_Seek(wavFilehandle, 40, FS_SEEK_SET);
	FS_Write(&longval, sizeof(longval), wavFilehandle);

	FS_FCloseFile(wavFilehandle);
	wavFilehandle = 0;
	wavopenfailed = 0;
}

/*
===================
S_MME_ExportBufferToFile

Write export buffer to wav file
===================
*/
void S_MME_ExportBufferToFile(portable_samplepair_t *exportbuffer, int exportbuffersize, int numsamples) {
	byte *outbuffer;
	int numbytes = numsamples*SND_CHANNELS*2;
	int i, j;
	if (!wavFilehandle) return;

	outbuffer = Hunk_AllocateTempMemory(numbytes);
	for (i = 0, j = 0; i < numsamples*4; i += 4, j++) {
		int val = exportbuffer[j].left >> 8;

		if (val > 0x7fff) {
			val = 0x7fff;
		}
		if (val < -32768) {
			val = -32768;
		}

		outbuffer[i] = val & 255;
		outbuffer[i+1] = val >> 8;

		val = exportbuffer[j].right >> 8;
		if (val > 0x7fff) {
			val = 0x7fff;
		}
		if (val < -32768) {
			val = -32768;
		}

		outbuffer[i+2] = val & 255;
		outbuffer[i+3] = val >> 8;
	}
	FS_Write(outbuffer, numbytes, wavFilehandle);
	Hunk_FreeTempMemory(outbuffer);

	numwrittensamples += numsamples;
}

/*
=================================================================================

SOUND BUFFER RENDERING FUNCTIONS

=================================================================================
*/

static void S_MME_PaintChannelFrom16ForExport( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset, int snd_vol, portable_samplepair_t *exportbuffer ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata, fdiv, fleftvol, frightvol;

	samp = &exportbuffer[ bufferOffset ];

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	chunk = sc->soundData;
	while (sampleOffset>=SND_CHUNK_SIZE) {
		chunk = chunk->next;
		sampleOffset -= SND_CHUNK_SIZE;
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler || ch->dopplerScale==1.0f) {
		leftvol = ch->leftvol*snd_vol;
		rightvol = ch->rightvol*snd_vol;
		samples = chunk->sndChunk;
		for ( i=0 ; i<count ; i++ ) {
			data  = samples[sampleOffset++];
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;

			if (sampleOffset == SND_CHUNK_SIZE) {
				chunk = chunk->next;
				samples = chunk->sndChunk;
				sampleOffset = 0;
			}
		}
	} else {
		fleftvol = ch->leftvol*snd_vol;
		frightvol = ch->rightvol*snd_vol;

		ooff = sampleOffset;
		samples = chunk->sndChunk;

		for ( i=0 ; i<count ; i++ ) {
			aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			boff = ooff;
			fdata = 0;
			for (j=aoff; j<boff; j++) {
				if (j == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					if (!chunk) {
						chunk = sc->soundData;
					}
					samples = chunk->sndChunk;
					ooff -= SND_CHUNK_SIZE;
				}
				fdata  += samples[j&(SND_CHUNK_SIZE-1)];
			}
			fdiv = 256 * (boff-aoff);
			samp[i].left += (fdata * fleftvol)/fdiv;
			samp[i].right += (fdata * frightvol)/fdiv;
		}
	}
}

void S_MME_PaintChannelFromWaveletForExport( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset, int snd_vol, portable_samplepair_t *exportbuffer ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &exportbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE_FLOAT*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE_FLOAT*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		S_AdpcmGetSamples( chunk, sfxScratchBuffer );
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*2) {
			chunk = chunk->next;
			decodeWavelet(chunk, sfxScratchBuffer);
			sfxScratchIndex++;
			sampleOffset = 0;
		}
	}
}

void S_MME_PaintChannelFromADPCMForExport( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset, int snd_vol, portable_samplepair_t *exportbuffer ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &exportbuffer[ bufferOffset ];
	chunk = sc->soundData;

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	while (sampleOffset>=(SND_CHUNK_SIZE*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		S_AdpcmGetSamples( chunk, sfxScratchBuffer );
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*4) {
			chunk = chunk->next;
			S_AdpcmGetSamples( chunk, sfxScratchBuffer);
			sampleOffset = 0;
			sfxScratchIndex++;
		}
	}
}

void S_MME_PaintChannelFromMuLawForExport( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset, int snd_vol, portable_samplepair_t *exportbuffer ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	byte					*samples;
	float					ooff;

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	samp = &exportbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE*2)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*2);
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler) {
		samples = (byte *)chunk->sndChunk + sampleOffset;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[*samples];
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			samples++;
			if (samples == (byte *)chunk->sndChunk+(SND_CHUNK_SIZE*2)) {
				chunk = chunk->next;
				samples = (byte *)chunk->sndChunk;
			}
		}
	} else {
		ooff = sampleOffset;
		samples = (byte *)chunk->sndChunk;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[samples[(int)(ooff)]];
			ooff = ooff + ch->dopplerScale;
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			if (ooff >= SND_CHUNK_SIZE*2) {
				chunk = chunk->next;
				if (!chunk) {
					chunk = sc->soundData;
				}
				samples = (byte *)chunk->sndChunk;
				ooff = 0.0;
			}
		}
	}
}


/*
===================
S_MME_PaintChannelsForExport

Paint channels into exportbuffer
Doesn't mix music
===================
*/
void S_MME_PaintChannelsForExport (unsigned long numsamples, unsigned long endsample, portable_samplepair_t* exportbuffer) {
	int exportbuffersize = sizeof(portable_samplepair_t) * numsamples;

	channel_t *ch;
	sfx_t	*sc;

	unsigned long currentsample = endsample - numsamples;
	unsigned long sampleOffset;

	int		i;
	int		snd_vol	= 204;
	int		count;

	// fill the buffer with zeros
	Com_Memset(exportbuffer, 0, exportbuffersize);

	// paint normal channels
	ch = s_channels;
	for (i = 0; i < MAX_CHANNELS; i++, ch++) {
		if (!ch->thesfx || (ch->leftvol < 0.25 && ch->rightvol < 0.25)) {
			continue;
		}

		sc = ch->thesfx;

		if (!(sc->soundData) || !(sc->soundLength)) {
			continue;
		}

		sampleOffset = currentsample - ch->startSample;
		count = endsample - currentsample;
		if (sampleOffset + count > sc->soundLength) {
			count = sc->soundLength - sampleOffset;
		}

		if (count > 0) {
			if( sc->soundCompressionMethod == 1) {
				S_MME_PaintChannelFromADPCMForExport	(ch, sc, count, sampleOffset, 0, snd_vol, exportbuffer);
			} else if( sc->soundCompressionMethod == 2) {
				S_MME_PaintChannelFromWaveletForExport	(ch, sc, count, sampleOffset, 0, snd_vol, exportbuffer);
			} else if( sc->soundCompressionMethod == 3) {
				S_MME_PaintChannelFromMuLawForExport	(ch, sc, count, sampleOffset, 0, snd_vol, exportbuffer);
			} else {
				S_MME_PaintChannelFrom16ForExport		(ch, sc, count, sampleOffset, 0, snd_vol, exportbuffer);
			}
		}

	}

	// paint looped channels
	ch = loop_channels;
	for (i = 0; i < numLoopChannels; i++, ch++) {
		int loopsample;
		if (!ch->thesfx || (!(ch->leftvol) && !(ch->rightvol))) {
			continue;
		}

		sc = ch->thesfx;

		if (!(sc->soundData) || !(sc->soundLength)) {
			continue;
		}

		loopsample = currentsample;
		while (loopsample < endsample) {
			sampleOffset = (loopsample % sc->soundLength);

			count = endsample - loopsample;
			if (sampleOffset + count > sc->soundLength) {
				count = sc->soundLength - sampleOffset;
			}

			if (count > 0) {
				if( sc->soundCompressionMethod == 1) {
					S_MME_PaintChannelFromADPCMForExport	(ch, sc, count, sampleOffset, loopsample-currentsample, snd_vol, exportbuffer);
				} else if( sc->soundCompressionMethod == 2) {
					S_MME_PaintChannelFromWaveletForExport	(ch, sc, count, sampleOffset, loopsample-currentsample, snd_vol, exportbuffer);
				} else if( sc->soundCompressionMethod == 3) {
					S_MME_PaintChannelFromMuLawForExport	(ch, sc, count, sampleOffset, loopsample-currentsample, snd_vol, exportbuffer);
				} else {
					S_MME_PaintChannelFrom16ForExport		(ch, sc, count, sampleOffset, loopsample-currentsample, snd_vol, exportbuffer);
				}
				loopsample += count;
			}
		}

	}
}


/*
========================
S_MME_ScanChannelStarts

Scans channels for started sounds

Returns qtrue if any new sounds were started since the last mix
========================
*/
qboolean S_MME_ScanChannelStarts( int currentsample ) {
	channel_t		*ch;
	int				i;
	qboolean		newSamples;

	newSamples = qfalse;
	ch = s_channels;

	for (i=0; i<MAX_CHANNELS ; i++, ch++) {
		if ( !ch->thesfx ) {
			continue;
		}
		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if ( ch->startSample == START_SAMPLE_IMMEDIATE ) {
			ch->startSample = currentsample;
			newSamples = qtrue;
			continue;
		}

		// if it is completely finished by now, clear it
		if ( ch->startSample + (ch->thesfx->soundLength) <= currentsample ) {
			S_ChannelFree(ch);
		}
	}

	return newSamples;
}


/*
===================
S_MME_Update

Called from CL_Frame() in cl_main.c when shooting avidemo
===================
*/
void S_MME_Update(double currenttime, double frametime, char* demoname) {
	unsigned long currentsample = ((currenttime*0.001)*SND_SAMPLERATE);
	unsigned long numsamples = ((frametime*0.001)*SND_SAMPLERATE);
	unsigned long endsample = currentsample+numsamples;

	static int oldcurrenttime = -1;

	int exportbuffersize = sizeof(portable_samplepair_t) * numsamples;
	portable_samplepair_t* exportbuffer;

	Com_DPrintf("S_MME_Update(%f, %f, %s)\n", currenttime, frametime, demoname);

	if (oldcurrenttime == currenttime) return;
	oldcurrenttime = currenttime;

	if (!wavFilehandle && !wavopenfailed) {
		wavopenfailed = !S_MME_WavOpen(demoname);
	}

	// allocate export buffer
	exportbuffer = Hunk_AllocateTempMemory(exportbuffersize);

	// clear any sound effects that end before the current time,
	// and start any new sounds
	S_MME_ScanChannelStarts(currentsample);

	// paint channels to export buffer
	S_MME_PaintChannelsForExport (numsamples, endsample, exportbuffer);
	S_MME_ExportBufferToFile(exportbuffer, exportbuffersize, numsamples);

	Hunk_FreeTempMemory(exportbuffer);
}
