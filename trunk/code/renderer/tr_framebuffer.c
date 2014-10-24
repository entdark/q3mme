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

//Sjoerd
//Butchered this up a bit for MME needs and only really has been tested for my ATI videocard
#include "tr_local.h"
#ifndef HIDDEN_FBO
#include "tr_glslprogs.h"
#include "qgl.h"

cvar_t *r_fbo;
cvar_t *r_fboMultiSample;
cvar_t *r_fboBlur;
cvar_t *r_fboWidth;
cvar_t *r_fboHeight;

#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

#define RGBA32F_ARB                      0x8814
#define RGB32F_ARB                       0x8815
#define ALPHA32F_ARB                     0x8816
#define INTENSITY32F_ARB                 0x8817
#define LUMINANCE32F_ARB                 0x8818
#define LUMINANCE_ALPHA32F_ARB           0x8819
#define RGBA16F_ARB                      0x881A
#define RGB16F_ARB                       0x881B
#define ALPHA16F_ARB                     0x881C
#define INTENSITY16F_ARB                 0x881D
#define LUMINANCE16F_ARB                 0x881E
#define LUMINANCE_ALPHA16F_ARB           0x881F

#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT GL_DEPTH_STENCIL_NV
#endif

#ifndef GL_UNSIGNED_INT_24_8_EXT
#define GL_UNSIGNED_INT_24_8_EXT GL_UNSIGNED_INT_24_8_NV
#endif

static qboolean	packedDepthStencilSupported;
static qboolean	depthTextureSupported;
static qboolean	seperateDepthStencilSupported;

static struct {
	frameBufferData_t *multiSample;
	frameBufferData_t *main;
	frameBufferData_t *blur;
	frameBufferData_t *dof;
	int screenWidth, screenHeight;
} fbo;

struct glslobj {
	const char **vertex_glsl;
	int vert_numSources;
	const char **fragment_glsl;
	int frag_numSources;
	GLuint vertex;
	GLuint fragment;
	GLuint program;
} glslobj;

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
	qglEnd();

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

static int CreateTextureBuffer( int width, int height, GLenum internalFormat, GLenum format, GLenum type ) {
	int ret = 0;
	int error = qglGetError();
	qglGenTextures( 1, &ret );
	qglBindTexture(	GL_TEXTURE_2D, ret );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	qglTexImage2D(	GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, 0 );
	error = qglGetError();
	return ret;
}

static int CreateRenderBuffer( int samples, int width, int height, GLenum bindType) {
	int ret = 0;
	qglGenRenderbuffers( 1, &ret );
	qglBindRenderbuffer( GL_RENDERBUFFER_EXT, ret );
	if ( samples ) {
		qglRenderbufferStorageMultisampleEXT( GL_RENDERBUFFER_EXT, samples, bindType, width, height );
	} else {
		qglRenderbufferStorage(	GL_RENDERBUFFER_EXT, bindType, width, height );
	}
	return ret;
}

//------------------------------
// better framebuffer creation
//------------------------------
// for this we do a more opengl way of figuring out what level of framebuffer
// objects are supported. we try each mode from 'best' to 'worst' until we 
// get a mode that works.


void R_FrameBufferDelete( frameBufferData_t* buffer ) {  
	if ( !buffer )
		return;
	qglDeleteFramebuffers(1, &(buffer->fbo));
	if ( buffer->color ) {
		if ( buffer->flags & FB_MULTISAMPLE ) {
			qglDeleteRenderbuffers(1, &(buffer->color) );
		} else {
			qglDeleteTextures( 1, &(buffer->color) );
		}
	}
	if ( buffer->depth ) {
		qglDeleteRenderbuffers(1, &(buffer->depth) );
	}
	if ( buffer->stencil ) {
		qglDeleteRenderbuffers(1, &(buffer->stencil) );
	}
	if ( buffer->packed ) {
		if ( buffer->flags & FB_MULTISAMPLE ) {
			qglDeleteRenderbuffers(1, &(buffer->packed) );
		} else {
			qglDeleteTextures( 1, &(buffer->packed) );
		}
	}
	free( buffer );
}



frameBufferData_t* R_FrameBufferCreate( int width, int height, int flags ) {
	frameBufferData_t *buffer;
	GLuint status;
	int samples = 0;

	buffer = malloc( sizeof( *buffer ) );
	memset( buffer, 0, sizeof( *buffer ) );
	buffer->flags = flags;
	buffer->width = width;
	buffer->height = height;

	//gen the frame buffer
	qglGenFramebuffers(1, &(buffer->fbo) );
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, buffer->fbo);

	if ( flags & FB_MULTISAMPLE ) {
		samples = r_fboMultiSample->integer;
	}

	if ( flags & FB_PACKED ) {
		if ( samples ) {
			buffer->packed = CreateRenderBuffer( samples, width, height, GL_DEPTH24_STENCIL8_EXT );
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->packed );
			qglFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->packed );
		} else {
			// Setup depth_stencil texture (not mipmap)
			buffer->packed = CreateTextureBuffer( width, height, GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT );
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->packed, 0);
			qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, buffer->packed, 0);
		}
	} else {
		if ( flags & FB_DEPTH ) {
			buffer->depth = CreateRenderBuffer( samples, width, height, GL_DEPTH_COMPONENT );
			qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->depth );
		}
		if ( flags & FB_STENCIL ) {
			buffer->stencil = CreateRenderBuffer( samples, width, height, GL_STENCIL_INDEX8_EXT );
			qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, buffer->stencil );
		}
	}
	/* Attach the color buffer */
	
	if ( samples ) {
		buffer->color = CreateRenderBuffer( samples, width, height, GL_RGBA );
		qglFramebufferRenderbuffer(	GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, buffer->color );
	} else if ( flags & FB_FLOAT16 ) {
		buffer->color = CreateTextureBuffer( width, height, RGBA16F_ARB, GL_RGBA, GL_FLOAT );
		qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, buffer->color, 0);
	} else if ( flags & FB_FLOAT32 ) {
		buffer->color = CreateTextureBuffer( width, height, RGBA32F_ARB, GL_RGBA, GL_FLOAT );
		qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, buffer->color, 0);
	} else {
		buffer->color = CreateTextureBuffer( width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE );
		qglFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, buffer->color, 0);
	}
		
	status = qglCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	if ( status != GL_FRAMEBUFFER_COMPLETE_EXT ) {
		switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            ri.Printf( PRINT_ALL, "Unsupported framebuffer format\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing attachment\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, duplicate attachment\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, attached images must have same dimensions\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, attached images must have same format\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing draw buffer\n" );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            ri.Printf( PRINT_ALL, "Framebuffer incomplete, missing read buffer\n" );
            break;
	    }
		R_FrameBufferDelete( buffer );
		return 0;
	}		
	return buffer;
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


void R_FrameBuffer_Draw( void ) {
	//draws the framebuffer to the screen, pretty simple really.
	if ( fbo.main ) {
		R_SetGL2DSize( glConfig.vidWidth, glConfig.vidHeight );
		R_DrawQuad(	fbo.main->color, glConfig.vidWidth, glConfig.vidHeight);
	}
}

void R_FrameBuffer_Init( void ) {
	int flags, width, height;

	memset( &fbo, 0, sizeof( fbo ) );
	r_fbo = ri.Cvar_Get( "r_fbo", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboBlur = ri.Cvar_Get( "r_fboBlur", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboWidth = ri.Cvar_Get( "r_fboWidth", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboHeight = ri.Cvar_Get( "r_fboHeight", "0", CVAR_ARCHIVE | CVAR_LATCH);	
	r_fboMultiSample = ri.Cvar_Get( "r_fboMultiSample", "0", CVAR_ARCHIVE | CVAR_LATCH);	

	// make sure all the commands added here are also		

	if (!glMMEConfig.framebufferObject || !glMMEConfig.shaderSupport) {
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
		return;
	}

	if ( !r_fbo->integer ) {
		return;
	}

	ri.Printf( PRINT_ALL, "----- Enabling FrameBuffer Path -----\n" );

	//set our main screen flags									
	flags = 0;
	if ( (glConfig.stencilBits > 0) ) {
		flags |= FB_PACKED;
	} else {
		flags |= FB_DEPTH;
	}
	
	fbo.screenWidth = glMMEConfig.glWidth;
	fbo.screenHeight = glMMEConfig.glHeight;

	width = r_fboWidth->integer;
	height = r_fboHeight->integer;
	//Illegal width/height use original opengl one
	if ( width <= 0 || height <= 0 ) {
		width = fbo.screenWidth;
		height = fbo.screenHeight;
	}

	//create our main frame buffer
	fbo.main = R_FrameBufferCreate( width, height, flags );

	if (!fbo.main) {
		// if the main fbuffer failed then we should disable framebuffer 
		// rendering
		glMMEConfig.framebufferObject = qfalse;
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer creation failed\n");
		ri.Printf( PRINT_WARNING, "WARNING: Framebuffer rendering path disabled\n");
		//Reinit back to window rendering in case status fails
		qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0 );
		return;
	}

	if ( r_fboMultiSample->integer ) {
		flags |= FB_MULTISAMPLE;
		flags &= ~FB_PACKED;
		flags |= FB_DEPTH;
		fbo.multiSample = R_FrameBufferCreate( width, height, flags );
	}

	glConfig.vidWidth = width;
	glConfig.vidHeight = height;

	if ( r_fboBlur->integer > 1 ) {
		flags = FB_FLOAT32;
		fbo.blur = R_FrameBufferCreate( width, height, flags );
	} else if ( r_fboBlur->integer  ) {
		flags = FB_FLOAT16;
		fbo.blur = R_FrameBufferCreate( width, height, flags );
	}

	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0 );
}

static qboolean usedFloat;

/* Startframe checks if the framebuffer is still active or was just activated */
void R_FrameBuffer_StartFrame( void ) {
	if ( !fbo.main ) {
		return;
	}
	if ( fbo.multiSample ) {
		//Bind the framebuffer at the beginning to be drawn in
		qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.multiSample->fbo );
	} else {
		qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
	}
	qglDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	usedFloat = qfalse;
}

#endif
qboolean R_FrameBuffer_Blur( float scale, int frame, int total ) {
#ifndef HIDDEN_FBO
	float c;
	if ( !fbo.blur )
		return qfalse;
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.blur->fbo );
	qglDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	//The color used to blur add this frame
	c = scale;
	qglColor4f( c, c, c, 1 );
	if ( frame == 0 ) {
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO | GLS_DEPTHTEST_DISABLE );
	} else {
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHTEST_DISABLE );
	}
	R_SetGL2DSize( glConfig.vidWidth, glConfig.vidHeight );
	R_DrawQuad(	fbo.main->color, glConfig.vidWidth, glConfig.vidHeight );
	//Reset fbo
	qglBindFramebuffer( GL_FRAMEBUFFER_EXT, fbo.main->fbo );
	qglDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );
	usedFloat = qtrue;
	if ( frame == total - 1 ) {
		qglColor4f( 1, 1, 1, 1 );
		GL_State( GLS_DEPTHTEST_DISABLE );
		R_DrawQuad(	fbo.blur->color, glConfig.vidWidth, glConfig.vidHeight );
	}
	return qtrue;
#else
	return qfalse;
#endif
}
#ifndef HIDDEN_FBO


void R_FrameBuffer_EndFrame( void ) {
	if ( !fbo.main ) {
		return;
	}
	if ( fbo.multiSample ) {
		const frameBufferData_t* src = fbo.multiSample;
		const frameBufferData_t* dst = fbo.main;

		qglBindFramebuffer( GL_READ_FRAMEBUFFER_EXT, src->fbo );
		qglBindFramebuffer( GL_DRAW_FRAMEBUFFER_EXT, dst->fbo );
		qglBlitFramebufferEXT(0, 0, src->width, src->height, 0, 0, dst->width, dst->height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
	}
	GL_State( GLS_DEPTHTEST_DISABLE );
	qglColor4f( 1, 1, 1, 1 );
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	R_SetGL2DSize( fbo.screenWidth, fbo.screenHeight );
	if ( usedFloat ) {
		R_DrawQuad(	fbo.blur->color, fbo.screenWidth, fbo.screenHeight );
	} else {
		R_DrawQuad(	fbo.main->color, fbo.screenWidth, fbo.screenHeight );
	}
	usedFloat = qfalse;
}



void R_FrameBuffer_Shutdown( void ) {
	qglBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	R_FrameBufferDelete( fbo.main );
	R_FrameBufferDelete( fbo.blur );
	R_FrameBufferDelete( fbo.multiSample );
}
#endif
