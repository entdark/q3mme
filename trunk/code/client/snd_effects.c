// Copyright 2000-2007 by Grant R. Griffin for "FIR filter algorithms for use in C".
// The original source code of "FIR filter algorithms for use in C" is open under The Wide Open License (WOL).

#include "snd_local.h"
#include "snd_mix.h"

static double		hh[ONETHIRD]; //hh represents equalizer
static double		hh2[2*ONETHIRD]; //same, used for the last method - fir_double_h

static sfxHandle_t	underWater;

#define UNDERWATER_SCALE 0.02
#define	UNDERWATER_VOLUME 0.8

void S_EffectInit(void) {
	int i;
	for (i = 0; i < ONETHIRD; i++) {
		if (i < (int)(0.55*ONETHIRD))
			hh[i] = UNDERWATER_SCALE;
		else if (i >= (int)(0.55*ONETHIRD) && i < (int)(0.64*ONETHIRD))
			hh[i] = UNDERWATER_SCALE * (1.0 - ((double)i - (int)(0.55*ONETHIRD)) / ((int)(0.64*ONETHIRD) - (int)(0.55*ONETHIRD))); //linear fading
		else
			hh[i] = 0.0;
		hh2[i] = hh2[i+ONETHIRD] = hh[i];
	}
	underWater = S_RegisterSound("data/underwater_loop", qfalse);
}

/****************************************************************************
* fir_basic: This does the basic FIR algorithm: store input sample, calculate  
* output sample, shift delay line                                           
*****************************************************************************/
static double S_EffectBasicFIR(const double input, const int ntaps, const double h[], double z[]) {
    int ii;
    double accum;
    
    /* store input at the beginning of the delay line */
    z[0] = input;

    /* calc FIR */
    accum = 0;
    for (ii = 0; ii < ntaps; ii++) {
        accum += h[ii] * z[ii];
    }

    /* shift delay line */
    for (ii = ntaps - 2; ii >= 0; ii--) {
        z[ii + 1] = z[ii];
    }

    return accum;
}

/****************************************************************************
* fir_circular: This function illustrates the use of "circular" buffers
* in FIR implementations.  The advantage of circular buffers is that they
* alleviate the need to move data samples around in the delay line (as
* was done in all the functions above).  Most DSP microprocessors implement
* circular buffers in hardware, which allows a single FIR tap to be
* calculated in a single instruction.  That works fine when programming in
* assembly, but since C doesn't have any constructs to represent circular
* buffers, you need to fake them in C by adding an extra "if" statement
* inside the FIR calculation.
*****************************************************************************/
static double S_EffectCircularFIR(const double input, const int ntaps, const double h[], double z[], int *p_state) {
    int ii, state;
    double accum;

    state = *p_state;               /* copy the filter's state to a local */

    /* store input at the beginning of the delay line */
    z[state] = input;
    if (++state >= ntaps) {         /* incr state and check for wrap */
        state = 0;
    }

    /* calc FIR and shift data */
    accum = 0;
    for (ii = ntaps - 1; ii >= 0; ii--) {
        accum += h[ii] * z[state];
        if (++state >= ntaps) {     /* incr state and check for wrap */
            state = 0;
        }
    }

    *p_state = state;               /* return new state to caller */

    return accum;
}

/****************************************************************************
* fir_shuffle: This is like fir_basic, except that data is shuffled by     
* moving it _inside_ the calculation loop.  This is similar to the MACD    
* instruction on TI's fixed-point processors.       
*****************************************************************************/
static double S_EffectShuffleFIR(const double input, const int ntaps, const double h[], double z[]) {
    int ii;
    double accum;
    
    /* store input at the beginning of the delay line */
    z[0] = input;

    /* calc FIR and shift data */
    accum = h[ntaps - 1] * z[ntaps - 1];
    for (ii = ntaps - 2; ii >= 0; ii--) {
        accum += h[ii] * z[ii];
        z[ii + 1] = z[ii];
    }

    return accum;
}

/****************************************************************************
* fir_split: This splits the calculation into two parts so the circular    
* buffer logic doesn't have to be done inside the calculation loop.   
*****************************************************************************/
static double S_EffectSplitFIR(const double input, const int ntaps, const double h[], double z[], int *p_state) {
    int ii, end_ntaps, state = *p_state;
    double accum;
    double const *p_h;
    double *p_z;

    /* setup the filter */
    accum = 0;
    p_h = h;

    /* calculate the end part */
    p_z = z + state;
    *p_z = input;
    end_ntaps = ntaps - state;
    for (ii = 0; ii < end_ntaps; ii++) {
        accum += *p_h++ * *p_z++;
    }

    /* calculate the beginning part */
    p_z = z;
    for (ii = 0; ii < state; ii++) {
        accum += *p_h++ * *p_z++;
    }

    /* decrement the state, wrapping if below zero */
    if (--state < 0) {
        state += ntaps;
    }
    *p_state = state;       /* pass new state back to caller */

    return accum;
}

/****************************************************************************
* fir_double_z: This uses a doubled delay line so the FIR calculation always
* operates on a flat buffer. 
*****************************************************************************/
static double S_EffectDoubleZFIR(const double input, const int ntaps, const double h[], double z[], int *p_state) {
    double accum;
    int ii, state = *p_state;
    double const *p_h, *p_z;

    /* store input at the beginning of the delay line as well as ntaps more */
    z[state] = z[state + ntaps] = input;

    /* calculate the filter */
    p_h = h;
    p_z = z + state;
    accum = 0;
    for (ii = 0; ii < ntaps; ii++) {
        accum += *p_h++ * *p_z++;
    }

    /* decrement state, wrapping if below zero */
    if (--state < 0) {
        state += ntaps;
    }
    *p_state = state;       /* return new state to caller */

    return accum;
}

/****************************************************************************
* fir_double_h: This uses doubled coefficients (supplied by caller) so that 
* the filter calculation always operates on a flat buffer.
*****************************************************************************/
static double S_EffectDoubleHFIR(const double input, const int ntaps, const double h[], double z[], int *p_state) {
    double accum;
    int ii, state = *p_state;
    double const *p_h, *p_z;

    /* store input at the beginning of the delay line */
    z[state] = input;

    /* calculate the filter */
    p_h = h + ntaps - state;
    p_z = z;
    accum = 0;
    for (ii = 0; ii < ntaps; ii++) {
        accum += *p_h++ * *p_z++;
    }

    /* decrement state, wrapping if below zero */
    if (--state < 0) {
        state += ntaps;
    }
    *p_state = state;       /* return new state to caller */

    return accum;
}

static double S_EffectFIR(const double input, const int ntaps, const double h[], double z[], int *p_state) {
	switch(s_effects->integer) {
	default:
	case 1:
		return S_EffectBasicFIR(input, ntaps, h, z);
	case 2:
		return S_EffectCircularFIR(input, ntaps, h, z, p_state);
	case 3:
		return S_EffectShuffleFIR(input, ntaps, h, z);
	case 4:
		return S_EffectSplitFIR(input, ntaps, h, z, p_state);
	case 5:
		return S_EffectDoubleZFIR(input, ntaps, h, z, p_state);
	case 6:
		return S_EffectDoubleHFIR(input, ntaps, hh2, z, p_state);
	}
}

void S_EffectUnderWater(mixEffect_t *effect, int speed, const int count, int *output) {
	double in[FIRSCALE*2048];
	int ntaps = count / 3;

	const mixSound_t *sound;
	int i, volume;
	int index, indexAdd, indexTotal;
	const short *data;

	if(ntaps <= 0)
		ntaps = 1;
	for (i = 0; i < count; i++) {
		in[i*2+0] = S_EffectFIR(output[i*2+0], ntaps, hh, effect->zl, &effect->statel);
		in[i*2+1] = S_EffectFIR(output[i*2+1], ntaps, hh, effect->zr, &effect->stater);
    }
	for (i = 0; i < count; i++) {
        output[i*2+0] = (int)in[i*2+0]*UNDERWATER_VOLUME;
		output[i*2+1] = (int)in[i*2+1]*UNDERWATER_VOLUME;
    }

	if (!underWater)
		return;
	/* the underwater echo sounds "data/underwater_loop" */
	volume = s_volume->value * (1 << MIX_SHIFT) * 0.5f;
	sound = S_MixGetSound(underWater);

	index = effect->index;
	indexAdd = (sound->speed * speed) >> MIX_SHIFT;
	indexTotal = sound->samples;
	data = sound->data;
	for (i = 0; i < count; i++) {
		int sample;
		if (index >= indexTotal) {
			index -= indexTotal;
		}
		sample = data[index >> MIX_SHIFT];
		output[i*2+0] += sample * volume;
		output[i*2+1] += sample * volume;
		index += indexAdd;
	}
	effect->index = index;
}

void S_MixEffects(mixEffect_t *effect, int speed, const int count, int *output) {
	if (!s_effects->integer)
		return;
	if (s_underWater)
		S_EffectUnderWater(effect, speed, count, output);
}
