/*
===========================================================================
Copyright (C) 2006 Sjoerd van der Berg

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
// alias.c -- Quake alias script handling
// Todo setup some system so it calls the vm's for alias tags

#include "../game/q_shared.h"
#include "qcommon.h"
#include "math.h"

typedef struct aliasEntry_s {
	struct	aliasEntry_s	*next;
	char	**cmdList;
	int		cmdCount;
	char	name[0];	
} aliasEntry_t;

#define ALIAS_LINE_SIZE 1024
#define ALIAS_HASH_SHIFT 7
#define ALIAS_HASH_SIZE ( 1 << ALIAS_HASH_SHIFT )
#define ALIAS_HASH_MASK ( ALIAS_HASH_SIZE - 1 )
#define ALIAS_ARGS	32

static aliasEntry_t *aliasHash[ALIAS_HASH_SIZE];
typedef struct {
	int				count;
	const char		*start[ALIAS_ARGS];
	char			buf[ ALIAS_LINE_SIZE ];
	char			line[ ALIAS_LINE_SIZE ];
} aliasArgs_t;

typedef struct {
	int					tagDepth;
	const aliasArgs_t	*args;
	const char			*read;
	const char			*input;
} aliasParse_t;

typedef int (*aliasTagHandler_t) ( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft);

typedef struct{
	const char *tag;
	aliasTagHandler_t handler;	
	const char *help;
} aliasTag_t;


/* A load of forward references */
static qboolean aliasParseArgs( aliasArgs_t *args, const char *input );
static int aliasRun( const char *cmd, const aliasArgs_t *args, char *out, int outSize );
static aliasEntry_t *Alias_Find( const char *name );

static unsigned int cmd_AliasMakeHash( const char *name ) {
	unsigned int hash = 0;
	while (*name) {
		char c = name[0] | 32;
		hash = (hash << 5 ) ^ (hash >> 27) ^ c;
		name++;
	}
	return (hash ^ (hash >> ALIAS_HASH_SHIFT) ^ (hash >> ALIAS_HASH_SHIFT * 2)) & ALIAS_HASH_MASK;    
}

static aliasEntry_t *Alias_Find( const char *name ) {
	unsigned int hashIndex = cmd_AliasMakeHash( name );
	aliasEntry_t *hash = aliasHash[ hashIndex ];
	while (hash) {
		if (!Q_stricmp( hash->name, name))
			return hash;
		hash = hash->next;
	}
	return 0;
}

static void Alias_Add( const char *name, const char *cmd ) {
	unsigned int hashIndex = cmd_AliasMakeHash( name );
	aliasEntry_t *hash = aliasHash[ hashIndex ];
	int nameLen = 1 + strlen( name );
	int cmdLen = 1 + strlen( cmd );
	int cmdCount = 1;
    int cmdParse = 0;
	char *cmdWrite;

	while (hash) {
		/* Might already exist, then replace the cmd */
		if (!Q_stricmp( hash->name, name)) {
			Z_Free( hash->cmdList );
			goto foundOne;
		}
		hash = hash->next;
	}
	/* Allocate a whole new hash set */
	hash = Z_TagMalloc( sizeof( aliasEntry_t ) + nameLen, TAG_SMALL );
	Com_Memcpy( hash->name, name, nameLen );
	hash->next = aliasHash[ hashIndex ];
	aliasHash[ hashIndex ] = hash;
foundOne:
	/* Determine amount of commands this alias contains */
	for (cmdParse = 0;cmdParse < cmdLen; cmdParse++) {
		if ( cmd [cmdParse] == ';')
			cmdCount++;
	}
	hash->cmdCount = cmdCount;
	hash->cmdList = Z_TagMalloc( (sizeof( char *) *cmdCount) + cmdLen, TAG_SMALL );
	cmdWrite = (char *)(hash->cmdList + cmdCount);
	cmdCount = 0;
	hash->cmdList[cmdCount++] = cmdWrite;
	for (cmdParse = 0;cmdParse < cmdLen; cmdParse++) {
		if ( cmd [cmdParse] == ';') {
			*cmdWrite++ = 0;
			hash->cmdList[cmdCount++] = cmdWrite;
		} else {
			*cmdWrite++ = cmd [cmdParse];
		}
	}
}

static qboolean Alias_Remove( const char *name ) {
	unsigned int hashIndex = cmd_AliasMakeHash( name );
	aliasEntry_t **hashAt = &aliasHash[ hashIndex ];
	while (*hashAt) {
		aliasEntry_t *hash = *hashAt;
		/* Might already exist, then replace the cmd */
		if (!Q_stricmp( hash->name, name )) {
			*hashAt = hash->next;		
			Z_Free( hash->cmdList );
			Z_Free( hash );
			return qtrue;
		}
		hashAt = &hash->next;
	}
	return qfalse;
}

/* Simple string copy that returns string length or -1 with string overflow */
static int aliasCopy( char *dest, const char *src, int destSize ) {
	int len = 0;
	while (src[0]) {
		if ( destSize <= 0) {
			Com_Printf("String overflow while copying %s\n", src );
			return -1;
		}
		*dest++ = *src++;
		len++;destSize--;
	}
	return len;
}

static int aliasArgsFrom( const aliasArgs_t *args, int index, char *out, int outLeft ) {
	int i, outIndex;
	if (index < 0 || index >= args->count) {
		Com_Printf("Illegal argument index %d\n", index);
		return -1;
	}
	outIndex = 0;
	outLeft--;
	for (i = index;i < args->count;i++) {
		int ret = aliasCopy( out + outIndex, args->start[i], outLeft - outIndex );
		if ( ret < 0)
			return -1;
		outIndex += ret;
	}
	out[outIndex] = 0;
	return outIndex;
}

static void aliasUnbrace( char *line ) {
	char *write = line;
    
	while (1) {
		char c = *line++;

		if ( c == '{' || c == '}')
			continue;
		*write++ = c;
		if (!c)
			break;
	}
}

static qboolean aliasScanInt( const char *check, int *result ) {
	const char *scan = check;
	int negMul;
	int val;

	if (scan[0] == '-') {
		scan++;
		negMul = -1;
	} else {
		negMul = 1;
	}
	val = 0;
	for (;scan[0];scan++) {
		if (scan[0] < '0' || scan[0] > '9') {
			Com_Printf("Invalid number %d\n", check );
			return qfalse;
		}
		val = val * 10 + scan[0] - '0';
	}
	*result = negMul * val;
	return qtrue;
}

static int tagVersion( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	const char *version = Q3_VERSION;
	return aliasCopy( out, version, outLeft );
}
static int tagCvar( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	const char *val;
	if (args->count != 1) {
		Com_Printf( "$cvar takes 1 argument\n");
		return -1;
	}
	val = Cvar_VariableString( args->start[0] );
	return aliasCopy( out, val, outLeft );
}
static int tagArg( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	int index;
	if (args->count != 1) {
		Com_Printf( "$arg takes 1 argument\n");
		return -1;
	}
	if (!aliasScanInt( args->start[0], &index ))
		return -1;
	if (index < 0 || index >= parse->args->count) {
		Com_Printf( "Illegal $arg index %d\n", index);
		return -1;
	}
	return aliasCopy( out, parse->args->start[index], outLeft );
}


static int tagAlias( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	aliasEntry_t *entry;
	
	if (args->count <= 0) {
		Com_Printf("no arguments for $alias\n" );
		return -1;
	}
	entry = Alias_Find( args->start[0] );
	if (!entry) {
		Com_Printf("unknown alias (%s)\n", args->start[0] );
		return -1;
	}
	if (!entry->cmdCount)
		return 0;
	return aliasRun( entry->cmdList[0], args, out, outLeft );
}

typedef struct {
	int	paraDepth;
	const char *read;
	const char *input;
	qboolean error;
} evalParse_t;

static float evalError( evalParse_t *parse, const char *error, ...) {
	parse->error = qtrue;
	Com_Printf("eval error %s in line %s\n", error, parse->input ) ;
	return 0;
}
static void evalPrepare( evalParse_t *parse, const char *line ) {
	Com_Memset( parse, 0, sizeof( evalParse_t ));
	parse->input = parse->read = line;
}

static float evalParse( evalParse_t *parse, qboolean fullLine ) {
	char buf[32];
	int numSize, wordSize;
	qboolean valDone = qfalse;
	float val;

	buf[0] = 0;
	numSize = wordSize = 0;
	while (1) {
		const char c = parse->read[0];
		if ( parse->error )
			return 0;
		if ( wordSize ) {
			buf[wordSize] = 0;
			if ( !c || c == ')' )
				return evalError( parse, "unexpected end at %s", buf );
			if ( c == '(' || c == ' ' ) {
				/* Easier to just parse the value first incase of trouble */
				parse->read++;
				if (c == '(') {
					parse->paraDepth++;
					val = evalParse( parse, qtrue );
				} else {
					val = evalParse( parse, qfalse );
				}
				if ( parse->error )
					return 0;
				if ( 0 ) { 
				} else if (!Q_stricmp( buf, "sin")) {
					val = sin( val * (M_PI/180.0) );
				} else if (!Q_stricmp( buf, "cos")) {
					val = cos( val* (M_PI/180.0) );
				} else if (!Q_stricmp( buf, "tan")) {
					val = tan( val * (M_PI/180.0) );
				} else if (!Q_stricmp( buf, "asin")) {
					val = asin( val ) * (180.0/M_PI);
				} else if (!Q_stricmp( buf, "acos")) {
					val = atan( val ) * (180.0/M_PI);
				} else if (!Q_stricmp( buf, "atan")) {
					val = atan( val ) * (180.0/M_PI);
				} else if (!Q_stricmp( buf, "sqrt")) {
					if ( val < 0)
						return evalError(parse, "negative sqrt");
					val = sqrt( val );
				} else if (!Q_stricmp( buf, "floor")) {
					val = floor( val );
				} else if (!Q_stricmp( buf, "ceil")) {
					val = ceil( val );
				}
				wordSize = 0;
				valDone = qtrue;
				continue;
			}
			if (wordSize >= sizeof( buf ) - 1) 
				return evalError( parse, "too long word %s", buf);
			buf[wordSize++] = c;
			parse->read++;
			continue;
		}
		/* Start scan for a new valber */
		if ( c == '.' || (c >= '0' && c <= '9') ) {
			if ( valDone )
				return evalError( parse, "unexpected number" );
			if (numSize < sizeof( buf ) - 1)
                buf[numSize++] = c;
			parse->read++;
			continue;
		}
		/* No more numbers parse the read in number and see what comes next */
		if ( !valDone && numSize ) {
			if ( valDone ) {
				parse->error = qtrue;			
				return 0;
			}
			buf[numSize] = 0;
			val = atof( buf );
			valDone = qtrue;
		}
		switch ( c ) {
		case ' ':
			parse->read++;
			continue;
		case '+':
			if (!valDone) {
				parse->read++;
				val = evalParse( parse, qfalse );
				valDone = qtrue;
				continue;
			}
			if ( !fullLine )
				return val;
			parse->read++;
			val += evalParse( parse, qfalse );
			continue;
		case '-':
			if (!valDone) {
				parse->read++;
				val = -evalParse( parse, qfalse );
				valDone = qtrue;
				continue;
			}
			if (!fullLine)
				return val;
			parse->read++;
			val -= evalParse( parse, qfalse );
			continue;
		case '*':
			if (!valDone)
				return evalError( parse, "* without value");
			parse->read++;
			val *= evalParse( parse, qfalse );
			continue;
		case '/':
			if (!valDone)
				return evalError( parse, "/ without value");
			parse->read++;
			val /= evalParse( parse, qfalse );
			continue;
		case '(':
			if (valDone)
				return evalError( parse, "Unexpected (" );
			parse->read++;
			parse->paraDepth++;
			val = evalParse( parse, qtrue );
			valDone = qtrue;
			continue;
		case ')':
			if (parse->paraDepth <= 0)
				return evalError( parse, "Unbalanced parantheses" );
			if (fullLine) {
				parse->read++;
				parse->paraDepth--;
			}
			if (!valDone)
				return evalError( parse, "empty parantheses" );
			return val;
		case 0:
			break;
		default:
			if ( valDone )
				return evalError( parse, "unexpected word" );
			buf[wordSize++] = c;
			parse->read++;
			continue;
		}
		break;
	}
	if (!valDone)
		return 0;
	return val;
}

static int tagEval( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	char line[ALIAS_LINE_SIZE];
	int ret;
	evalParse_t eval;
	float v;

	ret = aliasArgsFrom( args, 0, line, sizeof( line ));
	aliasUnbrace( line );
	if (ret < 0)
		return -1;
	evalPrepare( &eval, line );
	v = evalParse( &eval, qtrue );
	Com_sprintf( out, outLeft, "%f", v );
	return strlen( out );
}

static int tagGettok( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	int index;
	aliasArgs_t tokens;
	if (args->count < 2) {
		Com_Printf("$gettok with too few arguments\n");
		return -1;
	}
	if ( !aliasScanInt( args->start[1], &index))
		return -1;
	if (!aliasParseArgs( &tokens, args->start[0] ))
		return -1;
	if ( index == 0 ) {
		Com_sprintf( out, outLeft, "%d", tokens.count );
		return strlen( out );
	} else if ( index > tokens.count ) {
		Com_Printf("$gettok invalid index %d\n", index);
		return -1;
	}
	return aliasCopy( out, tokens.start[index -1], outLeft );
}

static int tagCycle( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	int index;
	aliasArgs_t tokens;
	if (args->count < 2) {
		Com_Printf("$cycle with too few arguments\n");
		return -1;
	}
	if (!aliasParseArgs( &tokens, args->start[0] ))
		return -1;
	if (!tokens.count) {
		Com_Printf("$cycle with empty list\n");
		return -1;
	}
	for (index = 0; index < tokens.count; index++) {
		if (!strcmp( tokens.start[index], args->start[1])) {
			return aliasCopy( out, tokens.start[(index + 1) % tokens.count], outLeft );
		}
	}
	return aliasCopy( out, tokens.start[0], outLeft );
}

static int tagVector( const aliasParse_t *parse, const aliasArgs_t *args, char *out, int outLeft ) {
	const char *cmd;
	aliasArgs_t t1, t2;
	vec3_t retVal;
	qboolean retVector;

	if (args->count < 2) {
		Com_Printf("Need at least 2 arguments for $vector\n");
		return -1;
	}
	cmd = args->start[0];
	if (!Q_stricmp( cmd, "scale" )) {
		if (args->count < 2) {
			Com_Printf("Need at least 3 arguments for vector scale\n");
			return -1;
		}
	}
	return 0;
}


static aliasTag_t aliasTagTable[] = {
	{ "alias",			tagAlias, "name arguments, $alias(combine first last) " },
	{ "cvar",			tagCvar, "name, $cvar(fs_game) => mme or some other mod"  },
	{ "arg",			tagArg , "index, $arg(2) => Second argument given to alias" },
	{ "eval",			tagEval , "expression, $eval( 2 + 2 ) => 4, check aliasEval" },
	{ "vector",			tagVector, "Vector Math..." },
	{ "gettok",			tagGettok, "{string} index, $gettok({meep blah} 2) => meep" },
	{ "cycle",			tagCycle, "{list} entry, $cycle({0 4 5} 4) => 5" },
	{ "version",		tagVersion, "nothing, $version() => " Q3_VERSION  },
};

static aliasTagHandler_t aliasTagFind( const char *tagName, int tagLen ) {
	int tagCount = sizeof( aliasTagTable ) / sizeof( aliasTag_t );
	aliasTag_t *searchTag = aliasTagTable;
	for ( ; tagCount > 0; tagCount--, searchTag++) {
		if (Q_stricmpn( tagName, searchTag->tag,tagLen ))
			continue;
		if ( searchTag->tag[ tagLen ])
			continue;
		return searchTag->handler;
	}
	return 0;
}

static int aliasParse( aliasParse_t *parse, char *out, int outLeft ) {
	int	outIndex;
	int tagRet;
	/* Find end of the tag */
	char tagInput[ ALIAS_LINE_SIZE ];
	const char *tagStart = 0;
	aliasTagHandler_t handler;
	aliasArgs_t	tagArgs;
	int paraDepth;

	/* Will stop processing this line if paraDepth goes back to 0 */
	paraDepth = 0;

	for (outIndex = 0 ; outIndex < outLeft; parse->read++ ) {
		char c = parse->read[0];

		if ( tagStart ) {
			if ( c == '(') {
				handler = aliasTagFind( tagStart, parse->read - tagStart );
				if (!handler) {
					Com_Printf("tagReplace: couldn't find tag %s in line: %s", tagStart, parse->input );
					return -1;
				}
				parse->tagDepth++;
				tagRet = aliasParse( parse, tagInput, sizeof( tagInput ));
				if (tagRet < 0)
					return tagRet;
				if (!aliasParseArgs( &tagArgs, tagInput ))
					return -1;
				parse->tagDepth--;
			
				if (parse->tagDepth) {
					if (outIndex >= outLeft) {
						Com_Printf( "No room for subtag closing braces" );
						return -1;
					}
					out[outIndex++] = '{';
				}
				tagRet = handler( parse, &tagArgs, out + outIndex, outLeft - outIndex );
				if (tagRet <0)
					return tagRet;
				outIndex += tagRet;
				tagStart = 0;
				if (parse->tagDepth) {
					if (outIndex >= outLeft) {
						Com_Printf( "No room for subtag closing braces" );
						return -1;
					}
					out[outIndex++] = '}';
				}
				continue;
			} else if (c == 0) {
				Com_Printf("tagReplace: unexpected end of line: %s\n", parse->input );
				return -1;
			}
		} else {
			if (!c) {
				break;
			} else if ( c == '$' ) {
				/* Is it a double $$ return a single one */
				if (parse->read[1] == '$') {
					parse->read++;
				} else {
					tagStart = parse->read + 1;
					continue;
				}
			} else if ( parse->tagDepth && c == '(' ) {
                paraDepth++;
				/* Skip the first ( */
				if ( paraDepth == 1)
					continue;
			} else if ( parse->tagDepth && c == ')' ) {
                paraDepth--;
				/* Back to 0 with last ), end */
				if ( paraDepth == 0)
					break;
				else if (paraDepth < 0) {
					Com_Printf("Unbalaced parantheses in line %s\n", parse->input );
					return -1;
				}
			}
			out[outIndex++] = c;
		}
	}
	/* Finalize the return string */
	out[outIndex] = 0;
	return outIndex;
}

static qboolean aliasParseArgs( aliasArgs_t *args, const char *input ) {
	char *output;
	int	 outIndex;
	qboolean haveQuote;
	qboolean haveWord;
	int bracketDepth;

	args->count = 0;
	output = args->buf;
	outIndex = 0;
	haveWord = haveQuote = bracketDepth = 0;
	for ( ;args->count < ALIAS_ARGS; input++ ) {
		/* Skip initial seperators */
		if (outIndex >= sizeof( args->buf )) {
			Com_Printf("aliasParseArgs:too long string\n");
			return qfalse;
		}

		if ( bracketDepth ) {
			if (!input[0]) {
				Com_Printf("ArgParse: End of line with open bracket\n");
				return qfalse;
			}
			if (input[0] == '}')
				bracketDepth--;

			if ( bracketDepth == 0 ) {
				if ((input[1] != 0) && (input[1] != ' ') && (input[1] != ')')) {
					Com_Printf("Closing bracket not followed by seperator\n");
					return qfalse;
				}
				/* Close the arg string */
				output[outIndex++] = 0;
				continue;
			}
		} else if ( haveQuote ) {
			if (!input[0]) {
				Com_Printf("ArgParse: End of line with open quote\n");
				return qfalse;
			}
			if (input[0] == '"') {
				if ((input[1] != 0) && (input[1] != ' ')) {
					Com_Printf("ArgParse:Closing quote not followed by seperator\n");
					return qfalse;
				}
				/* Close the string */
				output[outIndex++] = 0;
				haveQuote = 0;
				continue;
			}
		} else if ( haveWord ) {
			if ((input[0] == 0) || (input[0] == ' ')) {
				output[outIndex++] = 0;
				haveWord = 0;
				if ( !input[0] )
					break;
				continue;
			}
		} else {
			if ( input[0] == '{') {
				bracketDepth++;
				args->start[ args->count++ ] = output + outIndex;
				continue;
			} else if ( input[0] == '"') {
				haveQuote = qtrue;
				args->start[ args->count++ ] = output + outIndex;
				continue;
			} else if ( input[0] == ' ') {
				continue;
			} else if ( input[0] ) {
				haveWord = qtrue;
				args->start[ args->count++ ] = output + outIndex;
			} else {
				break;
			}
		}
		output[outIndex++] = input[0];
	}
	return qtrue;
}


static int aliasRun( const char *cmd, const aliasArgs_t *args, char *out, int outSize ) {
	aliasParse_t parse;

	parse.args = args;
	parse.input = cmd;
	parse.read = cmd;
	parse.tagDepth = 0;
	return aliasParse( &parse, out, outSize );
}

/* Control commands */

void	Alias_CommandCompletion( void(*callback)(const char *s) ) {
	int i;
	for(i = 0;i<ALIAS_HASH_SIZE;i++) {
		aliasEntry_t *h = aliasHash[i];
		while (h) {
			callback( h->name );
			h = h->next;
		}
	}
}

qboolean Alias_Command( void ) {
	aliasEntry_t *alias;
	aliasArgs_t args;
	int cmd;
	char retBuf[ ALIAS_LINE_SIZE];

	alias = Alias_Find( Cmd_Argv(0) );
	if (!alias)
		return qfalse;
	if (aliasParseArgs( &args, Cmd_ArgsFromOriginal( 0 ) )) {
		for (cmd = 0; cmd < alias->cmdCount; cmd++) {
			if (aliasRun( alias->cmdList[cmd], &args, retBuf, sizeof( retBuf )) > 0)
				Cbuf_ExecuteText( EXEC_NOW, retBuf );
		}
	}
	return qtrue;
}

static void Alias_Alias_f( void ) {
	const char *name, *cmd;

	name = Cmd_Argv( 1 );
	cmd = Cmd_ArgsFrom( 2 );

	if (!name[0]) {
		Com_Printf( "alias (name) (cmd) to set an alias\n" );
		return;
	}
	if (!cmd[0]) {
		aliasEntry_t *exist = Alias_Find( name );
		if (exist) {
			int i;
			Com_Printf( "alias %s set to\n", name );
			for (i = 0;i< exist->cmdCount;i++)
				Com_Printf("    %s\n", exist->cmdList[i]);
		} else {
			Com_Printf( "alias %s doesn't exist\n", name );
		}
		return;
	}
	Alias_Add( name, cmd );
}

static void Alias_Unalias_f( void ) {
	const char *name, *cmd;

	name = Cmd_Argv( 1 );
	cmd = Cmd_ArgsFrom( 2 );

	if (!name[0]) {
		Com_Printf( "unalias (name) remove an alias\n" );
		return;
	}
	if (Alias_Remove( name )) {
		Com_Printf( "alias %s removed", name );
	} else {
		Com_Printf( "alias %s not found", name );
	}
}

static void Alias_AliasList_f( void ) {
	int i, count;
	aliasEntry_t **sortTable;

	count = 0;
	for(i = 0;i<ALIAS_HASH_SIZE;i++) {
		aliasEntry_t *h = aliasHash[i];
		while(h) {
			count++;
			h = h->next;
		}
	}
	/* Create a table of entries to be sorted */
	sortTable = Hunk_AllocateTempMemory( count * sizeof( void *));
	count = 0;
	for(i = 0;i<ALIAS_HASH_SIZE;i++) {
		aliasEntry_t *h = aliasHash[i];
		while(h) {
			sortTable[count++] = h;
			h = h->next;
		}
	}
	if (!count) {
		Com_Printf( "No aliases defined\n" );
		return;
	} else {
		Com_Printf( "%d aliases defined\n", count );
	}
	/* Do a simple selecttion sort to get them alphabetical */
	for(i = 0;i < count-1;i++) {
		aliasEntry_t *copy;
		int j, min = i;
		for (j = i+1; j < count; j++){
			if (Q_stricmp( sortTable[j]->name, sortTable[min]->name) < 0) {
				min = j;
			}
		}
		copy = sortTable[i]; 
		sortTable[i] = sortTable[min]; 
		sortTable[min] = copy;
	}
	for(i = 0;i < count;i++) {
		int line;
		Com_Printf( "alias %s\n", sortTable[i]->name );
		for (line = 0;line< sortTable[i]->cmdCount;line++)
			Com_Printf("    %s\n", sortTable[i]->cmdList[line]);
	}
	Hunk_FreeTempMemory( sortTable );
}

static void Alias_Eval_f( void ) {
	evalParse_t parse;
	
	if (Cmd_Argc() < 2) {
		Com_Printf("Alias eval tester, valid words are +,-,*,/,(),sqrt,sin,cos,tan,asin,acos,atan,floor,ceil\n");
		Com_Printf("Angles are done in 360 degrees scale\n");
		return;
	}	
	evalPrepare( &parse, Cmd_ArgsFrom( 1 ) );
	if (!parse.error)
		Com_Printf( "Result %f\n", evalParse( &parse, qtrue ));
}

static void Alias_Tags_f( void ) {
	int tagCount = sizeof( aliasTagTable ) / sizeof( aliasTag_t );
	aliasTag_t *searchTag = aliasTagTable;
	
	Com_Printf("List of tags alias will replace\n");
	for ( ; tagCount > 0; tagCount--, searchTag++) {
		Com_Printf("%-8s, %s\n", searchTag->tag, searchTag->help );
	}
}

static void Alias_Run_f( void ) {
	char aliasCmd[ALIAS_LINE_SIZE];
	char aliasArgs[ALIAS_LINE_SIZE];
	char result[ALIAS_LINE_SIZE];
	aliasArgs_t args;

	if (Cmd_Argc() < 2) {
		Com_Printf("AliasRun \"command\" arguments \n");
	}
	Q_strncpyz( aliasCmd, Cmd_Argv( 1 ), sizeof( aliasCmd) );
	Com_sprintf( aliasArgs, sizeof( aliasArgs), "aliasRun %s", Cmd_ArgsFromOriginal( 2 ));
	if (!aliasParseArgs( &args, aliasArgs))
		return;
	if (aliasRun( aliasCmd, &args, result, sizeof( result )) > 0)
		Cbuf_ExecuteText( EXEC_INSERT, result );
}


void Alias_Init( void ) {
	Cmd_AddCommand ("alias", Alias_Alias_f);
	Cmd_AddCommand ("aliasList", Alias_AliasList_f);
	Cmd_AddCommand ("unalias", Alias_Unalias_f);
	Cmd_AddCommand ("aliasEval", Alias_Eval_f );
	Cmd_AddCommand ("aliasTags", Alias_Tags_f );
	Cmd_AddCommand ("aliasRun", Alias_Run_f );
}
