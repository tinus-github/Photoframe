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

typedef struct egl_display_data {
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
} egl_display_data;

void egl_display_setup();
egl_display_data *egl_display_init();

#endif /* egl_driver_h */
