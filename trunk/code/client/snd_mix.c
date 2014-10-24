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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "snd_local.h"
#include "snd_mix.h"

static	mixSound_t		*mixSounds[SFX_SOUNDS];

#define DEF_COMSOUNDMEGS "12"
static  mixSound_t		*mixAllocSounds = 0;
static  mixSound_t		mixEmptySound;
static  int				zeroFill;		//So emptySound.data has some data

static cvar_t	*s_mixSame;
static cvar_t	*s_mixSameTime;
cvar_t			*s_effects;

static mixSound_t *S_MixAllocSound( int samples ) {
	int allocSize, lastUsed = (1 << 30);
	mixSound_t *sound, *allocSound;
	int countUsed;
		
	/* Make sure the size contains the header and is aligned on a pointer size boundary */
	allocSize = sizeof( mixSound_t ) + samples * 2;
	allocSize = (allocSize + (sizeof(void *) -1)) & ~(sizeof(void *)-1);
	/* First pass we just scan for a free block and find the oldest sound */
	allocSound = 0;
	countUsed = 0;
	for (sound = mixAllocSounds; sound; sound = sound->next) {
		countUsed += sound->size;
		if (sound->handle) {
			if (sound->lastUsed < lastUsed) {
				lastUsed = sound->lastUsed;
				allocSound = sound;
			}
			continue;
		} else {
			if ( allocSize <= sound->size ) {
				allocSound = sound;
				break;
			}
		}
	}
	/* Still haven't found a block, this is bad... */
	if (!allocSound)
		return 0;
	mixSounds[allocSound->handle] = 0;
	allocSound->handle = 0;
	/* Do we need to allocate any extra blocks to make the right size */
	while (allocSound->size < allocSize) {
		/* First always check if we're surrounded by an empty block and take that first */
		if (allocSound->prev && !allocSound->prev->handle) {
takePrev:
			mixSounds[allocSound->prev->handle] = 0;
			/* Free block before this, take it! */
			allocSound->prev->size += allocSound->size;
			if (allocSound->next)
				allocSound->next->prev = allocSound->prev;
			allocSound->prev->next = allocSound->next;
			allocSound = allocSound->prev;
		} else if (allocSound->next && !allocSound->next->handle) {
takeNext:
			/* Free block after this, take it! */
			mixSounds[allocSound->next->handle] = 0;
			allocSound->size += allocSound->next->size;
			if (allocSound->next->next)
				allocSound->next->next->prev = allocSound;
			allocSound->next = allocSound->next->next;
		} else if (allocSound->prev && allocSound->next) {
			/* See if the one before or after this is older and take that one */
			if (allocSound->prev->lastUsed < allocSound->next->lastUsed)
				goto takePrev;
			else
				goto takeNext;
		} else if (allocSound->prev) {
			goto takePrev;
		} else if (allocSound->next) {
			goto takeNext;
		} else {
			Com_Error( ERR_FATAL, "Mix allocation block is corrupt\n");
		}
	}
	/* Is our new block bigger than we need, put it at the end */
	if (allocSound->size > ( 256 + allocSize + sizeof( mixSound_t ))) {
		int remainSize = allocSound->size - allocSize;
		sound = allocSound;
		
		allocSound = (mixSound_t*)(((byte*)sound) + remainSize);
		if (sound->next)
			sound->next->prev = allocSound;
		allocSound->next = sound->next;
		allocSound->prev = sound;
		sound->next = allocSound;
		sound->handle = 0;
		sound->size = remainSize;
		allocSound->size = allocSize;
	}
	return allocSound;
}

const mixSound_t *S_MixGetSound( sfxHandle_t sfxHandle ) {
	sfxEntry_t *entry;
	mixSound_t *sound;
	openSound_t *openSound;

	sound = mixSounds[sfxHandle];
	if ( sound ) {
		sound->lastUsed = com_frameTime;	
		return sound;
	}

	entry = sfxEntries + sfxHandle;
	openSound = S_SoundOpen( entry->name );
	if (!openSound) {
		return mixSounds[sfxHandle] = &mixEmptySound;
	}
	if (openSound->totalSamples > (1 << (30 - MIX_SHIFT))) {
		S_SoundClose( openSound );
		Com_Printf( "Mixer:Sound file too large\n" );
		return mixSounds[sfxHandle] = &mixEmptySound;
	}
	sound = S_MixAllocSound( openSound->totalSamples );
	if (!sound) {
		S_SoundClose( openSound );
		Com_Printf( "Mixer:Failed to alloc memory for sound\n" );
		return mixSounds[sfxHandle] = &mixEmptySound;
	}
	mixSounds[ sfxHandle ] = sound;
	sound->speed = (openSound->rate << MIX_SHIFT) / MIX_SPEED;
	sound->handle = sfxHandle;
	sound->lastUsed = com_frameTime;
	sound->samples = S_SoundRead( openSound, qfalse, openSound->totalSamples, sound->data );
	if (sound->samples != openSound->totalSamples) {
		Com_Printf( "Mixer:Failed to load %s fully\n", entry->name );
	}
	sound->samples <<= MIX_SHIFT;
	S_SoundClose( openSound );
	return sound;
}



void S_MixClipOutput (int count, const int *input, short *output, int outStart, int outMask) {
	int		i;
	int		val;

	for (i=0 ; i<count ; i++)
	{
		val = input[i*2+0] >> MIX_SHIFT;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < -32768)
			val = -32768;
		output[outStart*2 + 0] = val;

		val = input[i*2+1] >> MIX_SHIFT;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < -32768)
			val = -32768;
		output[outStart*2 + 1] = val;
		outStart = (outStart+1) & outMask;
	}
}

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

#define		SOUND_FULLVOLUME	80
#define		SOUND_ATTENUATE		0.0008f

static void S_MixSpatialize(const vec3_t origin, float volume, int *left_vol, int *right_vol)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;
    vec3_t		vec;

	const float dist_mult = SOUND_ATTENUATE * s_attenuate->value;
	
	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, s_listenOrigin, source_vec);

	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels
	
	VectorRotate( source_vec, s_listenAxis, vec );

	dot = -vec[1];
	rscale = 0.5 * (1.0 + dot);
	lscale = 0.5 * (1.0 - dot);
	//rscale = s_separation->value + ( 1.0 - s_separation->value ) * dot;
	//lscale = s_separation->value - ( 1.0 - s_separation->value ) * dot;
//	if ( rscale < 0 )
//		rscale = 0;
//	if ( lscale < 0 ) 
//		lscale = 0;
	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (volume * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (volume * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}

#define MAX_DOPPLER_SCALE 50			//arbitrary
static int S_MixDopplerOriginal( int speed, const vec3_t origin, const vec3_t velocity ) {
	vec3_t	out;
	float	lena, lenb, scale;

	lena = DistanceSquared( s_listenOrigin, origin);
	VectorAdd( origin, velocity, out);
	lenb = DistanceSquared( s_listenOrigin, out);
	scale = lenb/(lena*100);
	if (scale > MAX_DOPPLER_SCALE) {
		speed *= MAX_DOPPLER_SCALE;
	} else if (scale > 1) {
		speed *= scale;
	}
	return speed;
}

static int S_MixDopplerFull( int speed, const vec3_t origin, const vec3_t velocity ) {
	vec3_t	delta;
	float	vL, vO, ratio;

	VectorSubtract( origin, s_listenOrigin, delta );
	VectorNormalize( delta );

	vL = DotProduct( delta,  s_listenVelocity );
	vO = DotProduct( delta,  velocity );
	
	vL *= s_dopplerFactor->value;
	vO *= s_dopplerFactor->value;

	vL = s_dopplerSpeed->value - vL;
	vO = s_dopplerSpeed->value + vO;
    
	ratio = vL / vO;
	return speed * ratio;
}

static void S_MixChannel( mixChannel_t *ch, int speed, int count, int *output ) {
	const mixSound_t *sound;
	int i, leftVol, rightVol;
	int index, indexAdd, indexLeft;
	float *origin;
	const short *data;
	float volume;

	volume = s_volume->value * (1 << MIX_SHIFT) * 0.5;
	origin = (!ch->hasOrigin) ? s_entitySounds[ch->entNum].origin : ch->origin;
	S_MixSpatialize( origin, volume , &leftVol, &rightVol );
	sound = S_MixGetSound( ch->handle );

	index = ch->index;
	indexAdd = (sound->speed * speed) >> MIX_SHIFT;
	indexLeft = sound->samples - index;
	ch->wasMixed = (leftVol | rightVol) > 0;
	if (!ch->wasMixed) {
		indexAdd *= count;
		if ( indexAdd >= indexLeft ) {
			ch->handle = 0;
		} else {
			ch->index += indexAdd;
		}
		return;
	}
	data = sound->data;
	if (!indexAdd)
		return;
	indexLeft /= indexAdd;
	if ( indexLeft <= count) {
		count = indexLeft;
		ch->handle = 0;
	}

	for (i = 0; i < count;i++) {
		int sample;
		sample = data[index >> MIX_SHIFT];
		output[i*2+0] += sample * leftVol;
		output[i*2+1] += sample * rightVol;
		index += indexAdd;
	}
	ch->index = index;
}


static void S_MixLoop( mixLoop_t *loop, const loopQueue_t *lq, int speed, int count, int *output ) {
	const mixSound_t *sound;
	int i, leftVol, rightVol;
	int index, indexAdd, indexTotal;
	const short *data;
	float volume;

	if (s_doppler->integer == 2)
		speed = S_MixDopplerFull( speed, lq->origin, lq->velocity );
	else if (s_doppler->integer )
		speed = S_MixDopplerOriginal( speed, lq->origin, lq->velocity );

	volume = s_volume->value * lq->volume;
	S_MixSpatialize( lq->origin, volume, &leftVol, &rightVol );
	sound = S_MixGetSound( loop->handle );

	index = loop->index;
	indexAdd = (sound->speed * speed) >> MIX_SHIFT;
	indexTotal = sound->samples;
	data = sound->data;
	if ( (leftVol | rightVol) <= 0 ) {
		index += count * indexAdd;
		index %= indexTotal;
	} else for (i = 0; i < count;i++) {
		int sample;
		while (index >= indexTotal) {
			index -= indexTotal;
		}
		sample = data[index >> MIX_SHIFT];
		output[i*2+0] += sample * leftVol;
		output[i*2+1] += sample * rightVol;
		index += indexAdd;
	}
	loop->index = index;
}

void S_MixChannels( mixChannel_t *ch, int channels, int speed, int count, int *output ) {
	int queueLeft, freeLeft = channels;
	int activeCount;
	mixChannel_t *free = ch;
	const channelQueue_t *q = s_channelQueue;
	/* Go through the sound queue and add new channels */
	for (queueLeft = s_channelQueueCount; queueLeft > 0; queueLeft--, q++) {
		int scanCount, foundCount;
		mixChannel_t *scanChan;
		if (freeLeft <= 0) {
//			Com_Printf("No more free channels.\n");
			break;
		}
		foundCount = 0;
		scanChan = ch;
		for (scanCount = channels;scanCount > 0;scanCount--, scanChan++ ) {
            /* Large group of tests to see if this one should be counted as a same sound */
			/* Same sound effect ? */
			if ( q->handle != scanChan->handle )
				continue;
			/* Reasonably same start ? */
			if ( scanChan->index > (MIX_SPEED * s_mixSameTime->value))
				continue;
			if ( q->hasOrigin ) {
				vec3_t dist;
				/* Same or close origin */
				if (!scanChan->hasOrigin)
					continue;
				VectorSubtract( q->origin, scanChan->origin, dist );
				if (VectorLengthSquared( dist ) > 50*50)
					continue;
			} else {
				/* Coming from the same entity */
				if (q->entNum != scanChan->entNum )
					continue;
			}
			foundCount++;
			if (foundCount > s_mixSame->integer)
				goto skip_alloc;
		}
		for (;freeLeft > 0;free++, freeLeft--) {
			if (!free->handle) {
				free->handle = q->handle;
				free->entChan = q->entChan;
				free->entNum = q->entNum;
				free->index = 0;
				VectorCopy( q->origin, free->origin );
				free->hasOrigin = q->hasOrigin;
				freeLeft--;
				free++;
				break;
			}
		}
skip_alloc:;
	}
	activeCount = 0;
	for (;channels>0;channels--, ch++) {
		if (ch->handle <= 0 )
			continue;
		activeCount++;
		S_MixChannel( ch, speed, count, output );
	}
}

void S_MixLoops( mixLoop_t *mixLoops, int loopCount, int speed, int count, int *output ) {
	mixLoop_t *loop, *first;
	const loopQueue_t *lq;
	int queueLeft, loopLeft;
	int loopFirst = 0;

	for (queueLeft = s_loopQueueCount, lq = s_loopQueue;queueLeft > 0;queueLeft--, lq++ ) {
		loopLeft = loopCount - loopFirst;
		first = mixLoops + loopFirst++;
		loop = first;
		while ( 1 ) {
			/* Ran out of storage, better just stop mixing for now */
			if ( loopLeft <= 0)
				return;
			/* Reached end of active loop list, use this new one */
			if ( !loop->parent ) {
				if ( loop != first ) {
					*loop = *first;
				}
				first->parent = lq->parent;
				first->index = 0;
				first->handle = lq->handle;
				break;
			/* Found the same parent */
			} else if ( loop->parent == lq->parent ) {
				/* check if we're still playing the same sound */
				if ( lq->handle != loop->handle ) {
					loop->handle = lq->handle;
					first->index = 0;
				}
				/* First entry in the list, else swap it */
				if ( loop != first ) {
					int oldIndex = loop->index;
					*loop = *first;
					first->index = oldIndex;
					first->parent = lq->parent;
					first->handle = lq->handle;
				}
				break;
			/* Not found it yet, forward to the next one */
			} else {
				--loopLeft;
				loop++;
			}
		}
		S_MixLoop( first, lq, speed, count, output );
	}

	loopLeft = loopCount - loopFirst;
	loop = mixLoops + loopFirst;

	for (;loopLeft > 0 && loop->parent; loop++, loopLeft--) {
		loop->parent = 0;
	}
}

void S_MixBackground( mixBackground_t *background, int speed, int count, int *output ) {
	int		volumeMul;
	short	buf[2048][2];

	speed = (MIX_SPEED << MIX_SHIFT) / dma.speed;
	if ( s_background.playing ) {
		/* Do we need a reload */
		if ( s_background.reload ) {
			/* Already playing, check if we already have the right one open */
			if ( background->sound && Q_stricmp( background->soundName, s_background.startName )) {
				S_SoundClose( background->sound );
				background->sound = 0;
			}
			/* Do we have to load a sound */
			if ( !background->sound ) {
                background->sound = S_SoundOpen( s_background.startName );
				Q_strncpyz( background->soundName, s_background.startName, sizeof( background->soundName));
				if ( !background->sound && !s_background.override) {
					/* Regular try the loop sound */
	                background->sound = S_SoundOpen( s_background.loopName );
					Q_strncpyz( background->soundName, s_background.loopName, sizeof( background->soundName));
				}
			}
			if ( background->sound ) {
				/* Should we try a seek with an override */
				if ( s_background.override ) {
					if ( s_background.seekTime >= 0 ) {
						int sample = background->sound->rate * s_background.seekTime;
						S_SoundSeek( background->sound, sample );
						background->length = background->sound->rate * s_background.length;
					} else {
						background->length = 0;
					}
				} else {
					S_SoundSeek( background->sound, 0 );
					background->length = 0;
				}
			}
			background->index = 1 << MIX_SHIFT;
		}
		if (!count)
			return;
		//Special state used to signal stopped music
		if ( s_background.seekTime < 0 && s_background.override ) {
			Com_Memset( output, 0, count * sizeof(int) * 2 );
			return;
		}
		volumeMul = s_musicVolume->value * ( 1 << MIX_SHIFT);
		background->done += count;
		while ( count > 0 ) {
			openSound_t *sound;
			int indexAdd, indexLast, read, need;

			sound = background->sound;
			/* Fill the rest of the block with silence */
			if ( !sound ) {
				Com_Memset( output, 0, count * sizeof(int) * 2 );
				return;
			}
			indexAdd = (sound->rate * speed ) / MIX_SPEED;
			if ( background->length ) {
				background->length -= indexAdd;
				if ( background->length < 0 ) {
					background->length = 0;
					s_background.seekTime = -1;
				}
			}
			indexLast = background->index + count * indexAdd;
			need = (indexLast >> MIX_SHIFT);
			if ( need > (sizeof( buf ) / 4) - 1)
				need = (sizeof( buf ) / 4) - 1;

			/* Is this read gonna take us to the end of the sound stream */
			read = S_SoundRead( sound, qtrue, need, buf[1] );
			if ( read == 0) {
				if ( !s_background.override) {
					if (!strcmp( s_background.loopName, background->soundName))
						S_SoundSeek( sound, 0 );
					else {
						S_SoundClose( background->sound );
						background->sound = S_SoundOpen( s_background.loopName );
						Q_strncpyz( background->soundName, s_background.loopName, sizeof( background->soundName));
					}
					background->index = 1 << MIX_SHIFT;
					if (background->sound)
						continue;
				}
				background->length = 0;
				//Fill the rest of the buffer with 0 with music stopped
				Com_Memset( output, 0, count * sizeof(int) * 2 );
				return;
			} else {
				/* Copy back original last sample if it needs to be processed again */
				buf[0][0] = background->last[0];
				buf[0][1] = background->last[1];

				if ( ((1 + read) << MIX_SHIFT ) < indexLast)
					indexLast = (1 + read) << MIX_SHIFT;
				while ( background->index < indexLast ) {
					int index = background->index >> MIX_SHIFT;
					output[0] = buf[index][0] * volumeMul;
					output[1] = buf[index][1] * volumeMul;
					count--;output+=2;
					background->index += indexAdd;
				}
				indexAdd = (background->index >> MIX_SHIFT);
				background->last[0] = buf[ indexAdd ][0];
				background->last[1] = buf[ indexAdd ][1];
				background->index &= (1 << MIX_SHIFT) - 1;
			}
		}
	} else {
		//Close up the sound file if no longer playing
		if ( background->sound ) {
			S_SoundClose( background->sound );
			background->sound = 0;
		}
		Com_Memset( output, 0, count * sizeof(int) * 2 );
	}
}

void S_MixInfo(void) {

}

void S_MixInit( void ) {
	cvar_t *cv;
	int allocSize;

	if (mixAllocSounds) {
		Com_Memset( mixSounds, 0, sizeof( mixSounds ));
		free( mixAllocSounds );
		mixAllocSounds = 0;
	}
	mixEmptySound.speed = (MIX_SPEED << MIX_SHIFT) / MIX_SPEED;;
	mixEmptySound.samples = 1 << MIX_SHIFT;

	cv = Cvar_Get( "com_soundMegs", DEF_COMSOUNDMEGS, CVAR_LATCH | CVAR_ARCHIVE );
	allocSize = cv->integer * 1024 * 1024;
	/* Sneakily force the soundmegs at atleast 12 mb */
	if (allocSize < 12 * 1024 * 1024)
		allocSize = 12 * 1024 * 1024;
	//Use calloc, seems faster when debugging
	mixAllocSounds = calloc( allocSize, 1 );
	if (!mixAllocSounds) {
		Com_Error (ERR_FATAL, "Failed to allocate memory for sound system\n");
	}
	/* How many similar sounding close to eachother sound effects */
	s_mixSame = Cvar_Get( "s_mixSame", "2", CVAR_ARCHIVE );
	s_mixSameTime = Cvar_Get( "s_mixSameTime", "10", CVAR_ARCHIVE );

	s_effects = Cvar_Get( "s_effects", "1", CVAR_ARCHIVE );
	S_EffectInit();

	/* Init the first block */
	mixAllocSounds->prev = 0;
	mixAllocSounds->next = 0;
	mixAllocSounds->handle = 0;
	mixAllocSounds->size = allocSize;
	mixAllocSounds->lastUsed = 0;
}
