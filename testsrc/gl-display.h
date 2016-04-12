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

// Data related to displaying images
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
} GLImageDisplayData;

typedef struct GLShapeInstanceData {
	unsigned int orientation;
	GLfloat objectX;
	GLfloat objectY;
	GLfloat objectWidth;
	GLfloat objectHeight;
} GLShapeInstanceData;

// Data related to a displayed image
typedef struct
{
	struct GLShapeInstanceData shape;
	
	// Texture info
	GLuint textureId;
	uint32_t textureWidth;
	uint32_t textureHeight;
} ImageInstanceData;

// General GL state data
typedef struct GL_STATE_T
{
	uint32_t width;
	uint32_t height;
	
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	
	EGL_DISPMANX_WINDOW_T nativewindow;
	ImageInstanceData *user_data;
	GLImageDisplayData *imageDisplayData;
	void (*draw_func) (struct GL_STATE_T* );
} GL_STATE_T;

void gl_image_draw(GL_STATE_T *p_state);
void gl_display_init(GL_STATE_T *state);
void gl_display_register_draw_func(GL_STATE_T *p_state, void (*gl_display_draw_func) (GL_STATE_T* ) );


GLuint gl_display_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc );

#endif /* gl_display_h */
