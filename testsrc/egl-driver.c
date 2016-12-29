//
//  egl-driver.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/12/2016.
//
//

#include "egl-driver.h"
#include "gl-texture.h"
#include "gl-shape.h"
#include "gl-tile.h"
#include "gl-stage.h"
#include "gl-container.h"
#include "gl-container-2d.h"
#include "gl-tiled-image.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"

#include <bcm_host.h>

#include <assert.h>

static void egl_driver_clear(egl_driver_data *data)
{
	gl_stage *stage = gl_stage_get_global_stage();
	
	// Set the viewport
	glViewport(0, 0, stage->data.width, stage->data.height);
	
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
}

static void egl_driver_clearf(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	egl_driver_data *data = (egl_driver_data *)action_data;
	
	egl_driver_clear(data);
}

static void egl_driver_swap(egl_driver_data *data)
{
	eglSwapBuffers(data->display, data->surface);
}

static void egl_driver_swapf(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	egl_driver_data *data = (egl_driver_data *)action_data;
	
	egl_driver_swap(data);
}

static void egl_driver_init_display(egl_driver_data *data)
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;
	
	bcm_host_init();
	
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
	data->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
	// initialize the EGL display connection
	result = eglInitialize(data->display, NULL, NULL);
	
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(data->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	
	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	
	
	// create an EGL rendering context
	data->context = eglCreateContext(data->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(data->context!=EGL_NO_CONTEXT);
	
	// create an EGL window surface
	unsigned int width, height;
	
	success = graphics_get_display_size(0 /* LCD */, &width, &height);
	assert( success >= 0 );
	
	gl_stage *stage = gl_stage_get_global_stage();
	stage->f->set_dimensions(stage, width, height);
	
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = width;
	dst_rect.height =height;
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = width << 16;
	src_rect.height = height << 16;
	
	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	
	dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
	
	data->nativewindow.element = dispman_element;
	data->nativewindow.width = width;
	data->nativewindow.height = height;
	vc_dispmanx_update_submit_sync( dispman_update );
	
	data->surface = eglCreateWindowSurface( data->display, config, &data->nativewindow, NULL );
	assert(data->surface != EGL_NO_SURFACE);
	
	// connect the context to the surface
	result = eglMakeCurrent(data->display, data->surface, data->surface, data->context);
	assert(EGL_FALSE != result);
}

static void egl_driver_register_renderloop(egl_driver_data *data)
{
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	
	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &egl_driver_clearf;
	renderloop_member->data.action_data = data;
	
	renderloop->f->append_child(renderloop, gl_renderloop_phase_clear, renderloop_member);
	
	renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &egl_driver_swapf;
	renderloop_member->data.action_data = data;
	
	renderloop->f->append_child(renderloop, gl_renderloop_phase_show, renderloop_member);
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
static GLuint egl_driver_load_shader(GLenum type, const GLchar *shaderSrc)
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

GLuint egl_driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;
	
	// Load the vertex/fragment shaders
	vertexShader = egl_driver_load_shader ( GL_VERTEX_SHADER, vertShaderSrc );
	if ( vertexShader == 0 )
		return 0;
	
	fragmentShader = egl_driver_load_shader ( GL_FRAGMENT_SHADER, fragShaderSrc );
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

egl_driver_data *egl_driver_init()
{
	egl_driver_data *data = calloc(1, sizeof(egl_driver_data));

	egl_driver_init_display(data);
	
	egl_driver_register_renderloop(data);
	
	return data;
}

void egl_driver_setup()
{
	gl_texture_setup();
	gl_shape_setup();
	gl_tile_setup();
	gl_stage_setup();
	gl_container_setup();
	gl_container_2d_setup();
	gl_tiled_image_setup();
	gl_renderloop_member_setup();
	gl_renderloop_setup();
}
