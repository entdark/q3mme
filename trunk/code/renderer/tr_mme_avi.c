/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

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

#include "tr_local.h"
#include "tr_mme.h"
#include "../client/snd_public.h"

static void aviWrite16( void *pos, unsigned short val) {
	unsigned char *data = (unsigned char *)pos;
	data[0] = (val >> 0) & 0xff;
	data[1] = (val >> 8) & 0xff;
}
static void aviWrite32( void *pos, unsigned int val) {
	unsigned char *data = (unsigned char *)pos;
	data[0] = (val >> 0) & 0xff;
	data[1] = (val >> 8) & 0xff;
	data[2] = (val >> 16) & 0xff;
	data[3] = (val >> 24) & 0xff;
}

int aviFillHeader( mmeAviFile_t *aviFile ) {
	char avi_header[AVI_HEADER_SIZE];
	int main_list, nmain, njunk;
	int header_pos=0, HEADER_SIZE = aviFile->audio?0:AVI_HEADER_SIZE;
	int i;
	qboolean storeMJPEG = (qboolean)aviFile->format;

	if (!aviFile->file)
		return 0;
	Com_Memset( avi_header, 0, sizeof( avi_header ));

#define AVIOUT4(_S_) memcpy(&avi_header[header_pos],_S_,4);header_pos+=4;
#define AVIOUTw(_S_) aviWrite16(&avi_header[header_pos], _S_);header_pos+=2;
#define AVIOUTd(_S_) aviWrite32(&avi_header[header_pos], _S_);header_pos+=4;
	/* Try and write an avi header */
	AVIOUT4("RIFF");                    // Riff header 
	AVIOUTd(HEADER_SIZE + aviFile->written - 8 + aviFile->iframes * 16);
	AVIOUT4("AVI ");
	AVIOUT4("LIST");                    // List header
	main_list = header_pos;
	AVIOUTd(0);				            // TODO size of list
	AVIOUT4("hdrl");
	AVIOUT4("avih");
	AVIOUTd(56);                        /* # of bytes to follow */
	AVIOUTd(1000000 / aviFile->fps);    /* Microseconds per frame */
	AVIOUTd(aviFile->audio?(aviFile->maxSize*aviFile->fps):0);
	AVIOUTd(0);                         /* PaddingGranularity (whatever that might be) */
	AVIOUTd(0x110);                     /* Flags,0x10 has index, 0x100 interleaved */
	AVIOUTd(aviFile->frames);		    /* TotalFrames */
	AVIOUTd(0);                         /* InitialFrames */
	if (aviFile->audio) {
		AVIOUTd(2);
	} else {
		AVIOUTd(1);						/* Stream count */
	}
	AVIOUTd(aviFile->audio?(aviFile->maxSize):0); /* SuggestedBufferSize */
	AVIOUTd(aviFile->width);	        /* Width */
	AVIOUTd(aviFile->height);		    /* Height */
	AVIOUTd(0);                         /* TimeScale:  Unit used to measure time */
	AVIOUTd(0);                         /* DataRate:   Data rate of playback     */
	AVIOUTd(0);                         /* StartTime:  Starting time of AVI data */
	AVIOUTd(0);                         /* DataLength: Size of AVI data chunk    */

	/* Video stream list */
	AVIOUT4("LIST");
	AVIOUTd(4 + 8 + 56 + 8 + 40);       /* Size of the list */
	AVIOUT4("strl");
	/* video stream header */
    AVIOUT4("strh");
	AVIOUTd(56);                        /* # of bytes to follow */
	AVIOUT4("vids");                    /* Type */
	if ( aviFile->format == 1) { /* Handler */
		AVIOUT4("MJPG");      		    
	} else {
		AVIOUTd(0);
	}
	AVIOUTd(0);                         /* Flags */
	AVIOUTd(0);                         /* Reserved, MS says: wPriority, wLanguage */
	AVIOUTd(0);                         /* InitialFrames */
	AVIOUTd(1000000);                   /* Scale */
	AVIOUTd(1000000 * aviFile->fps);    /* Rate: Rate/Scale == samples/second */
	AVIOUTd(0);                         /* Start */
	AVIOUTd(aviFile->frames);           /* Length */
	AVIOUTd(aviFile->audio?(aviFile->maxSize):0); /* SuggestedBufferSize */
	AVIOUTd(~0);                        /* Quality */
	AVIOUTd(0);                         /* SampleSize */
	if (aviFile->audio) {
		AVIOUTw(0);                     /* Frame */
		AVIOUTw(0);                     /* Frame */
		AVIOUTw(aviFile->width);        /* Frame */
		AVIOUTw(aviFile->height);       /* Frame */
	} else {
		AVIOUTd(0);                     /* Frame */
		AVIOUTd(0);                     /* Frame */
	}
	/* The video stream format */
	AVIOUT4("strf");
	AVIOUTd(40);                        /* # of bytes to follow */
	AVIOUTd(40);                        /* Size */
	AVIOUTd(aviFile->width);            /* Width */
	AVIOUTd(aviFile->height);           /* Height */
	AVIOUTw(1);							/* Planes */
	if (!storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTw(8);						/* BitCount */
	} else {
		AVIOUTw(24);					/* BitCount */
	}
	if ( storeMJPEG ) { /* Compression */
		AVIOUT4("MJPG");
		AVIOUTd(aviFile->width * aviFile->height); /* SizeImage (in bytes?) */
	} else {
		AVIOUTd(0);
		AVIOUTd(aviFile->width * aviFile->height*3); /* SizeImage (in bytes?) */
	}
	AVIOUTd(0);                  /* XPelsPerMeter */
	AVIOUTd(0);                  /* YPelsPerMeter */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTd(255);			 /* colorUsed */
	} else {
		AVIOUTd(0);				 /* colorUsed */
	}
	AVIOUTd(0);                  /* ClrImportant: Number of colors important */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		for(i = 0;i<255;i++) {
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = 0;
		}
	}
	/* Audio stream list */
	if (aviFile->audio) {
		AVIOUT4("LIST");
		AVIOUTd(4 + 8 + 56 + 8 + 18); /* Size of the list */
		AVIOUT4("strl");
		AVIOUT4("strh");
		AVIOUTd(56);			 /* # of bytes to follow */
		AVIOUT4("auds");
		AVIOUTd(0);				 /* FCC */
		AVIOUTd(0);				 /* Flags */
		AVIOUTd(0);				 /* Priority */
		AVIOUTd(0);				 /* InitialFrame */

		AVIOUTd((16/8)*2);		 /* Timescale */
		AVIOUTd(((16/8)*2) * MME_SAMPLERATE); /* DataRate */
		AVIOUTd(0);				 /* StartTime */
		AVIOUTd(aviFile->awritten / ((16/8)*2)); /* DataLength */

		AVIOUTd(0);				 /* SuggestedBufferSize */
		AVIOUTd(~0);			 /* Quality */
		AVIOUTd(((16/8)*2));	 /* SampleSize */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */

		AVIOUT4("strf");
		AVIOUTd(18);			 /* # of bytes to follow */
		AVIOUTw(1);				 /* FormatTag */
		AVIOUTw(2);				 /* Channels */
		AVIOUTd(MME_SAMPLERATE); /* SamplesPerSec */
		AVIOUTd(((16/8)*2) * MME_SAMPLERATE); /* AvgBytesPerSec */
		AVIOUTw(((16/8)*2));	 /* BlockAlign */
		AVIOUTw(16);			 /* BitsPerSample */
		AVIOUTw(0);				 /* bSize */
	}
	nmain = header_pos - main_list - 4;
	/* Finish stream list, i.e. put number of bytes in the list to proper pos */
	if (!aviFile->audio) {
		njunk = AVI_HEADER_SIZE - 8 - 12 - header_pos;
		AVIOUT4("JUNK");
		AVIOUTd(njunk);
	}
	if (header_pos > HEADER_SIZE && !aviFile->audio) {
		ri.Error( ERR_FATAL, "Avi Header too large\n" );
	}
	/* Fix the size of the main list */
	if (aviFile->audio) {
		int last_header_pos = header_pos;
		header_pos = main_list;
		AVIOUTd(nmain);
		header_pos = last_header_pos;
	} else {
		header_pos = main_list;
		AVIOUTd(nmain);
		header_pos = AVI_HEADER_SIZE - 12;
	}
	AVIOUT4("LIST");
	AVIOUTd(aviFile->written+4); /* Length of list in bytes */
	AVIOUT4("movi");
	if (aviFile->audio)
		fwrite( &avi_header, 1, header_pos, aviFile->file );
	else
		fwrite( &avi_header, 1, HEADER_SIZE, aviFile->file );
	return header_pos;
}

void aviClose( mmeAviFile_t *aviFile ) {
	char avi_header[AVI_HEADER_SIZE];
	char index[16];
	int main_list, nmain, njunk;
	int header_pos=0, HEADER_SIZE = aviFile->audio?aviFile->header:AVI_HEADER_SIZE;
	int framePos, i;
	qboolean storeMJPEG = aviFile->format;

	if (!aviFile->file)
		return;
	Com_Memset( avi_header, 0, sizeof( avi_header ));

#define AVIOUT4(_S_) memcpy(&avi_header[header_pos],_S_,4);header_pos+=4;
#define AVIOUTw(_S_) aviWrite16(&avi_header[header_pos], _S_);header_pos+=2;
#define AVIOUTd(_S_) aviWrite32(&avi_header[header_pos], _S_);header_pos+=4;
	/* Try and write an avi header */
	AVIOUT4("RIFF");                    // Riff header 
	AVIOUTd(HEADER_SIZE + aviFile->written - 8 + aviFile->iframes * 16);
	AVIOUT4("AVI ");
	AVIOUT4("LIST");                    // List header
	main_list = header_pos;
	AVIOUTd(0);				            // TODO size of list
	AVIOUT4("hdrl");
	AVIOUT4("avih");
	AVIOUTd(56);                         /* # of bytes to follow */
	AVIOUTd(1000000 / aviFile->fps);     /* Microseconds per frame */
	AVIOUTd(aviFile->audio?(aviFile->maxSize*aviFile->fps):0);
	AVIOUTd(0);                         /* PaddingGranularity (whatever that might be) */
	AVIOUTd(0x110);                     /* Flags,0x10 has index, 0x100 interleaved */
	AVIOUTd(aviFile->frames);		    /* TotalFrames */
	AVIOUTd(0);                         /* InitialFrames */
	if (aviFile->audio) {
		AVIOUTd(2);
	} else {
		AVIOUTd(1);						/* Stream count */
	}
	AVIOUTd(aviFile->audio?(aviFile->maxSize):0); /* SuggestedBufferSize */
	AVIOUTd(aviFile->width);	        /* Width */
	AVIOUTd(aviFile->height);		    /* Height */
	AVIOUTd(0);                         /* TimeScale:  Unit used to measure time */
	AVIOUTd(0);                         /* DataRate:   Data rate of playback     */
	AVIOUTd(0);                         /* StartTime:  Starting time of AVI data */
	AVIOUTd(0);                         /* DataLength: Size of AVI data chunk    */

	/* Video stream list */
	AVIOUT4("LIST");
	AVIOUTd(4 + 8 + 56 + 8 + 40);       /* Size of the list */
	AVIOUT4("strl");
	/* video stream header */
    AVIOUT4("strh");
	AVIOUTd(56);                        /* # of bytes to follow */
	AVIOUT4("vids");                    /* Type */
	if ( aviFile->format == 1) { /* Handler */
		AVIOUT4("MJPG");      		    
	} else {
		AVIOUTd(0);
	}
	AVIOUTd(0);                         /* Flags */
	AVIOUTd(0);                         /* Reserved, MS says: wPriority, wLanguage */
	AVIOUTd(0);                         /* InitialFrames */
	AVIOUTd(1000000);                   /* Scale */
	AVIOUTd(1000000 * aviFile->fps);    /* Rate: Rate/Scale == samples/second */
	AVIOUTd(0);                         /* Start */
	AVIOUTd(aviFile->frames);           /* Length */
	AVIOUTd(aviFile->audio?(aviFile->maxSize):0); /* SuggestedBufferSize */
	AVIOUTd(~0);                        /* Quality */
	AVIOUTd(0);                         /* SampleSize */
	if (aviFile->audio) {
		AVIOUTw(0);                     /* Frame */
		AVIOUTw(0);                     /* Frame */
		AVIOUTw(aviFile->width);        /* Frame */
		AVIOUTw(aviFile->height);       /* Frame */
	} else {
		AVIOUTd(0);                     /* Frame */
		AVIOUTd(0);                     /* Frame */
	}
	/* The video stream format */
	AVIOUT4("strf");
	AVIOUTd(40);                        /* # of bytes to follow */
	AVIOUTd(40);                        /* Size */
	AVIOUTd(aviFile->width);            /* Width */
	AVIOUTd(aviFile->height);           /* Height */
	AVIOUTw(1);							/* Planes */
	if (!storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTw(8);						/* BitCount */
	} else {
		AVIOUTw(24);					/* BitCount */
	}
	if ( storeMJPEG ) { /* Compression */
		AVIOUT4("MJPG");      		    
		AVIOUTd(aviFile->width * aviFile->height); /* SizeImage (in bytes?) */
	} else {
		AVIOUTd(0);
		AVIOUTd(aviFile->width * aviFile->height*3);  /* SizeImage (in bytes?) */
	}
	AVIOUTd(0);                  /* XPelsPerMeter */
	AVIOUTd(0);                  /* YPelsPerMeter */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		AVIOUTd(255);			 /* colorUsed */
	} else {
		AVIOUTd(0);				 /* colorUsed */
	}
	AVIOUTd(0);                  /* ClrImportant: Number of colors important */
	if ( !storeMJPEG && aviFile->type == mmeShotTypeGray ) {
		for(i = 0;i<255;i++) {
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = i;
			avi_header[header_pos++] = 0;
		}
	}
	/* Audio stream list */
	if (aviFile->audio) {
		AVIOUT4("LIST");
		AVIOUTd(4 + 8 + 56 + 8 + 18); /* Size of the list */
		AVIOUT4("strl");
		AVIOUT4("strh");
		AVIOUTd(56);			 /* # of bytes to follow */
		AVIOUT4("auds");
		AVIOUTd(0);				 /* FCC */
		AVIOUTd(0);				 /* Flags */
		AVIOUTd(0);				 /* Priority */
		AVIOUTd(0);				 /* InitialFrame */

		AVIOUTd((16/8)*2);		 /* Timescale */
		AVIOUTd(((16/8)*2) * MME_SAMPLERATE); /* DataRate */
		AVIOUTd(0);				 /* StartTime */
		AVIOUTd(aviFile->awritten / ((16/8)*2)); /* DataLength */

		AVIOUTd(0);				 /* SuggestedBufferSize */
		AVIOUTd(~0);			 /* Quality */
		AVIOUTd(((16/8)*2));	 /* SampleSize */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */
		AVIOUTw(0);				 /* Frame */

		AVIOUT4("strf");
		AVIOUTd(18);			 /* # of bytes to follow */
		AVIOUTw(1);				 /* FormatTag */
		AVIOUTw(2);				 /* Channels */
		AVIOUTd(MME_SAMPLERATE); /* SamplesPerSec */
		AVIOUTd(((16/8)*2) * MME_SAMPLERATE); /* AvgBytesPerSec */
		AVIOUTw(((16/8)*2));	 /* BlockAlign */
		AVIOUTw(16);			 /* BitsPerSample */
		AVIOUTw(0);				 /* bSize */
	}
	nmain = header_pos - main_list - 4;
	/* Finish stream list, i.e. put number of bytes in the list to proper pos */
	if (!aviFile->audio) {
		njunk = AVI_HEADER_SIZE - 8 - 12 - header_pos;
		AVIOUT4("JUNK");
		AVIOUTd(njunk);
	}
	if ( header_pos > HEADER_SIZE) {
		ri.Error( ERR_FATAL, "Avi Header too large\n" );
	}
	/* Fix the size of the main list */
	if (aviFile->audio) {
		int last_header_pos = header_pos;
		header_pos = main_list;
		AVIOUTd(nmain);
		header_pos = last_header_pos;
	} else {
		header_pos = main_list;
		AVIOUTd(nmain);
		header_pos = AVI_HEADER_SIZE - 12;
	}
	AVIOUT4("LIST");
	AVIOUTd(aviFile->written+4); /* Length of list in bytes */
	AVIOUT4("movi");
	/* First add the index table to the end */
	fwrite( "idx1", 4, 1, aviFile->file );
	aviWrite32( index, aviFile->iframes * 16 );
	fwrite( index, 4, 1, aviFile->file );
	/* Bring on the load of small writes! */
	index[0] = '0';index[1] = '0';
	index[2] = 'd';index[3] = 'b';
	framePos = 4;		//filepos points to the block size
	for (i = 0; i < aviFile->iframes; i++) {
		int size = aviFile->index[i];
		if (aviFile->audio && aviFile->aindex[i] == aviFile->index[i]) {
			index[1] = '1';index[2] = 'w';
			aviWrite32( index+4, 0 );
		} else {
			index[1] = '0';index[2] = 'd';
			aviWrite32( index+4, 0x10 );
		}
		aviWrite32( index+8, framePos );
		aviWrite32( index+12, size );
		framePos += (size + 9) & ~1;
		fwrite( index, 1, 16, aviFile->file );
	}
	fseek( aviFile->file, 0, SEEK_SET );
	fwrite( &avi_header, 1, HEADER_SIZE, aviFile->file );
	fclose( aviFile->file );
	Com_Memset( aviFile, 0, sizeof( *aviFile ));
}

static qboolean aviOpen( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, qboolean audio) {
	char fileName[MAX_OSPATH];
	int i;

	if (aviFile->file) {
		Com_Printf( "wtf openAvi on an open handler" );
		return qfalse;
	}
	/* First see if the file already exist */
	for (i = 0;i < AVI_MAX_FILES;i++) {
		Com_sprintf( fileName, sizeof(fileName), "%s.%03d.avi", name, i );
		if (!FS_FileExists( fileName ))
			break;
	}
	if (i == AVI_MAX_FILES) {
		Com_Printf( "Max avi segments reached\n");
		return qfalse;
	}
	ri.FS_WriteFile( fileName, &tr, AVI_HEADER_SIZE );
	aviFile->file = ri.FS_DirectOpen( fileName, "w+b" );
	if (!aviFile->file) {
		Com_Printf( "Failed to open %s for avi output\n", fileName );
		return qfalse;
	}
	/* File should have been reset to 0 size */
//	fwrite( &tr, 1, AVI_HEADER_SIZE, aviFile->file );
	aviFile->width = width;
	aviFile->height = height;
	aviFile->fps = fps;
	aviFile->frames = 0;
	aviFile->iframes = 0;
	aviFile->written = 0;
	aviFile->maxSize = 0;
	aviFile->format = mme_aviFormat->integer;
	aviFile->type = type;
	Q_strncpyz( aviFile->name, name, sizeof( aviFile->name ));
	
	if (audio) {
//		aviFile->header = aviFillHeader(aviFile);
//		ri.FS_Write( aviHeader, aviFile->header, aviFile->f );
		aviFile->aframes = 0;
		aviFile->awritten = 0;
		aviFile->audio = qtrue;
	}
	aviFile->header = aviFillHeader(aviFile);
	return qtrue;
}

static qboolean aviValid( const mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, qboolean audio ) {
	if (!aviFile->file)
		return qfalse;
	if (aviFile->width != width)
		return qfalse;
	if (aviFile->height != height)
		return qfalse;
	if (aviFile->fps != fps )
		return qfalse;
	if (aviFile->type != type)
		return qfalse;
	if (Q_stricmp(aviFile->name, name))
		return qfalse;
	if (aviFile->written >= AVI_MAX_SIZE)
		return qfalse;
	if (mme_aviFormat->integer != aviFile->format)
		return qfalse;
	if (aviFile->audio != audio)
		return qfalse;
	return qtrue;
}

void mmeAviShot( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, byte *inBuf, qboolean audio ) {
	byte *outBuf;
	int i, pixels, outSize;
	if (!fps)
		return;
	if (!aviValid( aviFile, name, type, width, height, fps, audio )) {
		aviClose( aviFile );
		if (!aviOpen( aviFile, name, type, width, height, fps, audio ))
			return;
	}
	pixels = width*height;
	outSize = width*height*3 + 2048; //Allocate bit more to be safish?
	outBuf = ri.Hunk_AllocateTempMemory( outSize + 8);
	outBuf[0] = '0';outBuf[1] = '0';
	outBuf[2] = 'd';outBuf[3] = 'b';
	if ( aviFile->format == 0 ) {
		switch (type) {
		case mmeShotTypeGray:
			for (i = 0;i<pixels;i++) {
				outBuf[8 + i] = inBuf[i];
			};
			outSize = pixels;
			break;
		case mmeShotTypeRGB:
			for (i = 0;i<pixels;i++) {
				outBuf[8 + i*3 + 0 ] = inBuf[ i*3 + 2];
				outBuf[8 + i*3 + 1 ] = inBuf[ i*3 + 1];
				outBuf[8 + i*3 + 2 ] = inBuf[ i*3 + 0];
			}
			outSize = width * height * 3;
			break;
		} 
	} else if ( aviFile->format == 1 ) {
		outSize = SaveJPG( mme_jpegQuality->integer, width, height, type, inBuf, outBuf + 8, outSize );
	}
	aviWrite32( outBuf + 4, outSize );
	aviFile->index[ aviFile->iframes ] = outSize;
	aviFile->aindex[ aviFile->iframes ] = -1;
	aviFile->frames++;
	aviFile->iframes++;

	outSize = (outSize + 9) & ~1;	//make sure we align on 2 byte boundary, hurray M$
	fwrite( outBuf, 1, outSize, aviFile->file );
	aviFile->written += outSize;
	
	if (outSize > aviFile->maxSize)
		aviFile->maxSize = outSize;

	ri.Hunk_FreeTempMemory( outBuf );
}

void mmeAviSound( mmeAviFile_t *aviFile, const char *name, mmeShotType_t type, int width, int height, float fps, const byte *soundBuf, int size ) {
	static byte inBuf[MME_SAMPLERATE] = {0};
	static int bytesInBuf = 0;
	if (!fps)
		return;
	if (!aviValid( aviFile, name, type, width, height, fps, qtrue )) {
		aviClose( aviFile );
		if (!aviOpen( aviFile, name, type, width, height, fps, qtrue ))
			return;
	}
	if (!size)
		return;
	if (bytesInBuf + size > MME_SAMPLERATE) {
#ifdef _DEBUG
		Com_Printf( S_COLOR_YELLOW "WARNING: Audio capture buffer overflow - truncating\n" );
#endif
		size = MME_SAMPLERATE - bytesInBuf;
	}
	Com_Memcpy(&inBuf[bytesInBuf], soundBuf, size);
	bytesInBuf += size;

	if(bytesInBuf >= (int)(double)((double)MME_SAMPLERATE / (double)fps * ((16.0/8.0)*2.0))) {
		byte *outBuf;
		int i;
		size = bytesInBuf;
		outBuf = (byte *)ri.Hunk_AllocateTempMemory( size + 8 );
		outBuf[0] = '0';outBuf[1] = '1';
		outBuf[2] = 'w';outBuf[3] = 'b';
		for (i = 0; i < size; i++) {
			outBuf[8 + i] = inBuf[i];
		}
			
		aviWrite32( outBuf + 4, size );
		aviFile->index[ aviFile->iframes ] = size;
		aviFile->aindex[ aviFile->iframes ] = size;
		aviFile->aframes++;
		aviFile->iframes++;

		size = (size + 9) & ~1;	//make sure we align on 2 byte boundary, hurray M$
		fwrite( outBuf, 1, size, aviFile->file );
		aviFile->written += size;
		aviFile->awritten += size;

		ri.Hunk_FreeTempMemory( outBuf );
		bytesInBuf = 0;
	}
}
