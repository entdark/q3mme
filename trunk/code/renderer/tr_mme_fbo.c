/*
 *      tr_framebuffer.c
 *      
 *      Copyright 2007 Gord Allott <gordallott@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
*/
// tr_framebuffer.c: framebuffer object rendering path code
// Okay i am going to try and document what I doing here, appologies to anyone 
// that already understands this. basically the idea is that normally everything
// opengl renders will be rendered into client memory, that is the space the 
// graphics card reserves for anything thats going to be sent to the monitor.
// Using this method we instead redirect all the rendering to a seperate bit of 
// memory called a frame buffer. 
// we can then bind this framebuffer to a texture and render that texture to the
// client memory again so that the image will be sent to the monitor. this 
// redirection allows for some neat effects to be applied.

// Some ideas for what to use this path for:
//		- Bloom	-done
//		- Rotoscope cartoon effects (edge detect + colour mapping)
//		- Fake anti-aliasing. (edge detect and blur positive matches)
//		- Motion blur
//			- generate a speed vector based on how the camera has moved since 
//			  the last frame and use that to compute per pixel blur vectors 
//		- These would require mods to use some sort of framebuffer qvm api
//			- special effects for certain powerups 
//			- Image Blur when the player is hit
//			- Depth of field blur

#include "tr_local.h"
#ifdef HIDDEN_FBO
#include "tr_glslprogs.h"
#include "qgl.h"

cvar_t *r_framebuffer;
cvar_t *r_framebuffer_bloom;
cvar_t *r_framebuffer_blur_size;
cvar_t *r_framebuffer_blur_ammount;
cvar_t *r_framebuffer_blur_samples;

cvar_t *r_framebuffer_bloom_sharpness;
cvar_t *r_framebuffer_bloom_brightness;

cvar_t *r_framebuffer_rotoscope;
cvar_t *r_framebuffer_rotoscope_zedge;

#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT GL_DEPTH_STENCIL_NV
#endif

#ifndef GL_UNSIGNED_INT_24_8_EXT
#define GL_UNSIGNED_INT_24_8_EXT GL_UNSIGNED_INT_24_8_NV
#endif

static qboolean	packedDepthStencilSupported;
static qboolean	depthTextureSupported;
static qboolean	seperateDepthStencilSupported;


struct glslobj {
	const char **vertex_glsl;
	int vert_numSources;
	const char **fragment_glsl;
	int frag_numSources;
	GLuint vertex;
	GLuint fragment;
	GLuint program;
} glslobj;

struct r_fbuffer {
	GLuint 	fbo;
	GLuint	numBuffers;
	GLuint	*buffers;
	GLuint	*front;					//front buffer
	GLuint	*back;					//back buffer
	GLuint	*depth;					//depth buffer
	GLuint	*stencil;				//stencil buffer
	int		modeFlags;				//the modeflags
	int		renderbuff;				//either FB_FRONT or FB_BACK
} r_fbuffer;

static qboolean needBlur = 0;				 //is set if effects need a blur
static struct r_fbuffer screenBuffer;
static struct r_fbuffer gaussblurBuffer;

//two functions to bind and unbind the main framebuffer, generally just to be
//called externaly

void R_SetGL2DSize (int width, int height) {

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, width, height, 0, 0, 1);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	GL_State( GLS_DEPTHTEST_DISABLE );
	qglDisable( GL_BLEND );
}

void R_DrawQuadMT( GLuint tex1, GLuint tex2, int width, int height ) {
	qglEnable(GL_TEXTURE_2D);
	if ( glState.currenttextures[1] != tex2 ) {
		GL_SelectTexture( 1 );
		qglBindTexture(GL_TEXTURE_2D, tex2); 
		glState.currenttextures[1] = tex2; 
		}
	if ( glState.currenttextures[0] != tex1 ) {
		GL_SelectTexture( 0 );
		qglBindTexture(GL_TEXTURE_2D, tex1);
		glState.currenttextures[0] = tex1; 
	}
	  
	qglBegin(GL_QUADS);
	  qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 1.0); qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 1.0); qglVertex2f(0.0  , 0.0   );	
	  qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1.0, 1.0); qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 1.0); qglVertex2f(width, 0.0   );	
	  qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 1.0, 0.0); qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 0.0); qglVertex2f(width, height);	
	  qglMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 0.0); qglMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 0.0); qglVertex2f(0.0  , height);	
	qglEnd();	;
	
	qglActiveTextureARB(GL_TEXTURE0_ARB);
	//qglDisable(GL_TEXTURE_2D);
}

void R_DrawQuad( GLuint tex, int width, int height) {
	qglEnable(GL_TEXTURE_2D);
	if ( glState.currenttextures[0] != tex ) {
		GL_SelectTexture( 0 );
		qglBindTexture(GL_TEXTURE_2D, tex);
		glState.currenttextures[0] = tex; 
	};

	qglBegin(GL_QUADS);
	  qglTexCoord2f(0.0, 1.0); qglVertex2f(0.0  , 0.0   );	
	  qglTexCoord2f(1.0, 1.0); qglVertex2f(width, 0.0   );	
	  qglTexCoord2f(1.0, 0.0); qglVertex2f(width, height);	
	  qglTexCoord2f(0.0, 0.0); qglVertex2f(0.0  , height);	
	qglEnd();	
}

GLuint *R_CreateTexbuffer(	GLuint *store, int width, int height, 
							qboolean smooth, GLenum bindSize, GLenum bindType) 
{
	//GLuint *store = malloc(sizeof(GLuint));
	GLenum filter = GL_NEAREST;
	
	if (smooth) {
		filter = GL_LINEAR;
	}
	
	qglGenTextures( 1, store );
	qglBindTexture(	GL_TEXTURE_2D, *store );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	qglTexImage2D(	GL_TEXTURE_2D, 0, bindType, width, height, 0, 
					bindType, bindSize, 0 );
	return store;
}

GLuint *R_CreateRenderbuffer(	GLuint *store, int width, int height, 
								GLenum bindType) 
{
	//GLuint *store = malloc(sizeof(GLuint));
	
	qglGenRenderbuffers( 1, store );
	qglBindRenderbuffer( GL_RENDERBUFFER_EXT, *store );
	qglRenderbufferStorage(	GL_RENDERBUFFER_EXT, bindType, width, height );
	
	return store;
}

//------------------------------
// better framebuffer creation
//------------------------------
// for this we do a more opengl way of figuring out what level of framebuffer
// objects are supported. we try each mode from 'best' to 'worst' until we 
// get a mode that works.

#define FB_ZBUFFER 		0x01
#define FB_STENCIL 		0x02
#define FB_PACKED 		0x04
#define FB_ZTEXTURE 	0x08
#define FB_BACKBUFFER	0x10
#define FB_SEPERATEZS	0x20
#define FB_SMOOTH		0x40

#define FB_FRONT		0x01
#define FB_BACK			0x02

int R_DeleteFBuffer( struct r_fbuffer *buffer) {
	int flags = buffer->modeFlags;
	
	if (flags & FB_STENCIL) {
			if (flags & FB_ZTEXTURE) {
				qglDeleteFramebuffers(1, buffer->depth);
				qglDeleteFramebuffers(1, buffer->stencil);
				qglDeleteTextures(1, buffer->depth);
			}
			else {
				qglDeleteRenderbuffers(1, buffer->depth);
				qglDeleteRenderbuffers(1, buffer->stencil);
			}
	}
	else if (flags & FB_ZBUFFER) {
			if (flags & FB_ZTEXTURE) {
				qglDeleteFramebuffers(1, buffer->depth);
				qglDeleteTextures(1, buffer->depth);
			}
			else {
				qglDeleteRenderbuffers(1, buffer->depth);
			}
	}
	if (flags & FB_BACKBUFFER) {
		qglDeleteFramebuffers(1, buffer->back);
		qglDeleteTextures(1, buffer->back);
	}
	
	qglDeleteFramebuffers(1, buffer->front);
	qglDeleteTextures(1, buffer->front);
	
	free(buffer->buffers);
	
	qglDeleteFramebuffers(1, &(buffer->fbo));
	
	return 0;
	
}

int R_CreateFBuffer( struct r_fbuffer *buffer, int width, int height, int flags)
{
	int index = 0;
	qboolean filter = qfalse;
	GLenum glstatus;

	//do some checks
	if ((flags & FB_PACKED) && (flags & FB_SEPERATEZS)) {
		return -1;
	}
	if ((flags & FB_PACKED) && !(flags & FB_STENCIL)) {
		return -2;
	}
	if ((flags & FB_SEPERATEZS) && !(flags & FB_STENCIL)) {
		return -3;
	}
	
	memset( buffer, 0, sizeof( buffer ) );
	//store the flags in the struct
	buffer->modeFlags = flags;
	
	//allocate enough storage buffers
	buffer->numBuffers = 1;
	if (flags & FB_STENCIL) {
		if (flags & FB_PACKED) {
			buffer->numBuffers += 1;
		}
		if (flags & FB_SEPERATEZS) {
			buffer->numBuffers += 2;
		}
	}
	if (flags & FB_ZBUFFER) {
		buffer->numBuffers += 1;
	}
	if (flags & FB_BACKBUFFER) {
		buffer->numBuffers += 1;
	}
	
	buffer->buffers = malloc(sizeof(GLuint) * buffer->numBuffers);
	
	//link the named variables to the storage space
	buffer->front = &(buffer->buffers[index]);
	index++;
	
	if (flags & FB_BACKBUFFER) {
		buffer->back = &(buffer->buffers[index]);
		index++;
	}
	
	if (flags & FB_STENCIL) {
		if (flags & FB_PACKED) {
			buffer->stencil = &(buffer->buffers[index]);
			buffer->depth = &(buffer->buffers[index]);
			index++;
		}
		if (flags & FB_SEPERATEZS) {
			buffer->depth = &(buffer->buffers[index]);
			index++;
			buffer->stencil = &(buffer->buffers[index]);
			index++;
		}
	}
	else if (flags & FB_ZBUFFER) {
		buffer->depth = &(buffer->buffers[index]);
		index++;
	}

	//set the filter state
	if (flags & FB_SMOOTH) {
		filter = qtrue;
	}

	//gen the frame buffer
	qglGenFramebuffers(1, &(buffer->fbo));
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffer->fbo);
	
	if (flags & FB_STENCIL) {
		if (flags & FB_PACKED) {
			if (flags & FB_ZTEXTURE) {
				R_CreateTexbuffer( 	buffer->depth, width, height, qfalse, 
									GL_UNSIGNED_INT_24_8_EXT, GL_DEPTH_STENCIL_EXT);
	
				qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
										GL_TEXTURE_2D, *buffer->depth, 0);
				
				qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, 
										GL_TEXTURE_2D, *buffer->stencil, 0);	
			}
			else {
				R_CreateRenderbuffer(buffer->depth, width, height, GL_DEPTH_STENCIL_EXT);
				qglFramebufferRenderbuffer(		GL_FRAMEBUFFER_EXT, 
												GL_DEPTH_ATTACHMENT_EXT,
												GL_RENDERBUFFER_EXT, 
												*buffer->depth);

				qglFramebufferRenderbuffer(		GL_FRAMEBUFFER_EXT, 
												GL_STENCIL_ATTACHMENT_EXT,
												GL_RENDERBUFFER_EXT, 
												*buffer->stencil);
			}		

		}
		if (flags & FB_SEPERATEZS) {
			if (flags & FB_ZTEXTURE) {
				R_CreateTexbuffer( 	buffer->depth, width, height, qfalse, 
									GL_UNSIGNED_INT, GL_DEPTH_COMPONENT);
	
				qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
										GL_TEXTURE_2D, *buffer->depth, 0);
										
				R_CreateRenderbuffer(buffer->stencil, width, height, GL_STENCIL_INDEX8_EXT);
				
				qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, 
											GL_RENDERBUFFER_EXT, *buffer->stencil);	
			}
			else {
				R_CreateRenderbuffer(buffer->depth, width, height, GL_DEPTH_STENCIL_EXT);
				qglFramebufferRenderbuffer(		GL_FRAMEBUFFER_EXT, 
												GL_DEPTH_ATTACHMENT_EXT,
												GL_RENDERBUFFER_EXT, 
												*buffer->depth);
												
				R_CreateRenderbuffer(buffer->stencil, width, height, GL_STENCIL_INDEX8_EXT);
				qglFramebufferRenderbuffer(		GL_FRAMEBUFFER_EXT, 
												GL_STENCIL_ATTACHMENT_EXT,
												GL_RENDERBUFFER_EXT, 
												*buffer->stencil);
			}
		}
	}
	else if (flags & FB_ZBUFFER) {
		if (flags & FB_ZTEXTURE) {
			R_CreateTexbuffer( 	buffer->depth, width, height, qfalse, 
								GL_UNSIGNED_INT, GL_DEPTH_COMPONENT);
	
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
									GL_TEXTURE_2D, *buffer->depth, 0);
		}
		else {
//			R_CreateRenderbuffer(buffer->depth, width, height, GL_DEPTH_STENCIL_EXT);
			R_CreateRenderbuffer(buffer->depth, width, height, GL_DEPTH_COMPONENT );
			qglFramebufferRenderbuffer(		GL_FRAMEBUFFER_EXT, 
											GL_DEPTH_ATTACHMENT_EXT,
											GL_RENDERBUFFER_EXT, 
											*buffer->depth);
		}
	}
	if (flags & FB_BACKBUFFER) {
		R_CreateTexbuffer(	buffer->back, width, height, filter, 
							GL_UNSIGNED_BYTE, GL_RGBA);
	
		//we link to the second colour attachment
		qglFramebufferTexture2D(	GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
									GL_TEXTURE_2D, *buffer->back, 0);
	}
	//create the main colour buffer
	R_CreateTexbuffer(	buffer->front, width, height, filter, 
						GL_UNSIGNED_BYTE, GL_RGBA);
	
	qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
							GL_TEXTURE_2D, *buffer->front, 0);
	
	buffer->renderbuff = FB_FRONT;
	
	glstatus = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (!(glstatus == GL_FRAMEBUFFER_COMPLETE_EXT)) {
		return -1;
	}		
	
	return 0;
}

qboolean R_TestFbuffer_SeperateDS( void ) {	
	int width = 512;
	int height = 512;
	qboolean status;
	GLenum glstatus;

	//try and get a perfect seperate stencil/zbuffer
	//this does not work on any hardware i know, but might in the future.
	GLuint buffers[3]; //three buffers GL_UNSIGNED_INT_24_8_EXT
	qglGenFramebuffers(3, buffers);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffers[0]);
		
	//create the depth buffer
	R_CreateRenderbuffer( &buffers[2], width, height, GL_DEPTH_COMPONENT);
							
	//stencil buffer as a render buffer
	R_CreateRenderbuffer( &buffers[2], width, height, GL_STENCIL_INDEX8_EXT);
	//attach the textures/renderbuffers to the framebuffer
	qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
								GL_RENDERBUFFER_EXT, buffers[1]);									
	qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
								GL_RENDERBUFFER_EXT, buffers[2]);	
									
	//create our framebuffer context		
	
	R_CreateTexbuffer(	&buffers[0], width, height, qfalse, 
						GL_UNSIGNED_BYTE, GL_RGBA);
	
	//shall we link our texture to the frame buffer? yes!
	qglFramebufferTexture2D(	GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
								GL_TEXTURE_2D, buffers[0], 0);
	
	status = qtrue;
	glstatus = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (!(glstatus == GL_FRAMEBUFFER_COMPLETE_EXT)) {
		status = qfalse;
	}	
	
	
	qglDeleteFramebuffers(1, &buffers[0]);
	qglDeleteTextures(1,  &buffers[0]);
	qglDeleteRenderbuffers(1, &buffers[1]);
	qglDeleteRenderbuffers(1, &buffers[2]);
	
	return status;
}

qboolean R_TestFbuffer_PackedDS( void ) {	
	int width = 512;
	int height = 512;
	qboolean status;
	GLenum glstatus;
	GLuint buffers[2];

	//try and get a packed seperate stencil/zbuffer
	qglGenFramebuffers(2, buffers);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffers[0]);
		
	//create the buffer
	R_CreateRenderbuffer( &buffers[1], width, height, GL_DEPTH_STENCIL_EXT);
	//attach the textures/renderbuffers to the framebuffer
	qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
								GL_RENDERBUFFER_EXT, buffers[1]);									
	qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
								GL_RENDERBUFFER_EXT, buffers[1]);

	
	qglBindRenderbuffer(GL_RENDERBUFFER_EXT, buffers[1]);						
	//create our framebuffer context		
	
	R_CreateTexbuffer(	&buffers[0], width, height, qfalse, 
						GL_UNSIGNED_BYTE, GL_RGBA);
	
	//shall we link our texture to the frame buffer? yes!
	qglFramebufferTexture2D(	GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
								GL_TEXTURE_2D, buffers[0], 0);
	
	status = qtrue;
	glstatus = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (!(glstatus == GL_FRAMEBUFFER_COMPLETE_EXT)) {
		status = qfalse;
	}	
	
	qglDeleteFramebuffers(1, &buffers[0]);
	qglDeleteTextures(1,  &buffers[0]);
	qglDeleteRenderbuffers(1, &buffers[1]);
	
	return status;
}

qboolean R_TestFbuffer_texD( void ) {	
	int width = 512;
	int height = 512;
	qboolean status;
	GLenum glstatus;
	GLuint buffers[2];

	//try and get a packed seperate stencil/zbuffer
	qglGenFramebuffers(2, buffers);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffers[0]);
		
	//create the buffer

	R_CreateTexbuffer(	&buffers[0], width, height, qfalse, 
						GL_UNSIGNED_INT, GL_DEPTH_COMPONENT);
	
	qglFramebufferTexture2D(	GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
								GL_TEXTURE_2D, buffers[1], 0);
						
	//create our framebuffer context		
	
	R_CreateTexbuffer(	&buffers[0], width, height, qfalse, 
						GL_UNSIGNED_BYTE, GL_RGBA);
	
	//shall we link our texture to the frame buffer? yes!
	qglFramebufferTexture2D(	GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
								GL_TEXTURE_2D, buffers[0], 0);
	
	status = qtrue;
	glstatus = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
	if (!(glstatus == GL_FRAMEBUFFER_COMPLETE_EXT)) {
		status = qfalse;
	}	
	
	qglDeleteFramebuffers(1, &buffers[0]);
	qglDeleteTextures(1,  &buffers[0]);
	qglDeleteFramebuffers(1, &buffers[1]);
	qglDeleteTextures(1,  &buffers[1]);
	
	return status;
}


// for shader debugging
void printShaderInfoLog(GLuint obj)
{
#if 0
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	qglGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 1)
    {
        infoLog = (char *)malloc(infologLength);
        qglGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		ri.Printf( PRINT_ALL, "----- Shader InfoLog -----\n" );;
		ri.Printf( PRINT_ALL, infoLog );
        free(infoLog);
    }
#endif
}

void printProgramInfoLog(GLuint obj)
{
#if 0
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

	qglGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

    if (infologLength > 1)
    {
        infoLog = (char *)malloc(infologLength);
        qglGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		ri.Printf( PRINT_ALL, "----- Program InfoLog -----\n" );;
		ri.Printf( PRINT_ALL, infoLog );
        free(infoLog);
    }
#endif
}

void R_Build_glsl(struct glslobj *obj) {
	GLuint vert_shader, frag_shader, program;
	
	vert_shader = qglCreateShader( GL_VERTEX_SHADER_ARB );
	frag_shader = qglCreateShader( GL_FRAGMENT_SHADER_ARB );
	
	qglShaderSource(vert_shader, obj->vert_numSources, obj->vertex_glsl, NULL);
	qglShaderSource(frag_shader, obj->frag_numSources, obj->fragment_glsl, NULL);

	printShaderInfoLog(vert_shader);
	printShaderInfoLog(frag_shader);

	qglCompileShader(vert_shader);
	qglCompileShader(frag_shader);

	program = qglCreateProgram();
	qglAttachShader(program, vert_shader);
	qglAttachShader(program, frag_shader);
	qglLinkProgram(program);

	printProgramInfoLog(program);

	obj->vertex = vert_shader;
	obj->fragment = frag_shader;
	obj->program = program;

}

void R_Delete_glsl(struct glslobj *obj) {
	qglDeleteProgram(obj->program);
	qglDeleteShader(obj->vertex);
	qglDeleteShader(obj->fragment);
}

struct glslobj glslBlur;

void R_FrameBuffer_BlurInit( void ) {
	//inits our blur code;
	int fb_size, samples;
	if ( needBlur != 1 ) {
		return;
	}
	if ( r_framebuffer_blur_size->integer < 2 ) {
		return;
	}
	fb_size = r_framebuffer_blur_size->integer;
	
	R_CreateFBuffer( &gaussblurBuffer, fb_size, fb_size, FB_BACKBUFFER | FB_SMOOTH);
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	
	samples = r_framebuffer_blur_samples->integer;
	//create our glsl shader
	glslBlur.vert_numSources = 1;
	glslBlur.frag_numSources = 2;
	
	glslBlur.vertex_glsl = malloc(sizeof(char *) * glslBlur.vert_numSources); 
	glslBlur.vertex_glsl[0] = glslBase_vert;
	
	glslBlur.fragment_glsl = malloc(sizeof(char *) * glslBlur.frag_numSources); 
	switch (samples) {
		case (5):
			glslBlur.fragment_glsl[0] = glslGauss5;
			break;
		case (7):
			glslBlur.fragment_glsl[0] = glslGauss7;
			break;
		case (9):
			glslBlur.fragment_glsl[0] = glslGauss9;
			break;
		default:
			glslBlur.fragment_glsl[0] = glslGauss9;
			break;
	}
	glslBlur.fragment_glsl[1] = glslBlurMain;

	R_Build_glsl(&glslBlur);
	
}

void R_FrameBuffer_BlurDraw( GLuint *srcTex ) {
	int fb_size;
	GLuint program, loc;
	
	if ( r_framebuffer_blur_size->integer < 2 ) {
		return;
	}
	
	if ( needBlur != 1 ) {
		return;
	}
	fb_size = r_framebuffer_blur_size->integer;
	
	// first we draw the framebuffer into the blur buffer before any fragment
	// programs are used is quicker, the rational behind this is that we want 
	// as many texels to fit inside the texture cache as possible for the 
	// gaussian sampling
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, gaussblurBuffer.fbo);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	R_SetGL2DSize(fb_size, fb_size);		
	qglUseProgram(0);
	R_DrawQuad(	*srcTex, fb_size, fb_size);
	
	//now we do the first gaussian pass
	
	qglDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	R_SetGL2DSize(fb_size, fb_size);		
	
	program = glslBlur.program;
	qglUseProgram(0);
	qglUseProgram(program);
	
	//find and set the samplers
	//set the texture number... silly this really. oh well thats glsl
	loc = qglGetUniformLocation(program, "srcSampler");
	qglUniform1i(loc, 0);
	loc = qglGetUniformLocation(program, "blurSize");
	qglUniform2f(loc, r_framebuffer_blur_ammount->value / 100.0, 0.0);
	
	R_DrawQuad(	*gaussblurBuffer.front, fb_size, fb_size);
		
	//we do the second pass of the blur here
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	loc = qglGetUniformLocation(program, "blurSize");
	qglUniform2f(loc, 0.0, r_framebuffer_blur_ammount->value / 100.0);
	
	R_SetGL2DSize(fb_size, fb_size);
	R_DrawQuad(	*gaussblurBuffer.back, fb_size, fb_size);
	qglUseProgram(0);
	
	//finally the FRONT buffer in the fbo is given the blurred image
	
}

void R_FrameBuffer_BlurDelete( void ) {
	
	R_Delete_glsl(&glslBlur);
	free(( void *)glslBlur.vertex_glsl);
	free(( void *)glslBlur.fragment_glsl);
	
	R_DeleteFBuffer(&gaussblurBuffer);
}

struct glslobj glslRoto;
void R_FrameBuffer_RotoInit( struct r_fbuffer *src ) {
	
	// check to see if we need to create a framebuffer for this (only if there
	// is a shader running after this)
	 
	//create our glsl shader
	glslRoto.vert_numSources = 1;
	glslRoto.frag_numSources = 3;
	
	glslRoto.vertex_glsl = malloc(sizeof(char *) * glslRoto.vert_numSources); 
	glslRoto.vertex_glsl[0] = glslBase_vert;
	
	glslRoto.fragment_glsl = malloc(sizeof(char *) * glslRoto.frag_numSources); 
	glslRoto.fragment_glsl[0] = glslToonColour;
	
	if ((r_framebuffer_rotoscope_zedge->integer) && (src->modeFlags & FB_ZTEXTURE)) {
		glslRoto.fragment_glsl[1] = glslSobelZ;
		glslRoto.fragment_glsl[2] = glslRotoscopeZ;
	} else { 
		glslRoto.fragment_glsl[1] = glslSobel;
		glslRoto.fragment_glsl[2] = glslRotoscope;
	}
	
	R_Build_glsl(&glslRoto);
}

void R_FrameBuffer_RotoDraw( struct r_fbuffer *src, GLuint *srcTex ) { 
	GLuint program, loc;
	program = glslRoto.program;
	qglUseProgram(0);
	qglUseProgram(program);
	
	R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
	
	//find and set the samplers
	loc = qglGetUniformLocation(program, "srcSampler");
	qglUniform1i(loc, 0);
	loc = qglGetUniformLocation(program, "depthSampler");
	qglUniform1i(loc, 1);
	loc = qglGetUniformLocation(program, "texelSize");
	qglUniform2f(loc, 1.0 / glConfig.vidWidth, 1.0 / glConfig.vidHeight);
	
	if ((r_framebuffer_rotoscope_zedge->integer) && (src->modeFlags & FB_ZTEXTURE)) {
		R_DrawQuadMT(	*srcTex, *src->depth, 
						glConfig.vidWidth, glConfig.vidHeight);
	} else {
		R_DrawQuad(	*srcTex, glConfig.vidWidth, glConfig.vidHeight);
	} 
	qglUseProgram(0);
}

void R_FrameBuffer_RotoDelete( void ) {
	R_Delete_glsl(&glslRoto);
	free(( void *)glslRoto.vertex_glsl);
	free(( void *)glslRoto.fragment_glsl);
}

struct glslobj glslBloom;

void R_FrameBuffer_BloomInit( void ) {
	//we need blur for this
	needBlur = 1;
	//create our glsl shader
	glslBloom.vert_numSources = 1;
	glslBloom.frag_numSources = 1;
	glslBloom.vertex_glsl = malloc(sizeof(char *) * glslBlur.frag_numSources); 
	glslBloom.vertex_glsl[0] = glslBase_vert;
	glslBloom.fragment_glsl = malloc(sizeof(char *) * glslBlur.frag_numSources); 
	glslBloom.fragment_glsl[0] = glslSigScreen;

	R_Build_glsl(&glslBloom);
}

void R_FrameBuffer_BloomDraw( GLuint *srcTex ) {
	GLuint program, loc;
	program = glslBloom.program;
	qglUseProgram(0);
	qglUseProgram(program);
	
	R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
	
	//find and set the samplers
	loc = qglGetUniformLocation(program, "srcSampler");
	qglUniform1i(loc, 0);
	loc = qglGetUniformLocation(program, "blurSampler");
	qglUniform1i(loc, 1);
	loc = qglGetUniformLocation(program, "brightness");
	qglUniform1f(loc, r_framebuffer_bloom_brightness->value);
	loc = qglGetUniformLocation(program, "sharpness");
	qglUniform1f(loc, r_framebuffer_bloom_sharpness->value);
	
	R_DrawQuadMT(	*srcTex, *gaussblurBuffer.front, 
					glConfig.vidWidth, glConfig.vidHeight);
	qglUseProgram(0);
	//quick test to just see the blur
	//R_DrawQuad(*gaussblurBuffer.front, glConfig.vidWidth, glConfig.vidHeight);
}

void R_FrameBuffer_BloomDelete( void ) {
	R_Delete_glsl(&glslBloom);
	free(( void *)glslBloom.vertex_glsl);
	free(( void *)glslBloom.fragment_glsl);
}

void R_FrameBuffer_Draw( void ) {
	//draws the framebuffer to the screen, pretty simple really.
	R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
	R_DrawQuad(	*(screenBuffer.front), glConfig.vidWidth, glConfig.vidHeight);
}

void R_FrameBuffer_Init( void ) {
	GLint maxbuffers;
	int screenbuff_flags, status;

	needBlur = 0;
	
	r_framebuffer = ri.Cvar_Get( "r_framebuffer", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_framebuffer_bloom = ri.Cvar_Get( "r_framebuffer_bloom", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_framebuffer_blur_size = ri.Cvar_Get( "r_framebuffer_blur_size", "256", CVAR_ARCHIVE | CVAR_LATCH);
	r_framebuffer_blur_ammount = ri.Cvar_Get( "r_framebuffer_blur_amount", "7", CVAR_ARCHIVE);
	r_framebuffer_blur_samples = ri.Cvar_Get( "r_framebuffer_blur_samples", "9", CVAR_ARCHIVE | CVAR_LATCH);
	r_framebuffer_bloom_sharpness = ri.Cvar_Get( "r_framebuffer_bloom_sharpness", "0.75", CVAR_ARCHIVE );
	r_framebuffer_bloom_brightness = ri.Cvar_Get( "r_framebuffer_bloom_brightness", "0.85", CVAR_ARCHIVE );
	r_framebuffer_rotoscope = ri.Cvar_Get( "r_framebuffer_rotoscope", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_framebuffer_rotoscope_zedge = ri.Cvar_Get( "r_framebuffer_rotoscope_zedge", "0", CVAR_ARCHIVE | CVAR_LATCH);

	// make sure all the commands added here are also		

	if (!glMMEConfig.framebufferObject || !glMMEConfig.shaderSupport) {
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
		return;
	}


	if (r_framebuffer->integer != 1) {
		return;
	}

	ri.Printf( PRINT_ALL, "----- Enabling FrameBuffer Path -----\n" );

	//lets see what works and what doesn't
	packedDepthStencilSupported = qfalse;
	depthTextureSupported = qfalse;
	seperateDepthStencilSupported = qfalse;

	if (R_TestFbuffer_PackedDS()) {
		packedDepthStencilSupported = qtrue;
	} else {
		ri.Printf( PRINT_WARNING, "WARNING: packed_depth_stencil failed\n");
	}
	if (R_TestFbuffer_SeperateDS()) {
		seperateDepthStencilSupported = qtrue;
	} else {
		ri.Printf( PRINT_WARNING, "WARNING: seperate depth stencil failed\n");
	}
	if (R_TestFbuffer_texD()) {
		depthTextureSupported = qtrue;
	} else {
		ri.Printf( PRINT_WARNING, "WARNING: depth texture failed\n");
	}
	
	qglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &maxbuffers);
	if (maxbuffers < 2) {
		glMMEConfig.framebufferObject = qfalse;
		ri.Printf( PRINT_WARNING, "WARNING: Not enough color buffers available\n");
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
	}
		
		
	//set our main screen flags									
	screenbuff_flags = 0x00;
	if ((glConfig.stencilBits > 0)) {
		if ( packedDepthStencilSupported ) {
			screenbuff_flags |= FB_PACKED | FB_STENCIL;
		}
		else if ( seperateDepthStencilSupported ) {
			screenbuff_flags |= FB_SEPERATEZS | FB_STENCIL;
		}
	}
	
	if ((depthTextureSupported) && (r_framebuffer_rotoscope_zedge->integer)) {
		screenbuff_flags |= FB_ZTEXTURE;
	}
	screenbuff_flags |= FB_BACKBUFFER;
	
	screenbuff_flags |= FB_ZBUFFER;
	
	//create our main frame buffer
	status = R_CreateFBuffer( 	&screenBuffer, glConfig.vidWidth, 
								glConfig.vidHeight, screenbuff_flags);
	
	if (status) {
		// if the main fbuffer failed then we should disable framebuffer 
		// rendering
		glMMEConfig.framebufferObject = qfalse;
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer creation failed\n");
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
		//Reinit back to window rendering in case status fails
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0 );
		return;
	}
						
	//init our effects
	if (r_framebuffer_rotoscope->integer == 1) {
		R_FrameBuffer_RotoInit(&screenBuffer);
	}
	
	if (r_framebuffer_bloom->integer == 1) {
		R_FrameBuffer_BloomInit();
	}
	//we don't need an if here, if any effects before need a blur then this 
	//auto detects that its needed
	R_FrameBuffer_BlurInit();
	
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, *screenBuffer.front);

}


/* Startframe checks if the framebuffer is still active or was just activated */
void R_FrameBuffer_StartFrame( void ) {
	if (!glMMEConfig.framebufferObject || !glMMEConfig.shaderSupport) {
		return;
	}
	if (r_framebuffer->integer != 1) {
		return;
	}
	//Bind the framebuffer at the beginning to be drawn in
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, screenBuffer.fbo);
	qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
}


void R_FrameBuffer_DoEffect( void ) {


}

void R_FrameBuffer_EndFrame( void ) {
	qboolean screenDrawDone;
	GLuint *srcBuffer;

	if (!glMMEConfig.framebufferObject || !glMMEConfig.shaderSupport) {
		return;
	}
	if (r_framebuffer->integer != 1) {
		return;
	}
	
	//don't flip if drawing to front buffer
	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) == 0 ) {
		return;
	}
	
	GL_State( GLS_DEPTHTEST_DISABLE );
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	
	screenDrawDone = 0;
	qglColor4f( 1, 1, 1, 1 );
	
	srcBuffer = screenBuffer.front;
	
	if (r_framebuffer_rotoscope->integer == 1) {
		if (r_framebuffer_bloom->integer == 1) {
			//we need to draw to the back buffer
			qglDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
			R_FrameBuffer_RotoDraw(&screenBuffer, srcBuffer);
			srcBuffer = screenBuffer.back;
			qglDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		}
		else {
			qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
			R_FrameBuffer_RotoDraw(&screenBuffer, srcBuffer);
			screenDrawDone = 1;
		}
	}
	//everything before this point does not need blurred surfaces (or can do
	//with the last frames burred surface)
	//call the blur code, it auto detects weather its needed :)
	R_FrameBuffer_BlurDraw(srcBuffer);
	
	if (r_framebuffer_bloom->integer == 1) {
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		R_FrameBuffer_BloomDraw(srcBuffer);
		screenDrawDone = 1;
	}
	
	if (screenDrawDone == 0) {
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
		//R_SetGL2DSize(glConfig.vidWidth, glConfig.vidHeight);
		R_DrawQuad(	*srcBuffer, glConfig.vidWidth, glConfig.vidHeight);
	}
}

void R_FrameBuffer_Shutdown( void ) {
	//cleanup
	
	if (!r_framebuffer->integer) {
		return;
	}
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	
	
	if (r_framebuffer_bloom->integer == 1) {
		R_FrameBuffer_BloomDelete();
	}
	if ( needBlur == 1 ) {
		R_FrameBuffer_BlurDelete();
	}
	if ( r_framebuffer_rotoscope->integer == 1) {
		R_FrameBuffer_RotoDelete();
	}
	
	//delete the main screen buffer
	R_DeleteFBuffer(&screenBuffer);
}
#endif
