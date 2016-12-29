//
//  egl-driver.h
//  Photoframe
//
//  Created by Martijn Vernooij on 29/12/2016.
//
//

#ifndef egl_driver_h
#define egl_driver_h

#include <stdio.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef struct egl_driver_data {
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	
	EGL_DISPMANX_WINDOW_T nativewindow;
} egl_driver_data;

void egl_driver_setup();
egl_driver_data *egl_driver_init();

GLuint egl_driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc );

#endif /* egl_driver_h */
