//
//  gl-display.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/16.
//
// This is actually a GL/bcm (for the Raspberry Pi) hybrid

#include "gl-display.h"

#include <bcm_host.h>

#include <assert.h>
#include <sys/time.h>

#include "../lib/linmath/linmath.h"

#include "gl-texture.h"
#include "gl-shape.h"
#include "gl-tile.h"
#include "gl-stage.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

static GL_STATE_T *global_gl_state = 0;
static void set_global_gl_state(GL_STATE_T *state);

void gl_display_setup();

///
// Create a shader object, load the shader source, and
// compile the shader.
//
static GLuint gl_display_load_shader(GLenum type, const GLchar *shaderSrc)
{
	GLuint shader;
	GLint compiled;
	// Create the shader object
	shader = glCreateShader(type);
	if(shader == 0)
		return 0;
	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	// Compile the shader
	glCompileShader(shader);
	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char* infoLog = malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint gl_display_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;
	
	// Load the vertex/fragment shaders
	vertexShader = gl_display_load_shader ( GL_VERTEX_SHADER, vertShaderSrc );
	if ( vertexShader == 0 )
		return 0;
	
	fragmentShader = gl_display_load_shader ( GL_FRAGMENT_SHADER, fragShaderSrc );
	if ( fragmentShader == 0 )
	{
		glDeleteShader( vertexShader );
		return 0;
	}
	
	// Create the program object
	programObject = glCreateProgram ( );
	
	if ( programObject == 0 )
		return 0;
	
	glAttachShader ( programObject, vertexShader );
	glAttachShader ( programObject, fragmentShader );
	
	// Link the program
	glLinkProgram ( programObject );
	
	// Check the link status
	glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
	
	if ( !linked )
	{
		GLint infoLen = 0;
		glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
		
		if ( infoLen > 1 )
		{
			char* infoLog = malloc (sizeof(char) * infoLen );
			
			glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
			fprintf (stderr, "Error linking program:\n%s\n", infoLog );
			
			free ( infoLog );
		}
		
		glDeleteProgram ( programObject );
		return 0;
	}
	
	// Free up no longer needed shader resources
	glDeleteShader ( vertexShader );
	glDeleteShader ( fragmentShader );
	
	return programObject;
}

void gl_display_init(GL_STATE_T *state)
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	set_global_gl_state(state);
	
	memset( state, 0, sizeof( GL_STATE_T) );
	
	bcm_host_init();
	gl_display_setup();
	
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	
	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	
	static const EGLint context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	
	EGLConfig config;
	
	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	
	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	
	
	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(state->context!=EGL_NO_CONTEXT);
	
	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &state->width, &state->height);
	assert( success >= 0 );
	
	gl_stage *stage = gl_stage_get_global_stage();
	stage->f->set_dimensions(stage, state->width, state->height);

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = state->width;
	dst_rect.height = state->height;
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = state->width << 16;
	src_rect.height = state->height << 16;
	
	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	
	dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
	
	state->nativewindow.element = dispman_element;
	state->nativewindow.width = state->width;
	state->nativewindow.height = state->height;
	vc_dispmanx_update_submit_sync( dispman_update );
	
	state->surface = eglCreateWindowSurface( state->display, config, &(state->nativewindow), NULL );
	assert(state->surface != EGL_NO_SURFACE);
	
	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);
}

static void gl_display_draw (GL_STATE_T *p_state)
{
	// Set the viewport
	glViewport ( 0, 0, p_state->width, p_state->height );
	
	// Clear the color buffer
	glClear ( GL_COLOR_BUFFER_BIT );
	
	
	if (p_state->draw_func != NULL)
		p_state->draw_func(p_state);
	
	eglSwapBuffers(p_state->display, p_state->surface);
}

void  esMainLoop (GL_STATE_T *esContext )
{
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	float totaltime = 0.0f;
	unsigned int frames = 0;
	
	gettimeofday ( &t1 , &tz );
	
	while(1)
	{
		gettimeofday(&t2, &tz);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		t1 = t2;
		
		ImageInstanceData *data = esContext->user_data;
		
		data->shape.objectX = t2.tv_usec / 10000;

		gl_display_draw(esContext);
		
		totaltime += deltatime;
		frames++;
		if (totaltime >  2.0f)
		{
			printf("%4d frames rendered in %1.4f seconds -> FPS=%3.4f\n", frames, totaltime, frames/totaltime);
			totaltime -= 2.0f;
			frames = 0;
		}
	}
}

void gl_display_register_draw_func(GL_STATE_T *p_state, void (*gl_display_draw_func) (GL_STATE_T* ) )
{
	p_state->draw_func = gl_display_draw_func;
}


void gl_display_setup()
{
	gl_texture_setup();
	gl_shape_setup();
	gl_tile_setup();
	gl_stage_setup();
}


// Global state hack for simplicity

static void set_global_gl_state(GL_STATE_T *state)
{
	global_gl_state = state;
}

GL_STATE_T *get_global_gl_state()
{
	return global_gl_state;
}
