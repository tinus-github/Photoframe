//
//  gl-includes.h
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#ifndef gl_includes_h
#define gl_includes_h

#ifdef __APPLE__

#include <OpenGL/gl3.h>

#else

#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#endif

#endif /* gl_includes_h */
