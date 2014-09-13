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

//All mixing speeds have this as 1.0
#define		MIX_SPEED		22050
#define		MIX_SHIFT		8

#define		FIRSCALE		4
#define		ONETHIRD		342*FIRSCALE

typedef struct mixSound_s {
	int			lastUsed;
	int			speed;
	int			samples;
	int			size;
	sfxHandle_t	handle;
	struct		mixSound_s *prev, *next;
	short		data[0];
} mixSound_t;

typedef struct {
	sfxHandle_t		handle;
	int				index;
	vec3_t			origin;
	short			entNum;
	char			entChan;
	char			hasOrigin;
	char			wasMixed;
} mixChannel_t;

typedef struct {
	const void		*parent;
	sfxHandle_t		handle;
	int				index;
} mixLoop_t;

typedef struct {
	openSound_t		*sound;
	char			soundName[MAX_QPATH];
	int				index;
	int				length;
	int				done;
	short			last[2];
} mixBackground_t;

typedef struct {
	int				statel, stater;
	double			zl[ONETHIRD], zr[ONETHIRD];
	int				index;
} mixEffect_t;

extern	cvar_t			*s_effects;

extern void S_DMAInit(void);
extern void S_DMA_Update( float scale );
void S_MMEClearBuffer(void);
void S_DMAClearBuffer(void);

void S_MixInit(void);
void S_MixBackground( mixBackground_t *background, int speed, int count, int *output );
void S_MixChannels( mixChannel_t *ch, int channels, int speed, int count, int *output );
void S_MixLoops( mixLoop_t *loop, int loopCount, int speed, int count, int *output );
void S_MixClipOutput (int count, const int *input, short *output, int outStart, int outMask);
const mixSound_t *S_MixGetSound( sfxHandle_t sfxHandle );

void S_MixEffects(mixEffect_t *effect, int speed, const int count, int *output);
void S_EffectInit(void);
