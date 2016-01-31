// cg_demos_cut.c -- cutting demos commands

#include "cg_demos.h" 

char *QDECL timeConvert(int time) {
	if (time >= 0) {
		static char t[32];
		int msec = time % 1000;
		int secs = (time / 1000);
		int mins = (secs / 60);
		if (time >= 60000) {
			secs %= 60;
			Com_sprintf(t, sizeof(t), "%d:%02d.%03d", mins, secs, msec);
		} else {
			Com_sprintf(t, sizeof(t), "%d.%03d", secs, msec);
		}
		return t;
	}
	return "";
}

void demoCutCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "it")) {
		int start, end;
		start = demo.cut.start;
		end = demo.cut.end;
		if (end <= start) {
			CG_DemosAddLog("Invalid cut range: %s >= %s", va("%s", timeConvert(start)), va("%s", timeConvert(end)));
		} else if (demo.cut.end > demo.cut.start + 50) {
			CG_DemosAddLog("Cutting demo in the range %s - %s", va("%s", timeConvert(start)), va("%s", timeConvert(end)));
			trap_SendConsoleCommand(va("demoCut %d.%03d %d.%03d", start / 1000, start % 1000, end / 1000, end % 1000));
		} else {
			CG_DemosAddLog("Invalid cut range: <= 50 msecs");
		}
	} else if (!Q_stricmp(cmd, "start")) { 
		demo.cut.start = demo.play.time;
		CG_DemosAddLog("Cut start at %s", timeConvert(demo.cut.start));
	} else if (!Q_stricmp(cmd, "end")) { 
		demo.cut.end = demo.play.time;
		CG_DemosAddLog("Cut end at %s", timeConvert(demo.cut.end));
	} else {
		Com_Printf("cut usage:\n" );
		Com_Printf("cut it, cut the currently playing demo with the set range.\n" );
		Com_Printf("cut start/end, current time will be set as start/end of cutting.\n" );
	}
}