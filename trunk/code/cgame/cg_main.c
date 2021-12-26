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
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"

void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum );
void CG_Shutdown( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
extern void CG_DemosDrawActiveFrame( int serverTime, stereoFrame_t stereoView );
extern qboolean CG_DemosConsoleCommand( void );
long vmMain( long command, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6, long arg7, long arg8, long arg9, long arg10, long arg11  ) {

	switch ( command ) {
	case CG_INIT:
		CG_Init( arg0, arg1, arg2 );
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		if (cg.demoPlayback == 2)
			return CG_DemosConsoleCommand();
		else 
			return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		if (arg2 == 2) {
			CG_DemosDrawActiveFrame( arg0, arg1 );
		} else {
			CG_DrawActiveFrame( arg0, arg1, arg2 );
		}
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		return CG_KeyEvent(arg0, arg1);;
	case CG_MOUSE_EVENT:
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0);
		return 0;
	default:
		CG_Error( "vmMain: unknown command %i", command );
		break;
	}
	return -1;
}


static void CG_Set2DRatio(void) {
	if (mov_ratioFix.integer)
		cgs.widthRatioCoef = (float)(SCREEN_WIDTH * cgs.glconfig.vidHeight) / (float)(SCREEN_HEIGHT * cgs.glconfig.vidWidth);
	else
		cgs.widthRatioCoef = 1.0f;
}

static void CG_SetMovementKeysPos( void ) {
	if ( sscanf( cg_drawMovementKeysPos.string, "%f %f", &cg.moveKeysPos[0], &cg.moveKeysPos[1] ) != 2 ) {
		cg.moveKeysPos[0] = (SCREEN_WIDTH / 2);
		cg.moveKeysPos[1] = (SCREEN_HEIGHT / 2);
	}
}

static void CG_SetSpedometerPos( void ) {
	if ( sscanf( cg_drawSpeedometerPos.string, "%f %f", &cg.speedPos[0], &cg.speedPos[1] ) != 2 ) {
		cg.speedPos[0] = (SCREEN_WIDTH / 2);
		cg.speedPos[1] = (SCREEN_HEIGHT / 2);
	}
}

static void CG_SetNewSkin(void) {
	int i;
	for ( i = 0; i < MAX_CLIENTS ; i++ )
		CG_NewClientInfo( i );
}


cg_t				cg;
cgs_t				cgs;
centity_t			cg_entities[MAX_GENTITIES];
weaponInfo_t		cg_weapons[MAX_WEAPONS];
itemInfo_t			cg_items[MAX_ITEMS];


vmCvar_t	cg_railTrailTime;
vmCvar_t	cg_centertime;
vmCvar_t	cg_runpitch;
vmCvar_t	cg_runroll;
vmCvar_t	cg_damageKick;
vmCvar_t	cg_bobup;
vmCvar_t	cg_bobpitch;
vmCvar_t	cg_bobroll;
vmCvar_t	cg_swingSpeed;
vmCvar_t	cg_shadows;
vmCvar_t	cg_gibs;
vmCvar_t	cg_drawTimer;
vmCvar_t	cg_drawFPS;
vmCvar_t	cg_drawSnapshot;
vmCvar_t	cg_draw3dIcons;
vmCvar_t	cg_drawIcons;
vmCvar_t	cg_drawAmmoWarning;
vmCvar_t	cg_drawCrosshair;
vmCvar_t	cg_drawCrosshairNames;
vmCvar_t	cg_drawRewards;
vmCvar_t	cg_crosshairSize;
vmCvar_t	cg_crosshairX;
vmCvar_t	cg_crosshairY;
vmCvar_t	cg_crosshairHealth;
vmCvar_t	cg_draw2D;
vmCvar_t	cg_drawStatus;
vmCvar_t	cg_animSpeed;
vmCvar_t	cg_debugAnim;
vmCvar_t	cg_debugPosition;
vmCvar_t	cg_debugEvents;
vmCvar_t	cg_errorDecay;
vmCvar_t	cg_nopredict;
vmCvar_t	cg_noPlayerAnims;
vmCvar_t	cg_showmiss;
vmCvar_t	cg_footsteps;
vmCvar_t	cg_addMarks;
vmCvar_t	cg_brassTime;
vmCvar_t	cg_viewsize;
vmCvar_t	cg_drawGun;
vmCvar_t	cg_gun_frame;
vmCvar_t	cg_gun_x;
vmCvar_t	cg_gun_y;
vmCvar_t	cg_gun_z;
vmCvar_t	cg_tracerChance;
vmCvar_t	cg_autoswitch;
vmCvar_t	cg_ignore;
vmCvar_t	cg_simpleItems;
vmCvar_t	cg_fov;
vmCvar_t	cg_zoomFov;
vmCvar_t	cg_thirdPerson;
vmCvar_t	cg_thirdPersonRange;
vmCvar_t	cg_thirdPersonAngle;
vmCvar_t	cg_stereoSeparation;
vmCvar_t	cg_lagometer;
vmCvar_t	cg_drawAttacker;
vmCvar_t	cg_synchronousClients;
vmCvar_t 	cg_teamChatTime;
vmCvar_t 	cg_teamChatHeight;
vmCvar_t 	cg_buildScript;
vmCvar_t	cg_paused;
vmCvar_t	cg_blood;
vmCvar_t	cg_predictItems;
vmCvar_t	cg_drawFriend;
vmCvar_t	cg_teamChatsOnly;
vmCvar_t	cg_noVoiceChats;
vmCvar_t	cg_noVoiceText;
vmCvar_t	cg_hudFiles;
vmCvar_t 	cg_scorePlum;
vmCvar_t 	cg_smoothClients;
vmCvar_t	pmove_fixed;
//vmCvar_t	cg_pmove_fixed;
vmCvar_t	pmove_msec;
vmCvar_t	cg_pmove_msec;
vmCvar_t	cg_cameraMode;
vmCvar_t	cg_cameraOrbit;
vmCvar_t	cg_cameraOrbitDelay;
vmCvar_t	cg_timescaleFadeEnd;
vmCvar_t	cg_timescaleFadeSpeed;
vmCvar_t	cg_timescale;
vmCvar_t	cg_smallFont;
vmCvar_t	cg_bigFont;
vmCvar_t	cg_noTaunt;
vmCvar_t	cg_noProjectileTrail;
vmCvar_t	cg_oldRail;
vmCvar_t	cg_oldRocket;
vmCvar_t	cg_oldPlasma;
vmCvar_t	cg_trueLightning;

vmCvar_t	cg_drawDamageBlob;
vmCvar_t	cg_drawPowerups;
vmCvar_t	cg_drawPickupItem;
vmCvar_t	cg_drawVote;
vmCvar_t	cg_drawFollow;
vmCvar_t	cg_drawScores;
vmCvar_t	cg_drawWeaponSelect;
vmCvar_t	cg_centerScale;
vmCvar_t	cg_centerPrint;
vmCvar_t	cg_gibDirectional;
vmCvar_t	cg_muzzleFlash;

vmCvar_t	cg_commandSmooth;

vmCvar_t	cg_drawSpeedometer;
vmCvar_t	cg_drawSpeedometerScale;
vmCvar_t	cg_drawSpeedometerPos;
vmCvar_t	cg_drawSpeedometerFormat;
vmCvar_t	cg_drawSpeedometerAlignment;

vmCvar_t	cg_drawMovementKeys;
vmCvar_t	cg_drawMovementKeysPos;
vmCvar_t	cg_drawMovementKeysScale;

vmCvar_t	mov_Obituaries;
vmCvar_t	mov_chatBeep;
vmCvar_t	mov_fragFormat;
vmCvar_t	mov_fragOnly;
vmCvar_t	mov_gameType;
vmCvar_t	mov_debug;
vmCvar_t	mov_captureCamera;
vmCvar_t	mov_captureName;
vmCvar_t	mov_captureFPS;

vmCvar_t	mov_filterMask;
vmCvar_t	mov_stencilMask;
vmCvar_t	mov_seekInterval;
vmCvar_t	mov_musicFile;
vmCvar_t	mov_musicStart;
vmCvar_t	mov_chaseRange;
vmCvar_t	mov_fontName;
vmCvar_t	mov_fontSize;
vmCvar_t	mov_teamSkins;

vmCvar_t	mov_gridOffset;
vmCvar_t	mov_gridWidth;
vmCvar_t	mov_gridStep;
vmCvar_t	mov_gridRange;
vmCvar_t	mov_gridColor;
vmCvar_t	mov_hudOverlay;

vmCvar_t	mov_deltaYaw;
vmCvar_t	mov_deltaPitch;
vmCvar_t	mov_deltaRoll;

vmCvar_t	mov_ratioFix;
vmCvar_t	mov_rewardCount;
vmCvar_t	mov_bobScale;
vmCvar_t	mov_wallhack;
vmCvar_t	mov_smoothCamPos;

vmCvar_t	mov_hitSounds;
vmCvar_t	mov_chatBox;
vmCvar_t	mov_chatBoxHeight;
vmCvar_t	mov_chatBoxScale;

vmCvar_t	mme_demoFileName;

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
} cvarTable_t;

static cvarTable_t cvarTable[] = { // bk001129
	{ &cg_ignore, "cg_ignore", "0", NULL, 0 },	// used for debugging
	{ &cg_autoswitch, "cg_autoswitch", "1", NULL, CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", NULL, CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", NULL, CVAR_ARCHIVE },
	{ &cg_fov, "cg_fov", "90", NULL, CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", NULL, CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", NULL, CVAR_ARCHIVE  },
	{ &cg_shadows, "cg_shadows", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_gibs, "cg_gibs", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_draw2D, "cg_draw2D", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawStatus, "cg_drawStatus", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawTimer, "cg_drawTimer", "0", NULL, CVAR_ARCHIVE  },
	{ &cg_drawFPS, "cg_drawFPS", "0", NULL, CVAR_ARCHIVE  },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", NULL, CVAR_ARCHIVE  },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawIcons, "cg_drawIcons", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", NULL, CVAR_ARCHIVE  },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "4", NULL, CVAR_ARCHIVE },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", NULL, CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", NULL, CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", NULL, CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "1", NULL, CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", NULL, CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", NULL, CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", NULL, CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", NULL, CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", NULL, CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", NULL, CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", NULL, CVAR_ARCHIVE  },
	{ &cg_gun_x, "cg_gunX", "0", NULL, CVAR_CHEAT },
	{ &cg_gun_y, "cg_gunY", "0", NULL, CVAR_CHEAT },
	{ &cg_gun_z, "cg_gunZ", "0", NULL, CVAR_CHEAT },
	{ &cg_centertime, "cg_centertime", "3", NULL, CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.002", NULL, CVAR_ARCHIVE},
	{ &cg_runroll, "cg_runroll", "0.005", NULL, CVAR_ARCHIVE },
	{ &cg_damageKick , "cg_damageKick", "0", NULL, CVAR_ARCHIVE },
	{ &cg_bobup , "cg_bobup", "0.005", NULL, CVAR_CHEAT },
	{ &cg_bobpitch, "cg_bobpitch", "0.002", NULL, CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.002", NULL, CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", NULL, CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", NULL, CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", NULL, CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", NULL, CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", NULL, CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100", NULL, 0 },
	{ &cg_nopredict, "cg_nopredict", "0", NULL, 0 },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", NULL, CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0", NULL, 0 },
	{ &cg_footsteps, "cg_footsteps", "1", NULL, CVAR_CHEAT },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", NULL, CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "60", NULL, CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", NULL, CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0", NULL, 0 },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", NULL, CVAR_ARCHIVE  },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", NULL, CVAR_ARCHIVE  },
	{ &cg_predictItems, "cg_predictItems", "1", NULL, CVAR_ARCHIVE },
	{ &cg_drawFriend, "cg_drawFriend", "1", NULL, CVAR_ARCHIVE },
	{ &cg_teamChatsOnly, "cg_teamChatsOnly", "0", NULL, CVAR_ARCHIVE },
	{ &cg_noVoiceChats, "cg_noVoiceChats", "0", NULL, CVAR_ARCHIVE },
	{ &cg_noVoiceText, "cg_noVoiceText", "0", NULL, CVAR_ARCHIVE },
	// the following variables are created in other parts of the system,
	// but we also reference them here
	{ &cg_buildScript, "com_buildScript", "0", NULL, 0 },	// force loading of all possible data amd error on failures
	{ &cg_paused, "cl_paused", "0", NULL, CVAR_ROM },
	{ &cg_blood, "com_blood", "1", NULL, CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0", NULL, 0 },	// communicated by systeminfo
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", NULL, CVAR_CHEAT},
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", NULL, CVAR_ARCHIVE},
	{ &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", NULL, 0},
	{ &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", NULL, 0},
	{ &cg_timescale, "timescale", "1", NULL, 0},
	{ &cg_scorePlum, "cg_scorePlums", "1", NULL, CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_smoothClients, "cg_smoothClients", "0", NULL, CVAR_USERINFO | CVAR_ARCHIVE},
	{ &cg_cameraMode, "com_cameraMode", "0", NULL, CVAR_CHEAT},

	{ &pmove_fixed, "pmove_fixed", "0", NULL, 0},
	{ &pmove_msec, "pmove_msec", "8", NULL, 0},
	{ &cg_noTaunt, "cg_noTaunt", "0", NULL, CVAR_ARCHIVE},
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", NULL, CVAR_ARCHIVE},
	{ &cg_oldRail, "cg_oldRail", "1", NULL, CVAR_ARCHIVE},
	{ &cg_oldRocket, "cg_oldRocket", "1", NULL, CVAR_ARCHIVE},
	{ &cg_oldPlasma, "cg_oldPlasma", "1", NULL, CVAR_ARCHIVE},
	{ &cg_trueLightning, "cg_trueLightning", "0.0", NULL, CVAR_ARCHIVE},
	//New MME ones
	{ &cg_drawDamageBlob,	"cg_drawDamageBlob",	"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawPowerups,		"cg_drawPowerups",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawPickupItem,	"cg_drawPickupItem",	"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawVote,			"cg_drawVote",			"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawFollow,		"cg_drawFollow",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawScores,		"cg_drawScores",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawWeaponSelect,	"cg_drawWeaponSelect",	"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_centerScale,		"cg_centerScale",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_centerPrint,		"cg_centerPrint",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_gibDirectional,	"cg_gibDirectional",	"1",			NULL,	CVAR_ARCHIVE	},
	{ &cg_muzzleFlash,		"cg_muzzleFlash",		"1",			NULL,	CVAR_ARCHIVE	},
	
	{ &cg_commandSmooth,	"cg_commandSmooth",		"2",			NULL,	CVAR_ARCHIVE	},
	
	{ &cg_drawSpeedometer,	"cg_drawSpeedometer",	"0",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawSpeedometerScale,"cg_drawSpeedometerScale","1.0",		NULL,	CVAR_ARCHIVE	},
	{ &cg_drawSpeedometerPos, "cg_drawSpeedometerPos", "640.0 300.0", CG_SetSpedometerPos, CVAR_ARCHIVE	},
	{ &cg_drawSpeedometerFormat,"cg_drawSpeedometerFormat", "Speed: %tups", NULL, CVAR_ARCHIVE	},
	{ &cg_drawSpeedometerAlignment,"cg_drawSpeedometerAlignment","right",	NULL, CVAR_ARCHIVE },
	
	{ &cg_drawMovementKeys,	"cg_drawMovementKeys",	"0",			NULL,	CVAR_ARCHIVE	},
	{ &cg_drawMovementKeysPos, "cg_drawMovementKeysPos", "320.0 140.0",	CG_SetMovementKeysPos, CVAR_ARCHIVE	},
	{ &cg_drawMovementKeysScale, "cg_drawMovementKeysScale", "1.0",	NULL,	CVAR_ARCHIVE	},

	{ &mov_Obituaries,		"mov_Obituaries",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &mov_chatBeep,		"mov_chatBeep",			"1",			NULL,	CVAR_ARCHIVE	},
	{ &mov_fragFormat, "mov_fragFormat", "You fragged %t%n%p place with %s", NULL, CVAR_ARCHIVE},
	{ &mov_fragOnly,		"mov_fragOnly",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_gameType,		"mov_gameType",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_debug,			"mov_debug",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_filterMask,		"mov_filterMask",		"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_stencilMask,		"mov_stencilMask",		"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_seekInterval,	"mov_seekInterval",		"4",			NULL,	CVAR_ARCHIVE	},
	{ &mov_deltaYaw,		"mov_deltaYaw",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_deltaPitch,		"mov_deltaPitch",		"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_deltaRoll,		"mov_deltaRoll",		"0",			NULL,	CVAR_ARCHIVE	},
	
	{ &mov_ratioFix,		"mov_ratioFix",			"1",   CG_Set2DRatio,	CVAR_ARCHIVE	},
	{ &mov_rewardCount,		"mov_rewardCount",		"10",			NULL,	CVAR_ARCHIVE	},
	{ &mov_bobScale,		"mov_bobScale",			"1.0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_wallhack,		"mov_wallhack",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_smoothCamPos,	"mov_smoothCamPos",		"0",			NULL,	CVAR_ARCHIVE	},
	
	{ &mov_hitSounds,		"mov_hitSounds",		"1",			NULL,	CVAR_ARCHIVE	},
	{ &mov_chatBox,			"mov_chatBox",			"10000",		NULL,	CVAR_ARCHIVE	},
	{ &mov_chatBoxHeight,	"mov_chatBoxHeight",	"350",			NULL,	CVAR_ARCHIVE	},
	{ &mov_chatBoxScale,	"mov_chatBoxScale",		"1.0",			NULL,	CVAR_ARCHIVE	},

	/* Copy over some cvar's from the renderer */
//	{ &mme_blurFrames,		"mme_blurFrames",		"",				NULL,	0				},
//	{ &mme_blurOverlap,		"mme_blurOverlap",		"",				NULL,	0				},
	{ &mme_demoFileName,	"mme_demoFileName",		"",				NULL,	0				},

	{ &mov_captureCamera,	"mov_captureCamera",	"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_captureName,		"mov_captureName",		"",				NULL,	CVAR_TEMP		},
	{ &mov_captureFPS,		"mov_captureFPS",		"25",			NULL,	CVAR_ARCHIVE	},
	{ &mov_musicFile,		"mov_musicFile",		"",				NULL,	CVAR_TEMP		},
	{ &mov_musicStart,		"mov_musicStart",		"0",			NULL,	CVAR_TEMP		},
	{ &mov_chaseRange,		"mov_chaseRange",		"20",			NULL,	CVAR_ARCHIVE	},
	{ &mov_fontName,		"mov_fontName",			"",				NULL,	CVAR_ARCHIVE	},
	{ &mov_fontSize,		"mov_fontSize",			"20",			NULL,	CVAR_ARCHIVE	},
	{ &mov_teamSkins,		"mov_teamSkins",		"1",   CG_SetNewSkin,	CVAR_ARCHIVE	},
	{ &mov_gridOffset,		"mov_gridOffset",		"0 0 0",		NULL,	CVAR_ARCHIVE	},
	{ &mov_gridWidth,		"mov_gridWidth",		"0.2",			NULL,	CVAR_ARCHIVE	},
	{ &mov_gridStep,		"mov_gridStep",			"0",			NULL,	CVAR_ARCHIVE	},
	{ &mov_gridRange,		"mov_gridRange",		"500",			NULL,	CVAR_ARCHIVE	},
	{ &mov_gridColor,		"mov_gridColor",		"x222222",		NULL,	CVAR_ARCHIVE	},
	{ &mov_hudOverlay,		"mov_hudOverlay",		"",				NULL,	CVAR_ARCHIVE	},
};

static int  cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	char		var[MAX_TOKEN_CHARS];

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer( "sv_running", var, sizeof( var ) );
	cgs.localServer = atoi( var );

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE );
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap_Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount > modCount ) {
				if ( cv->update )
					cv->update();
			}
		}
	}
}

float CG_Cvar_Get(const char *cvar) {
	char buff[128];
	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

int CG_CrosshairPlayer( void ) {
	if ( cg.time > ( cg.crosshairClientTime + 1000 ) ) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker( void ) {
	if ( !cg.attackerTime ) {
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[BIG_INFO_STRING];

	va_start (argptr, msg);
	vsnprintf (text, sizeof( text ), msg, argptr);
	va_end (argptr);

	trap_Print( text );
}

void QDECL CG_Error( const char *msg, ... ) {
	va_list		argptr;
	char		text[BIG_INFO_STRING];

	va_start (argptr, msg);
	vsnprintf (text, sizeof( text ), msg, argptr);
	va_end (argptr);

	trap_Error( text );
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error( int level, const char *error, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	CG_Error( "%s", text);
}

void QDECL Com_Printf( const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	CG_Printf ("%s", text);
}

#endif

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg ) {
	static char	buffer[MAX_STRING_CHARS];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

gameType_t CG_GameType( void ) {
	if ( mov_gameType.integer ) {
		return mov_gameType.integer - 1;
	}
	switch ( cgs.gametype ) {
	case GT_CTF:
		return gameTypeCTF;
	case GT_TEAM:
		return gameTypeTDM;
	case GT_FFA:
		return gameTypeFFA;
	}
	return gameTypeUnknown;
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds( int itemNum ) {
	gitem_t			*item;
	char			data[MAX_QPATH];
	char			*s, *start;
	int				len;

	item = &bg_itemlist[ itemNum ];

	if( item->pickup_sound ) {
		trap_S_RegisterSound( item->pickup_sound, qfalse );
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s) {
		start = s;
		while (*s && *s != ' ') {
			s++;
		}

		len = s-start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error( "PrecacheItem: %s has bad precache string", 
				item->classname);
			return;
		}
		memcpy (data, start, len);
		data[len] = 0;
		if ( *s ) {
			s++;
		}

		if ( !strcmp(data+len-3, "wav" )) {
			trap_S_RegisterSound( data, qfalse );
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds( void ) {
	int		i;
	char	items[MAX_ITEMS+1];
	char	name[MAX_QPATH];
	const char	*soundName;

	// voice commands
	cgs.media.oneMinuteSound = trap_S_RegisterSound( "sound/feedback/1_minute.wav", qtrue );
	cgs.media.fiveMinuteSound = trap_S_RegisterSound( "sound/feedback/5_minute.wav", qtrue );
	cgs.media.suddenDeathSound = trap_S_RegisterSound( "sound/feedback/sudden_death.wav", qtrue );
	cgs.media.oneFragSound = trap_S_RegisterSound( "sound/feedback/1_frag.wav", qtrue );
	cgs.media.twoFragSound = trap_S_RegisterSound( "sound/feedback/2_frags.wav", qtrue );
	cgs.media.threeFragSound = trap_S_RegisterSound( "sound/feedback/3_frags.wav", qtrue );
	cgs.media.count3Sound = trap_S_RegisterSound( "sound/feedback/three.wav", qtrue );
	cgs.media.count2Sound = trap_S_RegisterSound( "sound/feedback/two.wav", qtrue );
	cgs.media.count1Sound = trap_S_RegisterSound( "sound/feedback/one.wav", qtrue );
	cgs.media.countFightSound = trap_S_RegisterSound( "sound/feedback/fight.wav", qtrue );
	cgs.media.countPrepareSound = trap_S_RegisterSound( "sound/feedback/prepare.wav", qtrue );

	cgs.media.captureAwardSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
	cgs.media.redLeadsSound = trap_S_RegisterSound( "sound/feedback/redleads.wav", qtrue );
	cgs.media.blueLeadsSound = trap_S_RegisterSound( "sound/feedback/blueleads.wav", qtrue );
	cgs.media.teamsTiedSound = trap_S_RegisterSound( "sound/feedback/teamstied.wav", qtrue );
	cgs.media.hitTeamSound = trap_S_RegisterSound( "sound/feedback/hit_teammate.wav", qtrue );

	cgs.media.redScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_red_scores.wav", qtrue );
	cgs.media.blueScoredSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_scores.wav", qtrue );

	cgs.media.captureYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_yourteam.wav", qtrue );
	cgs.media.captureOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagcapture_opponent.wav", qtrue );

	cgs.media.returnYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_yourteam.wav", qtrue );
	cgs.media.returnOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );

	cgs.media.takenYourTeamSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_yourteam.wav", qtrue );
	cgs.media.takenOpponentSound = trap_S_RegisterSound( "sound/teamplay/flagtaken_opponent.wav", qtrue );

	cgs.media.redFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_red_returned.wav", qtrue );
	cgs.media.blueFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/voc_blue_returned.wav", qtrue );
	cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_flag.wav", qtrue );
	cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_flag.wav", qtrue );

	cgs.media.youHaveFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_you_flag.wav", qtrue );
	cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
	cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound( "sound/teamplay/flagreturn_opponent.wav", qtrue );
	cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_team_1flag.wav", qtrue );
	cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound( "sound/teamplay/voc_enemy_1flag.wav", qtrue );

	cgs.media.tracerSound = trap_S_RegisterSound( "sound/weapons/machinegun/buletby1.wav", qfalse );
	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change.wav", qfalse );
	cgs.media.wearOffSound = trap_S_RegisterSound( "sound/items/wearoff.wav", qfalse );
	cgs.media.useNothingSound = trap_S_RegisterSound( "sound/items/use_nothing.wav", qfalse );
	cgs.media.gibSound = trap_S_RegisterSound( "sound/player/gibsplt1.wav", qfalse );
	cgs.media.gibBounce1Sound = trap_S_RegisterSound( "sound/player/gibimp1.wav", qfalse );
	cgs.media.gibBounce2Sound = trap_S_RegisterSound( "sound/player/gibimp2.wav", qfalse );
	cgs.media.gibBounce3Sound = trap_S_RegisterSound( "sound/player/gibimp3.wav", qfalse );

	cgs.media.teleInSound = trap_S_RegisterSound( "sound/world/telein.wav", qfalse );
	cgs.media.teleOutSound = trap_S_RegisterSound( "sound/world/teleout.wav", qfalse );
	cgs.media.respawnSound = trap_S_RegisterSound( "sound/items/respawn1.wav", qfalse );

	cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav", qfalse );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/player/talk.wav", qfalse );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1.wav", qfalse);

	cgs.media.hitSound = trap_S_RegisterSound( "sound/feedback/hit.wav", qfalse );

	cgs.media.impressiveSound = trap_S_RegisterSound( "sound/feedback/impressive.wav", qtrue );
	cgs.media.excellentSound = trap_S_RegisterSound( "sound/feedback/excellent.wav", qtrue );
	cgs.media.deniedSound = trap_S_RegisterSound( "sound/feedback/denied.wav", qtrue );
	cgs.media.humiliationSound = trap_S_RegisterSound( "sound/feedback/humiliation.wav", qtrue );
	cgs.media.assistSound = trap_S_RegisterSound( "sound/feedback/assist.wav", qtrue );
	cgs.media.defendSound = trap_S_RegisterSound( "sound/feedback/defense.wav", qtrue );

	cgs.media.takenLeadSound = trap_S_RegisterSound( "sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound( "sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound( "sound/feedback/lostlead.wav", qtrue);

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound ("sound/world/jumppad.wav", qfalse );

	for (i=0 ; i<4 ; i++) {
		Com_sprintf (name, sizeof(name), "sound/player/footsteps/step%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/boot%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/mech%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/energy%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/splash%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound (name, qfalse);

		Com_sprintf (name, sizeof(name), "sound/player/footsteps/clank%i.wav", i+1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound (name, qfalse);
	}

	// only register the items that the server says we need
	strcpy( items, CG_ConfigString( CS_ITEMS ) );

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		CG_RegisterItemSounds( i );
	}

	for ( i = 1 ; i < MAX_SOUNDS ; i++ ) {
		soundName = CG_ConfigString( CS_SOUNDS+i );
		if ( !soundName[0] ) {
			break;
		}
		if ( soundName[0] == '*' ) {
			continue;	// custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound( soundName, qfalse );
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound( "sound/items/flight.wav", qfalse );
	cgs.media.medkitSound = trap_S_RegisterSound ("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound ("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound ("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound ("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_railg = trap_S_RegisterSound ("sound/weapons/railgun/railgf1a.wav", qfalse);
	cgs.media.sfx_rockexp = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound ("sound/weapons/plasma/plasmx1a.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse );
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);
	
	cgs.media.cpmaHitSound25 = trap_S_RegisterSound( "sound/feedback/hit25.wav", qfalse );
	cgs.media.cpmaHitSound50 = trap_S_RegisterSound( "sound/feedback/hit50.wav", qfalse );
	cgs.media.cpmaHitSound75 = trap_S_RegisterSound( "sound/feedback/hit75.wav", qfalse );
	cgs.media.cpmaHitSound100 = trap_S_RegisterSound( "sound/feedback/hit100.wav", qfalse );

}


//===================================================================================


/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics( void ) {
	int			i;
	char		items[MAX_ITEMS+1];
	static char		*sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_LoadingString( cgs.mapname );

	trap_R_LoadWorldMap( cgs.mapname );

	// precache status bar pics
	CG_LoadingString( "game media" );

	for ( i=0 ; i<11 ; i++) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader( sb_nums[i] );
	}

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader( "menu/art/skill1.tga" );
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader( "menu/art/skill2.tga" );
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader( "menu/art/skill3.tga" );
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader( "menu/art/skill4.tga" );
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader( "menu/art/skill5.tga" );

	cgs.media.viewBloodShader = trap_R_RegisterShader( "viewBloodBlend" );

	cgs.media.deferShader = trap_R_RegisterShaderNoMip( "gfx/2d/defer.tga" );

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip( "menu/tab/name.tga" );
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip( "menu/tab/ping.tga" );
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip( "menu/tab/score.tga" );
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip( "menu/tab/time.tga" );

	cgs.media.smokePuffShader = trap_R_RegisterShader( "smokePuff" );
	cgs.media.smokePuffRageProShader = trap_R_RegisterShader( "smokePuffRagePro" );
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader( "shotgunSmokePuff" );

	cgs.media.plasmaBallShader = trap_R_RegisterShader( "sprites/plasma1" );
	cgs.media.bloodTrailShader = trap_R_RegisterShader( "bloodTrail" );
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer" );
	cgs.media.connectionShader = trap_R_RegisterShader( "disconnected" );

	cgs.media.waterBubbleShader = trap_R_RegisterShader( "waterBubble" );

	cgs.media.tracerShader = trap_R_RegisterShader( "gfx/misc/tracer" );
	cgs.media.selectShader = trap_R_RegisterShader( "gfx/2d/select" );

	for ( i = 0 ; i < NUM_CROSSHAIRS ; i++ ) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader( va("gfx/2d/crosshair%c", 'a'+i) );
	}

	cgs.media.backTileShader = trap_R_RegisterShader( "gfx/2d/backtile" );
	cgs.media.noammoShader = trap_R_RegisterShader( "icons/noammo" );

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad" );
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon" );
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit" );
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon" );
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility" );
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen" );
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff" );

	cgs.media.redCubeModel = trap_R_RegisterModel( "models/powerups/orb/r_orb.md3" );
	cgs.media.blueCubeModel = trap_R_RegisterModel( "models/powerups/orb/b_orb.md3" );
	cgs.media.redCubeIcon = trap_R_RegisterShader( "icons/skull_red" );
	cgs.media.blueCubeIcon = trap_R_RegisterShader( "icons/skull_blue" );
	cgs.media.redFlagModel = trap_R_RegisterModel( "models/flags/r_flag.md3" );
	cgs.media.blueFlagModel = trap_R_RegisterModel( "models/flags/b_flag.md3" );
	cgs.media.redFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_red1" );
	cgs.media.redFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_red2" );
	cgs.media.redFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_red3" );
	cgs.media.blueFlagShader[0] = trap_R_RegisterShaderNoMip( "icons/iconf_blu1" );
	cgs.media.blueFlagShader[1] = trap_R_RegisterShaderNoMip( "icons/iconf_blu2" );
	cgs.media.blueFlagShader[2] = trap_R_RegisterShaderNoMip( "icons/iconf_blu3" );

	cgs.media.friendShader = trap_R_RegisterShader( "sprites/foe" );
	cgs.media.redQuadShader = trap_R_RegisterShader("powerups/blueflag" );
	cgs.media.teamStatusBar = trap_R_RegisterShader( "gfx/2d/colorbar.tga" );
	cgs.media.armorModel = trap_R_RegisterModel( "models/powerups/armor/armor_yel.md3" );
	cgs.media.armorIcon  = trap_R_RegisterShaderNoMip( "icons/iconr_yellow" );

	cgs.media.machinegunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/m_shell.md3" );
	cgs.media.shotgunBrassModel = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.gibAbdomen = trap_R_RegisterModel( "models/gibs/abdomen.md3" );
	cgs.media.gibArm = trap_R_RegisterModel( "models/gibs/arm.md3" );
	cgs.media.gibChest = trap_R_RegisterModel( "models/gibs/chest.md3" );
	cgs.media.gibFist = trap_R_RegisterModel( "models/gibs/fist.md3" );
	cgs.media.gibFoot = trap_R_RegisterModel( "models/gibs/foot.md3" );
	cgs.media.gibForearm = trap_R_RegisterModel( "models/gibs/forearm.md3" );
	cgs.media.gibIntestine = trap_R_RegisterModel( "models/gibs/intestine.md3" );
	cgs.media.gibLeg = trap_R_RegisterModel( "models/gibs/leg.md3" );
	cgs.media.gibSkull = trap_R_RegisterModel( "models/gibs/skull.md3" );
	cgs.media.gibBrain = trap_R_RegisterModel( "models/gibs/brain.md3" );

	cgs.media.smoke2 = trap_R_RegisterModel( "models/weapons2/shells/s_shell.md3" );

	cgs.media.balloonShader = trap_R_RegisterShader( "sprites/balloon3" );

	cgs.media.bloodExplosionShader = trap_R_RegisterShader( "bloodExplosion" );

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel( "models/misc/telep.md3" );
	cgs.media.teleportEffectShader = trap_R_RegisterShader( "teleportEffect" );
	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel( "models/powerups/shield/shield.md3" );
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip( "medal_impressive" );
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip( "medal_excellent" );
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip( "medal_gauntlet" );
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip( "medal_defend" );
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip( "medal_assist" );
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip( "medal_capture" );

	memset( cg_items, 0, sizeof( cg_items ) );
	memset( cg_weapons, 0, sizeof( cg_weapons ) );

	// only register the items that the server says we need
	strcpy( items, CG_ConfigString( CS_ITEMS) );

	for ( i = 1 ; i < bg_numItems ; i++ ) {
		if ( /*items[ i ] == '1' || */ cg_buildScript.integer ) {
			CG_LoadingItem( i );
			CG_RegisterItemVisuals( i );
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader( "gfx/damage/bullet_mrk" );
	cgs.media.burnMarkShader = trap_R_RegisterShader( "gfx/damage/burn_med_mrk" );
	cgs.media.holeMarkShader = trap_R_RegisterShader( "gfx/damage/hole_lg_mrk" );
	cgs.media.energyMarkShader = trap_R_RegisterShader( "gfx/damage/plasma_mrk" );
	cgs.media.shadowMarkShader = trap_R_RegisterShader( "markShadow" );
	cgs.media.wakeMarkShader = trap_R_RegisterShader( "wake" );
	cgs.media.bloodMarkShader = trap_R_RegisterShader( "bloodMark" );

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for ( i = 1 ; i < cgs.numInlineModels ; i++ ) {
		char	name[10];
		vec3_t			mins, maxs;
		int				j;

		Com_sprintf( name, sizeof(name), "*%i", i );
		cgs.inlineDrawModel[i] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[i], mins, maxs );
		for ( j = 0 ; j < 3 ; j++ ) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * ( maxs[j] - mins[j] );
		}
	}

	// register all the server specified models
	for (i=1 ; i<MAX_MODELS ; i++) {
		const char		*modelName;

		modelName = CG_ConfigString( CS_MODELS+i );
		if ( !modelName[0] ) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel( modelName );
	}

	CG_ClearParticles ();
	//MME additions
	
	cgs.media.df_unavailableItemShader = trap_R_RegisterShader( "mme_df_unavailableItem" );

}



/*																																			
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString(void) {
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR ) {
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen) {
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*																																			
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients( void ) {
	int		i;

//	CG_LoadingClient(cg.clientNum);
//	CG_NewClientInfo(cg.clientNum);

	for (i=0 ; i<MAX_CLIENTS ; i++) {
		const char		*clientInfo;

//		if (cg.clientNum == i) {
//			continue;
//		}

		clientInfo = CG_ConfigString( CS_PLAYERS+i );
		if ( !clientInfo[0]) {
			continue;
		}
		CG_LoadingClient( i );
		CG_NewClientInfo( i );
	}
	CG_BuildSpectatorString();
}

static void CG_RegisterWeapons( void ) {
	int i;

	for ( i = 0; i < WP_NUM_WEAPONS;i++ )
		CG_RegisterWeapon( i );

}

/*																																			
===================
CG_RegisterClients
===================
*/

static void CG_RegisterWeaponFX( cgWeaponFX_t *fx, const char *name ) {
	fx->flash = trap_FX_Register( va ( "weapon/%s/flash", name ) );
	fx->fire = trap_FX_Register( va ( "weapon/%s/fire", name ) );
	fx->impact = trap_FX_Register( va ( "weapon/%s/impact", name ) );
	fx->projectile = trap_FX_Register( va ( "weapon/%s/projectile", name ) );
	fx->trail = trap_FX_Register( va ( "weapon/%s/trail", name ) );
}

static void CG_RegisterFX( void ) {
	CG_RegisterWeaponFX( &cgs.fx.rocket, "rocket" );
	CG_RegisterWeaponFX( &cgs.fx.rail, "rail" );
	CG_RegisterWeaponFX( &cgs.fx.grenade, "grenade" );
	CG_RegisterWeaponFX( &cgs.fx.machinegun, "machinegun" );
	CG_RegisterWeaponFX( &cgs.fx.shotgun, "shotgun" );
	CG_RegisterWeaponFX( &cgs.fx.plasma, "plasma" );
	CG_RegisterWeaponFX( &cgs.fx.bfg, "bfg" );
	CG_RegisterWeaponFX( &cgs.fx.gauntlet, "gauntlet" );
	CG_RegisterWeaponFX( &cgs.fx.lightning, "lightning" );
	CG_RegisterWeaponFX( &cgs.fx.grapple, "grapple" );
	
	cgs.fx.bubbles = trap_FX_Register( "weapon/common/bubbles" );

	cgs.fx.player.gibbed = trap_FX_Register( "player/gibbed" );
	cgs.fx.player.teleportIn = trap_FX_Register( "player/teleportIn" );
	cgs.fx.player.teleportOut = trap_FX_Register( "player/teleportOut" );

	cgs.fx.player.quad = trap_FX_Register( "player/quad" );
	cgs.fx.player.haste = trap_FX_Register( "player/haste" );
	cgs.fx.player.talk = trap_FX_Register( "player/talk" );
	cgs.fx.player.connection = trap_FX_Register( "player/connection" );
	cgs.fx.player.impressive = trap_FX_Register( "player/impressive" );
	cgs.fx.player.excellent = trap_FX_Register( "player/excellent" );
	cgs.fx.player.gauntlet = trap_FX_Register( "player/gauntlet" );
	cgs.fx.player.capture = trap_FX_Register( "player/capture" );
	cgs.fx.player.assist = trap_FX_Register( "player/assist" );
	cgs.fx.player.friend = trap_FX_Register( "player/friend" );

}


//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index ) {
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		CG_Error( "CG_ConfigString: bad index: %i", index );
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic( void ) {
	char	*s;
	char	parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
extern void CG_SetConfigValues( void );
void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum ) {
	const char	*s;

	// clear everything
	memset( &cgs, 0, sizeof( cgs ) );
	memset( &cg, 0, sizeof( cg ) );
	memset( cg_entities, 0, sizeof(cg_entities) );
	memset( cg_weapons, 0, sizeof(cg_weapons) );
	memset( cg_items, 0, sizeof(cg_items) );

	cg.clientNum = clientNum;
	cg.loading = qtrue;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader		= trap_R_RegisterShader( "gfx/2d/bigchars" );
	cgs.media.whiteShader		= trap_R_RegisterShader( "white" );
	cgs.media.charsetProp		= trap_R_RegisterShaderNoMip( "menu/art/font1_prop.tga" );
	cgs.media.charsetPropGlow	= trap_R_RegisterShaderNoMip( "menu/art/font1_prop_glo.tga" );
	cgs.media.charsetPropB		= trap_R_RegisterShaderNoMip( "menu/art/font2_prop.tga" );

	CG_RegisterCvars();
	//Load the fonts
	cgs.textFont.glyphScale = 0;
	trap_R_RegisterFont( mov_fontName.string, mov_fontSize.integer, &cgs.textFont );
	cgs.textFontValid = cgs.textFont.glyphScale > 0;

	CG_InitConsoleCommands();

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig( &cgs.glconfig );
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// get the gamestate from the client system
	trap_GetGameState( &cgs.gameState );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );

	if (strstr( s, "cpma") ) {
		cg.cpma.detected = qtrue;
	} else if (strstr( s, "defrag") ) {
		CG_Printf( "Defrag demo detected\n" );
		cg.defrag.detected = qtrue;
	}
	//RA3 seems to use 12 as a version whatever index
	s = CG_ConfigString( 12 );
	if (strstr( s, "RA3") ) {
		CG_Printf( "Rocket Arena demo detected\n" );
		cg.arena.detected = qtrue;
	}

	/* Check for OSP value for encryption */
	s = CG_ConfigString ( 872 );
	if ( s[0] ) {
		CG_Printf( "Encrypted OSP player data used.\n" );
        cg.osp.playerEncryption = atoi(s) & 1;
		cg.osp.detected = qtrue;
	} else {
		cg.osp.playerEncryption = 0;
	}

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_ParseServerinfo();

	if ( cg.defrag.detected ) {
		CG_Printf( "Defrag demo detected\n" );
	} else if ( cg.osp.detected ) {
		CG_Printf( "OSP demo detected\n" );
	} else if ( cg.cpma.detected ) {
		CG_Printf( "CPMA demo detected\n" );
	} else {
		CG_Printf( "Regular/Unknown demo detected\n" );
	}

	// load the new map
	CG_LoadingString( "collision map" );

	trap_CM_LoadMap( cgs.mapname );

	BG_InitItems( bgItemCPMA );
//		BG_InitItems( bgItemBase );

	CG_LoadingString( "sounds" );

	CG_RegisterSounds();

	CG_LoadingString( "graphics" );

	CG_RegisterGraphics();

	CG_LoadingString( "clients" );

	CG_RegisterClients();		// if low on memory, some clients will be deferred

	CG_LoadingString( "weapons" );

	CG_RegisterWeapons();		// if low on memory, some clients will be deferred

	CG_LoadingString( "FX" );

	CG_RegisterFX();		// if low on memory, some clients will be deferred


	CG_InitLocalEntities();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString( "" );

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds( qtrue );
	cg.loading = qfalse;

	CG_Set2DRatio();
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown( void ) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int type) {
}




