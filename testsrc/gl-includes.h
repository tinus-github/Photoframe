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

#include <assert.h>

static void __attribute__((unused)) check_gl_error()
{
	GLenum glError = glGetError();
	assert (!glError);
}

static void __attribute__((unused)) check_gl_framebuffer()
{
	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	assert (framebufferStatus == GL_FRAMEBUFFER_COMPLETE);
}

#endif /* gl_includes_h */
