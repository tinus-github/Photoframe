//
//  gl-display.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/16.
//
//

#ifndef gl_display_h
#define gl_display_h

#include <stdio.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

// Data related to a displayed image
typedef struct
{
	// Handle to a program object
	GLuint programObject;
	
	// Attribute locations
	GLint  positionLoc;
	GLint  texCoordLoc;
	
	// Uniforms locations
	GLint  projectionLoc;
	GLint  modelViewLoc;
	
	// Sampler location
	GLint samplerLoc;
	
	// Texture info
	GLuint textureId;
	uint32_t textureWidth;
	uint32_t textureHeight;
	
	unsigned int orientation;
} ImageUserData;

// General GL state data
typedef struct GL_STATE_T
{
	uint32_t width;
	uint32_t height;
	
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	
	EGL_DISPMANX_WINDOW_T nativewindow;
	ImageUserData *user_data;
	void (*draw_func) (struct GL_STATE_T* );
} GL_STATE_T;

void Draw(GL_STATE_T *p_state);

#endif /* gl_display_h */
