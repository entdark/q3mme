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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "win_local.h"
#include <lmerr.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

/*
=================
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath(void)
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
==============
Sys_Dirname
==============
*/
const char *Sys_Dirname( char *path )
{
	static char dir[ MAX_OSPATH ] = { 0 };
	int length;

	Q_strncpyz( dir, path, sizeof( dir ) );
	length = strlen( dir ) - 1;

	while( length > 0 && dir[ length ] != '\\' )
		length--;

	dir[ length ] = '\0';

	return dir;
}

/*
================
Sys_Milliseconds
================
*/
int			sys_timeBase;
int Sys_Milliseconds (void)
{
	int			sys_curtime;
	static qboolean	initialized = qfalse;

	if (!initialized) {
		sys_timeBase = timeGetTime();
		initialized = qtrue;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

/*
================
Sys_SnapVector
================
*/
#ifndef __GNUC__ //see snapvectora.s
/*
================
Sys_SnapVector
================
*/
void Sys_SnapVector( float *v )
{
	int i;
	float f;

	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
	v++;
	f = *v;
	__asm	fld		f;
	__asm	fistp	i;
	*v = i;
}
#endif

/*
**
** Disable all optimizations temporarily so this code works correctly!
**
*/
#ifdef _MSC_VER
#pragma optimize( "", off )
#endif

// If you fancy porting this stuff to AT&T then feel free... :)
// It's not actually used functionally though, so it may be a waste of effort
#ifndef __MINGW32__
/*
** --------------------------------------------------------------------------------
**
** PROCESSOR STUFF
**
** --------------------------------------------------------------------------------
*/
static void CPUID( int func, unsigned regs[4] )
{
	unsigned regEAX, regEBX, regECX, regEDX;

#ifndef __VECTORC
	__asm mov eax, func
	__asm __emit 00fh
	__asm __emit 0a2h
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx

	regs[0] = regEAX;
	regs[1] = regEBX;
	regs[2] = regECX;
	regs[3] = regEDX;
#else
	regs[0] = 0;
	regs[1] = 0;
	regs[2] = 0;
	regs[3] = 0;
#endif
}

static int IsPentium( void )
{
	__asm 
	{
		pushfd						// save eflags
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		set21				// bit 21 is not set, so jump to set_21
		and		eax, 0xffdfffff		// clear bit 21
		push	eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		good
		jmp		err					// cpuid not supported
set21:
		or		eax, 0x00200000		// set ID bit
		push	eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop		eax
		test	eax, 0x00200000		// if bit 21 is on
		jnz		good
		jmp		err
	}

err:
	return qfalse;
good:
	return qtrue;
}

static int Is3DNOW( void )
{
	unsigned regs[4];
	char pstring[16];
	char processorString[13];

	// get name of processor
	CPUID( 0, ( unsigned int * ) pstring );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

//  REMOVED because you can have 3DNow! on non-AMD systems
//	if ( strcmp( processorString, "AuthenticAMD" ) )
//		return qfalse;

	// check AMD-specific functions
	CPUID( 0x80000000, regs );
	if ( regs[0] < 0x80000000 )
		return qfalse;

	// bit 31 of EDX denotes 3DNOW! support
	CPUID( 0x80000001, regs );
	if ( regs[3] & ( 1 << 31 ) )
		return qtrue;

	return qfalse;
}

static int IsKNI( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 25 of EDX denotes KNI existence
	if ( regs[3] & ( 1 << 25 ) )
		return qtrue;

	return qfalse;
}

static int IsMMX( void )
{
	unsigned regs[4];

	// get CPU feature bits
	CPUID( 1, regs );

	// bit 23 of EDX denotes MMX existence
	if ( regs[3] & ( 1 << 23 ) )
		return qtrue;
	return qfalse;
}

int Sys_GetProcessorId( void )
{
#if defined _M_ALPHA
	return CPUID_AXP;
#elif !defined _M_IX86
	return CPUID_GENERIC;
#else

	// verify we're at least a Pentium or 486 w/ CPUID support
	if ( !IsPentium() )
		return CPUID_INTEL_UNSUPPORTED;

	// check for MMX
	if ( !IsMMX() )
	{
		// Pentium or PPro
		return CPUID_INTEL_PENTIUM;
	}

	// see if we're an AMD 3DNOW! processor
	if ( Is3DNOW() )
	{
		return CPUID_AMD_3DNOW;
	}

	// see if we're an Intel Katmai
	if ( IsKNI() )
	{
		return CPUID_INTEL_KATMAI;
	}

	// by default we're functionally a vanilla Pentium/MMX or P2/MMX
	return CPUID_INTEL_MMX;

#endif
}
#endif

/*
**
** Re-enable optimizations back to what they were
**
*/
#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

//============================================

char *Sys_GetCurrentUser( void )
{
	static char s_userName[1024];
	unsigned long size = sizeof( s_userName );


	if ( !GetUserName( s_userName, &size ) )
		strcpy( s_userName, "player" );

	if ( !s_userName[0] )
	{
		strcpy( s_userName, "player" );
	}

	return s_userName;
}

char	*Sys_DefaultHomePath(void) {
	return NULL;
}

char *Sys_DefaultInstallPath(void)
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

#define SHAREDDATA_SIZE 512
#define MAPNAME PROGRAM_MUTEX"_Map"
HANDLE hMapFile;
qboolean Sys_CopySharedData(void *data, size_t size) {
	HANDLE hMapFile;
	LPCTSTR pBuf;

	hMapFile = CreateFileMapping(
					INVALID_HANDLE_VALUE,    // use paging file
					NULL,                    // default security
					PAGE_READWRITE,          // read/write access
					0,                       // maximum object size (high-order DWORD)
					SHAREDDATA_SIZE,         // maximum object size (low-order DWORD)
					MAPNAME); 
	if (hMapFile == NULL) {
		hMapFile = OpenFileMapping(
					FILE_MAP_ALL_ACCESS,   // read/write access
					FALSE,                 // do not inherit the name
					MAPNAME);  
		if (hMapFile == NULL) {
			return qfalse;
		}
	}

	pBuf = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
						FILE_MAP_ALL_ACCESS, // read/write permission
						0,
						0,
						SHAREDDATA_SIZE);
	if (pBuf == NULL) {
		return qfalse;
	}

	CopyMemory((PVOID)pBuf, data, size);
	UnmapViewOfFile(pBuf);
//	CloseHandle(hMapFile);
	return qtrue;
}

void *Sys_GetSharedData(void) {
	static char ret[SHAREDDATA_SIZE];
	HANDLE hMapFile;
	LPCTSTR pBuf;

	hMapFile = OpenFileMapping(
					FILE_MAP_ALL_ACCESS,   // read/write access
					FALSE,                 // do not inherit the name
					MAPNAME);              // name of mapping object
	if (hMapFile == NULL) {
		return NULL;
	}

	pBuf = (LPTSTR)MapViewOfFile(hMapFile, // handle to map object
				FILE_MAP_ALL_ACCESS,  // read/write permission
				0,
				0,
				SHAREDDATA_SIZE);
	if (pBuf == NULL) {
		CloseHandle(hMapFile);
		return NULL;
	}

	Com_sprintf(ret, sizeof(ret), pBuf);
	UnmapViewOfFile(pBuf);
	CloseHandle(hMapFile);
	return ret;
}

#include <intrin.h>
const char *Sys_CpuFeatures(void) {
	static char features[1024] = {0};
#define cpuidex(info, x) __cpuidex(info, x, 0)
	int info[4];
	cpuidex(info, 0);
	int ids = info[0];
	cpuidex(info, 0x80000000);
	unsigned int exids = info[0];
	//  Detect Features
	if (ids >= 0x00000001) {
		cpuidex(info, 0x00000001);
		if ((info[3] & ((int)1 << 23)) != 0)
			strcat(features, "MMX\n");
		if ((info[3] & ((int)1 << 25)) != 0)
			strcat(features, "SSE\n");
		if ((info[3] & ((int)1 << 26)) != 0)
			strcat(features, "SSE2\n");
		if ((info[2] & ((int)1 << 0)) != 0)
			strcat(features, "SSE3\n");

		if ((info[2] & ((int)1 << 9)) != 0)
			strcat(features, "SSSE3\n");
		if ((info[2] & ((int)1 << 19)) != 0)
			strcat(features, "SSE41\n");
		if ((info[2] & ((int)1 << 20)) != 0)
			strcat(features, "SSE42\n");
		if ((info[2] & ((int)1 << 25)) != 0)
			strcat(features, "AES\n");

		if ((info[2] & ((int)1 << 28)) != 0)
			strcat(features, "AVX\n");
		if ((info[2] & ((int)1 << 12)) != 0)
			strcat(features, "FMA3\n");

		if ((info[2] & ((int)1 << 30)) != 0)
			strcat(features, "RDRAND\n");
	}
	if (ids >= 0x00000007) {
		cpuidex(info, 0x00000007);
		if ((info[1] & ((int)1 << 5)) != 0)
			strcat(features, "AVX2\n");

		if ((info[1] & ((int)1 << 3)) != 0)
			strcat(features, "BMI1\n");
		if ((info[1] & ((int)1 << 8)) != 0)
			strcat(features, "BMI2\n");
		if ((info[1] & ((int)1 << 19)) != 0)
			strcat(features, "ADX\n");
		if ((info[1] & ((int)1 << 29)) != 0)
			strcat(features, "SHA\n");
		if ((info[2] & ((int)1 << 0)) != 0)
			strcat(features, "PREFETCHWT1\n");

		if ((info[1] & ((int)1 << 16)) != 0)
			strcat(features, "AVX512F\n");
		if ((info[1] & ((int)1 << 28)) != 0)
			strcat(features, "AVX512CD\n");
		if ((info[1] & ((int)1 << 26)) != 0)
			strcat(features, "AVX512PF\n");
		if ((info[1] & ((int)1 << 27)) != 0)
			strcat(features, "AVX512ER\n");
		if ((info[1] & ((int)1 << 31)) != 0)
			strcat(features, "AVX512VL\n");
		if ((info[1] & ((int)1 << 30)) != 0)
			strcat(features, "AVX512BW\n");
		if ((info[1] & ((int)1 << 17)) != 0)
			strcat(features, "AVX512DQ\n");
		if ((info[1] & ((int)1 << 21)) != 0)
			strcat(features, "AVX512IFMA\n");
		if ((info[2] & ((int)1 << 1)) != 0)
			strcat(features, "AVX512VBMI\n");
	}
	if (exids >= 0x80000001) {
		cpuidex(info, 0x80000001);
		if ((info[3] & ((int)1 << 29)) != 0)
			strcat(features, "x64\n");
		if ((info[2] & ((int)1 << 5)) != 0)
			strcat(features, "ABM\n");
		if ((info[2] & ((int)1 << 6)) != 0)
			strcat(features, "SSE4a\n");
		if ((info[2] & ((int)1 << 16)) != 0)
			strcat(features, "FMA4\n");
		if ((info[2] & ((int)1 << 11)) != 0)
			strcat(features, "XOP\n");
	}
	return features;
}