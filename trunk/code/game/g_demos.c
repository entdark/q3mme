#include "g_local.h"
#include "g_demos.h"
#include <stdio.h>

static struct {
	demoPlay_t *play;
	qboolean	playPaused;
	float		playSpeed;
	int			playTime;
	float		playFraction;
	demoRecord_t *record;
} serverdemo;

char *G_DemosClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	gclient_t	*client = level.clients + clientNum;
	gentity_t	*ent = g_entities + clientNum;
	
	ent->client = client;
	memset(client, 0, sizeof(gclient_t));
//CANATODO, check for ip and bans?
	trap_SendServerCommand( clientNum, "print \"Joined a server playing back a demo\n\"" );
	return 0;
}

void G_DemosClientBegin(int clientNum) {
	gclient_t	*client = level.clients + clientNum;
	gentity_t	*ent = g_entities + clientNum;
	
	G_InitGentity( ent );
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->ps.pm_type = PM_SPECTATOR;
	VectorCopy( level.intermission_origin, client->ps.origin );
	VectorCopy( level.intermission_angle, client->ps.viewangles );
}

void G_DemosClientCommand(int clientNum) {

}

void G_DemosClientThink(int clientNum) {
	gentity_t	*ent = g_entities + clientNum;
	gclient_t	*client = level.clients + clientNum;
	usercmd_t	*ucmd = &client->pers.cmd;
	int		msec;
	pmove_t pm;

	trap_GetUsercmd( clientNum, ucmd );
	msec = ucmd->serverTime - client->ps.commandTime;
	if ( msec < 1)
		return;
	if ( msec > 200 )
		msec = 200;

	client->ps.speed = 440;
	client->ps.pm_type = PM_SPECTATOR;
	// set up for pmove
	memset (&pm, 0, sizeof(pm));
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
	pm.trace = trap_TraceNoEnts;
	pm.pointcontents = trap_PointContents;

	// perform a pmove
	Pmove (&pm);

	BG_PlayerStateToEntityState( &client->ps, &ent->s, qfalse );

	VectorCopy( client->ps.origin, ent->r.currentOrigin );
	//	trap_LinkEntity( ent );
}

void G_DemosClientDisconnect( int clientNum ) {

}
void G_DemosClientUserinfoChanged( int clientNum ) {

}

void G_DemosRunFrame( int levelTime ) {
	int i, msec;
	char buf[1024];
	int demoState;

	level.framenum++;
	level.previousTime = level.time;
	level.time = levelTime;
	msec = level.time - level.previousTime;
	if (msec > 100)
		msec = 100;
	if (!serverdemo.playPaused) {
		serverdemo.playFraction += msec * serverdemo.playSpeed;
		serverdemo.playTime += (int)serverdemo.playFraction;
		serverdemo.playFraction -= (int)serverdemo.playFraction;
	}
	if (serverdemo.playTime < 0) {
		serverdemo.playTime = serverdemo.playFraction = 0;
	}
	demoPlayFillFrame( serverdemo.play, serverdemo.playTime, serverdemo.playFraction );
	for (i=0;i<MAX_GENTITIES;i++) {
		gentity_t *ent = &g_entities[i];
		int	demoFlags = demoPlayGetEntityState( serverdemo.play, i, &ent->s);

		if (demoFlags & demoFLAG_ENT_INVALID) {
			trap_UnlinkEntity( ent );
		} else {
			ent->r.bmodel = (demoFlags & demoFLAG_ENT_SOLID_MODEL) ? qtrue : qfalse;
			ent->r.svFlags = 0;
			if (demoFlags & demoFLAG_ENT_BROADCAST)
				ent->r.svFlags |= SVF_BROADCAST;
			if (ent->r.bmodel && (ent->soundPos1 != (ent->s.modelindex+1))) {
				char model[10];
				ent->soundPos1 = (ent->s.modelindex+1);
				Com_sprintf ( model, sizeof(model), "*%d", ent->s.modelindex);
				trap_SetBrushModel( ent, model );
				trap_AdjustAreaPortalState( ent, qtrue );
			}
			ent->r.contents = 0;
			VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
			VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );
			trap_LinkEntity( ent );
		}
	}
	
	for (i=0;i<CS_MAX;i++) {
		const char *str;
		if (demoIgnoreString[i])
			continue;
		str = demoPlayGetString( serverdemo.play, i );
		trap_GetConfigstring( i, buf, sizeof(buf));
		if (strncmp(buf, str, sizeof(buf))) {
			trap_SetConfigstring( i, str);
		}
	}
	demoState = DSF_ACTIVE | DSF_PLAYING;
	for (i=0;i<MAX_CLIENTS;i++) {
		gclient_t	*client = level.clients + i;
		//Store the demo state flags
		client->ps.demoState = demoState;
		client->ps.weaponTime = serverdemo.playTime;
		client->ps.weaponDelay = 0;
	}
}

void G_DemosRecordFrame(void) {
	int i;
	if (!serverdemo.record)
		return;
	demoRecordEndFrame( serverdemo.record, level.time - level.previousTime );
	demoRecordStartFrame( serverdemo.record, level.time );
	for (i=0; i<MAX_GENTITIES; i++) {
		gentity_t * ent = &g_entities[i];
		int demoFlags;
		if (!ent->inuse)
			continue;
		if (!ent->r.linked)
			continue;
		if (ent->r.svFlags & ( SVF_NOCLIENT | SVF_SINGLECLIENT))
			continue;
		demoFlags = 0;
		if (ent->r.bmodel)
			demoFlags |= demoFLAG_ENT_SOLID_MODEL;
		if (ent->r.svFlags & SVF_BROADCAST)
			demoFlags |= demoFLAG_ENT_BROADCAST;
		demoRecordEntityState( serverdemo.record, &ent->s, demoFlags );
	}
	for (i=0; i<MAX_CLIENTS; i++) {
		gentity_t * ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		demoRecordPlayerState( serverdemo.record, &ent->client->ps );
	}
	for (i=0; i<CS_MAX; i++) {
		uchar line[512];
		
		if (demoIgnoreString[i])
			continue;
		line[0] = 0;
        trap_GetConfigstring( i, line, sizeof(line));
		demoAddString( serverdemo.record, i, line);
	}
}



void G_DemosStartPlayback(void) {
	char fileName[MAX_QPATH];
	char mapName[MAX_QPATH];
	demoInfo_t info;
	int i;

	trap_Cvar_VariableStringBuffer( "mapname", mapName, sizeof(mapName) );	
	trap_Cvar_VariableStringBuffer( "g_demoFile", fileName, sizeof(fileName) );	
	trap_Cvar_Set( "g_demoFile", "");
	trap_Cvar_Set( "g_demoPlaying", "0");
	g_demoPlaying.integer = 0;
	demoGetInfo( fileName, &info);
	if (strncmp( mapName, info.mapName, sizeof( info.mapName)))
		return;
	serverdemo.play = demoPlayStart( fileName );
	if (!serverdemo.play) 
		return;
	serverdemo.playPaused = qtrue;
	serverdemo.playSpeed = 1;
	/* Find some special entities */
	level.numSpawnVars = 0;
	level.spawnIndex = 0;
	while( G_ParseSpawnVars() ) {
		char *classname = 0, *origin = 0, *angles = 0, *model=0;
		for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
			if (!Q_stricmp( "classname", level.spawnVars[i][0]))
				classname = level.spawnVars[i][1];
			else if (!Q_stricmp( "origin", level.spawnVars[i][0]))
				origin = level.spawnVars[i][1];
			else if (!Q_stricmp( "angles", level.spawnVars[i][0]))
				angles = level.spawnVars[i][1];
			else if (!Q_stricmp( "model", level.spawnVars[i][0]))
				model = level.spawnVars[i][1];
		}
		if (!classname)
			continue;
		if (!Q_stricmp("info_player_intermission", classname)) {
			if (origin) 
				sscanf (origin, "%f %f %f", &level.intermission_origin[0],&level.intermission_origin[1],&level.intermission_origin[2]);
			if (angles)
				sscanf (angles, "%f %f %f", &level.intermission_angle[0],&level.intermission_angle[1],&level.intermission_angle[2]);
//		} else if (0 && model && (
//			!Q_stricmp("func_door", classname) ||
//			!Q_stricmp("func_door_rotating", classname) )) {
//			gentity_t *ent = level.gentities + ENTITYNUM_WORLD;
//			trap_SetBrushModel( ent, model );
//			trap_AdjustAreaPortalState( ent, qtrue );
		}
	}

	/* We are playing a demo */
	trap_Cvar_Set( "g_demoPlaying", "1");
	//This seems to lag in init game so let's do it like this
	g_demoPlaying.integer = 1;
	return;
}

void G_DemosShutdown( int restart ) {
	if (serverdemo.record) {
		demoRecordStop( serverdemo.record );
		serverdemo.record = 0;
	}
}

void G_DemosServerDemoCommand(void) {
	char buf[MAX_QPATH];
	char fileName[MAX_QPATH];
	trap_Argv(1, buf, sizeof(buf));
	if (!Q_stricmp(buf, "record")) {
		if (serverdemo.record)
			return;
		trap_Argv(2, buf, sizeof(buf));
		if (!demoCreateFileName(buf, fileName)) {
			Com_Printf("Failed to open %s for recording\n", fileName );		
			return;
		}
		trap_Cvar_VariableStringBuffer( "mapname", buf, sizeof(buf) );
		serverdemo.record = demoRecordStart( fileName, buf );
		Com_Printf("Recording serverdemo to %s\n", fileName );		
	} else if (!Q_stricmp(buf, "info")) {
		demoInfo_t info;
		trap_Argv(2, buf, sizeof(buf));
		if (!demoCreateFileName(buf, fileName))
			return;
		demoGetInfo( fileName, &info );
		if (!strlen(info.mapName))
			return;
		Com_Printf("demo for map %s time %d frames %d\n",info.mapName, info.totalTime / 1000, info.totalFrames);
	} else if (!Q_stricmp(buf, "play" )) {
		demoInfo_t info;
		trap_Argv(2, buf, sizeof(buf));
		if (!demoCreateFileName(buf, fileName)) {
			Com_Printf("Failed to open %s for playing\n", fileName );		
			return;
		}
		demoGetInfo( fileName, &info );
		if (!strlen(info.mapName)) {
			Com_Printf("Failed to get info from %s\n", fileName );		
			return;
		}
		trap_Cvar_Set( "g_demoFile", fileName);
		trap_Cvar_Set( "g_demoPlaying", "0");
		trap_SendConsoleCommand(EXEC_APPEND, va("devmap \"%s\"\n", info.mapName));
	} else if (!Q_stricmp(buf, "pause")) {
		if (!serverdemo.play)
			return;
		serverdemo.playPaused = !serverdemo.playPaused;
	} else if (!Q_stricmp(buf, "seek")) {
		if (!serverdemo.play)
			return;
		trap_Argv(2, buf, sizeof(buf));
		if (buf[0] == '+') {
			if (Q_isdigit( buf[1])) {
				serverdemo.playTime += atof( buf + 1 ) * 1000;
				serverdemo.playFraction = 0;
			}
		} else if (buf[0] == '-') {
			if (Q_isdigit( buf[1])) {
				serverdemo.playTime -= atof( buf + 1 ) * 1000;
				serverdemo.playFraction = 0;
			}
		} else if (Q_isdigit( buf[0] )) {
			serverdemo.playTime = atof( buf ) * 1000;
			serverdemo.playFraction = 0;
		}
	} else if (!Q_stricmp(buf, "speed")) {
		if (!serverdemo.play)
			return;
		trap_Argv(2, buf, sizeof(buf));
		serverdemo.playSpeed = atof(buf);
		G_Printf("Speed at %f\n", serverdemo.playSpeed );
	} else if (!Q_stricmp(buf, "stoprecord")) {
		if (serverdemo.record) {
			G_Printf("Stopping recording to %s, recorded %d seconds\n",
				serverdemo.record->fileName,
				serverdemo.record->totalTime / 1000
				);
			demoRecordStop( serverdemo.record );
			serverdemo.record = 0;
		} else {
			G_Printf("Not recording a serverdemo\n");
		}
	} else {
		G_Printf("Usage: serverdemo record/stoprecord/play/info/pause/seek/list\n");
	}
}