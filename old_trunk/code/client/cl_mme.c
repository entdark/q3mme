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
// cl_mme.c -- Movie Maker's Edition client-side work

#include "client.h"

extern cvar_t *cl_avidemo;
extern cvar_t *cl_forceavidemo;

// MME cvars
cvar_t	*mme_anykeystopsdemo;
cvar_t	*mme_screenshot_format;
cvar_t	*mme_jpeg_quality;
cvar_t	*mme_jpeg_downsample_chroma;
cvar_t	*mme_jpeg_optimize_huffman_tables;
cvar_t	*mme_wavdemo_enabled;
cvar_t	*mme_tga_rle_compress;

cvar_t	*mme_forcemodelplayer1;
cvar_t	*mme_forcemodelplayer2;
cvar_t	*mme_forcemodelplayer3;
cvar_t	*mme_forcemodelplayer4;
cvar_t	*mme_forcemodelplayer5;
cvar_t	*mme_forcemodelplayer6;
cvar_t	*mme_forcemodelplayer7;
cvar_t	*mme_forcemodelplayer8;
cvar_t	*mme_forcemodelplayer9;
cvar_t	*mme_forcemodelplayer10;
cvar_t	*mme_forcemodelplayer11;
cvar_t	*mme_forcemodelplayer12;
cvar_t	*mme_forcemodelplayer13;
cvar_t	*mme_forcemodelplayer14;
cvar_t	*mme_forcemodelplayer15;
cvar_t	*mme_forcemodelplayer16;

void CL_MME_CheckCvarChanges(void) {
	int i, checkwavfile = 0;
	
	// Force models
	for (i = 0; i < MAX_CLIENTS; i++) {
		// MMETODO: optimize checking for cvar modification
		cvar_t *consolevar;
		
		consolevar = Cvar_FindVar(va("mme_forcemodelplayer%i", i+1));
		if (!consolevar) continue;
		
		if (consolevar->modified) {
			char cs[MAX_INFO_STRING];
			int ofs;
			int index;
			char cmd[MAX_GAMESTATE_CHARS];
			
			consolevar->modified = qfalse;
			if ( cls.state != CA_ACTIVE ) continue; // Everything below is for active client.
			
			ofs = cl.gameState.stringOffsets[i+CS_PLAYERS];
			if (!ofs) continue;
			
			// Make a fake serverCommand.
			strcpy(cs, cl.gameState.stringData + ofs);
			Info_SetValueForKey(cs, "model", consolevar->string);
			Info_SetValueForKey(cs, "hmodel", consolevar->string);
			
			Com_sprintf(cmd, MAX_GAMESTATE_CHARS, "cs %i \"%s\"\n", i+CS_PLAYERS, cs);
			
			clc.numFakeServerCommands++;
			index = (clc.numFakeServerCommands) & (32-1);
			Q_strncpyz(clc.fakeServerCommands[index], cmd, MAX_STRING_CHARS);
		}
	}

	if (cl_avidemo->modified) {
		cl_avidemo->modified = qfalse;
		checkwavfile = 1;
	}
	if (mme_wavdemo_enabled->modified) {
		mme_wavdemo_enabled->modified = qfalse;
		checkwavfile = 1;
	}

	if (checkwavfile) {
		if ((com_cl_running->integer) && 
			(cl_avidemo->integer) && 
			((cls.state == CA_ACTIVE) || (cl_forceavidemo->integer)) &&
			(mme_wavdemo_enabled->integer)) {
			S_MME_WavOpen(clc.demoName);
		} else {
			S_MME_WavClose();
		}
	}
}


// Dump player numbers, their names and models they're using
void CL_MME_PlayerNumbers_f(void) {
	int i, ofs;
	const char *cs;

	if ( cls.state != CA_ACTIVE ) {
		Com_Printf( "Not connected to a server.\n");
		return;
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		char *model, *headmodel, *name;
		ofs = cl.gameState.stringOffsets[i+CS_PLAYERS];
		if (!ofs) continue;

		cs = cl.gameState.stringData + ofs;
		model = Info_ValueForKey(cs, "model");
		Com_Printf("player%2i: model \"%s\", ", i+1, model);
		headmodel = Info_ValueForKey(cs, "hmodel");
		if (strcmp(model, headmodel)) {
			Com_Printf("headmodel \"%s\", ", headmodel);
		}
		name = Info_ValueForKey(cs, "n");
		Com_Printf("name \"%s\"\n", name);
	}
}

