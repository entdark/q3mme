#include "bg_demos.h"
#include "g_local.h"

void demoEvaluateTrajectory( const trajectory_t *tr, int atTime, float atTimeFraction, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001 + atTimeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) % tr->trDuration;
		deltaTime = ( deltaTime + atTimeFraction ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
			atTimeFraction = 0;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001 + atTimeFraction * 0.001;
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001 + atTimeFraction * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}


void demoCommandValue( const char *cmd, float * oldVal ) {
	if (!cmd[0])
		return;
	if (cmd[0] == 'a') {
		*oldVal += atof(cmd + 1);
	} else {
		*oldVal = atof(cmd);
	}
}

qboolean BG_XMLError(BG_XMLParse_t *parse, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);
	
	Com_Printf( "XML Error %s in line %d\n", text, parse->line);
	trap_FS_FCloseFile( parse->fileHandle );
	return qfalse;
}

void BG_XMLWarning(BG_XMLParse_t *parse, const char *msg, ... ) {
	va_list		argptr;
	char		text[1024];
	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);
	
	Com_Printf( "XML Warn %s in line %d\n", text, parse->line);
}


qboolean BG_XMLOpen( BG_XMLParse_t *parse, const char *fileName ) {
	if (!parse)
		return qfalse;
	parse->fileSize = trap_FS_FOpenFile( fileName, &parse->fileHandle, FS_READ);
	if ( parse->fileHandle <= 0 || !parse->fileSize )
		return qfalse;
	parse->filePos = 0;
	parse->line = 1;
	return qtrue;
}



qboolean BG_XMLParse( BG_XMLParse_t *parse, const BG_XMLParseBlock_t *fromBlock, const BG_XMLParseBlock_t *parseBlock, void *data ) {
	char tagName[128];
	char line[1024];
	char c;
	int  index;
	qboolean readTag, closeTag;
	const BG_XMLParseBlock_t *blockScan;

	readTag = qfalse;
	parse->depth++;
	index = 0;
	while ( parse->filePos < parse->fileSize ) {
		parse->filePos++;
		trap_FS_Read( &c, 1, parse->fileHandle );
		if ( readTag ) {
			if (c == '/') {
				if (!index) 
					closeTag = qtrue;
				else 
					return BG_XMLError(parse, "/ in tag");
			} else if (c == '>') {
				if (!index) 
					return BG_XMLError(parse, "empty tag");
				tagName[index] = 0;
				if ( closeTag ) {
					if (!fromBlock || Q_stricmp( tagName, fromBlock->tagName))
						return BG_XMLError(parse, "Unmatched close tag %s", tagName );
					return qtrue;
				}
				index = 0;
				readTag = qfalse;
				blockScan = parseBlock;
				while (blockScan) {
					if (!blockScan->tagName) {
						 blockScan = 0;
						 break;
					}
					if (!Q_stricmp(blockScan->tagName, tagName)) {
						if (blockScan->openHandler) {
							if (!blockScan->openHandler( parse, blockScan, data))
								return qfalse;
						} else {
							if (!BG_XMLParse( parse, blockScan, 0, data)) 
								return qfalse;
						}
						break;
					}
					blockScan++;
				}
				if (!blockScan) {
					BG_XMLParseBlock_t block;
					/* Handle unrecognized blocks with a temp handler */
					block.tagName = tagName;
					block.openHandler = 0;
					block.textHandler = 0;
//					Com_Printf("XML unrecognized tag %s, line %d\n", tagName, parse->line );
					if (!BG_XMLParse( parse, &block, 0, 0)) 
						return qfalse;
				}
			} else if (c == ' ' || c == '\t' || c =='\r') {
			} else if (c =='\n' ) {
				parse->line++;
			} else {
				if (index >= sizeof( tagName)-1) 
					return BG_XMLError(parse, "tag too long" );
				tagName[index++] = c;
			}
		} else {
			if (c == '<') {
				line[index] = 0;
				if (fromBlock && fromBlock->textHandler) {
					if ( !index ) 
						BG_XMLWarning( parse, "Empty line for tag %s", fromBlock->tagName );
					else if (!fromBlock->textHandler( parse, line, data ))
						return qfalse;
				}
				readTag = qtrue;
				closeTag = qfalse;
				index = 0;
			} else if (c) {
				if (c == '\n')
					parse->line++;
				if (index >= sizeof(line)-1)
					return BG_XMLError(parse, "line too long" );
				line[index++] = c;
			}
		}
		continue;
	}
	if (!fromBlock) {
		trap_FS_FCloseFile( parse->fileHandle );
	}
	return qtrue;
};

